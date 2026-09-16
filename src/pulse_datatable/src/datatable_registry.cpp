#include "datatable_internal.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <memory>

namespace pulse::datatable {

namespace {

constexpr const char* kAllowedNameChars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

void format_error(std::string& error, const char* format, ...) {
    char buffer[512];
    std::va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    error.assign(buffer);
}

bool name_is_valid(std::string_view name) {
    if (name.empty() || name.find_first_not_of(kAllowedNameChars) != std::string_view::npos) {
        return false;
    }
    return true;
}

bool type_is_valid(EPulseDataTableColumnType type) {
    return type >= PULSE_DATA_TABLE_COLUMN_TYPE_INT && type < PULSE_DATA_TABLE_COLUMN_TYPE_COUNT;
}

bool enum_whitelist_contains(const PulseDataTableEnumDesc& desc, std::string_view candidate) {
    for (size_t i = 0; i < desc.values_count; ++i) {
        if (desc.p_values[i] && candidate == desc.p_values[i]) {
            return true;
        }
    }
    return false;
}

bool validate_column_set(const PulseDataTableSchemaDesc* schema, const PulseDataTableColumnDesc* columns, size_t columns_count, std::string& error) {
    for (size_t i = 0; i < columns_count; ++i) {
        const PulseDataTableColumnDesc& column = columns[i];
        if (!column.name || !column.name[0]) {
            format_error(error, "schema '%s' declares a column without a name", schema->name);
            return false;
        }
        if (!name_is_valid(column.name)) {
            format_error(error, "column '%s' has invalid characters", column.name);
            return false;
        }
        for (size_t j = 0; j < i; ++j) {
            if (columns[j].name && std::strcmp(columns[j].name, column.name) == 0) {
                format_error(error, "schema '%s' declares column '%s' twice", schema->name, column.name);
                return false;
            }
        }
        if (!type_is_valid(column.type)) {
            format_error(error, "column '%s' has an invalid type", column.name);
            return false;
        }
        const bool numeric = column.type == PULSE_DATA_TABLE_COLUMN_TYPE_INT || column.type == PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT;
        if ((column.has_min || column.has_max) && !numeric) {
            format_error(error, "column '%s' cannot declare min or max", column.name);
            return false;
        }
        if (column.has_min && column.has_max && column.min_value > column.max_value) {
            format_error(error, "column '%s' has a minimum above its maximum", column.name);
            return false;
        }
        if (column.struct_type && column.type != PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT) {
            format_error(error, "column '%s' is not a struct but names a struct type", column.name);
            return false;
        }
        if (column.enum_type && column.type != PULSE_DATA_TABLE_COLUMN_TYPE_ENUM) {
            format_error(error, "column '%s' is not an enum but names an enum type", column.name);
            return false;
        }
        if (column.ref_type && column.type != PULSE_DATA_TABLE_COLUMN_TYPE_REF) {
            format_error(error, "column '%s' is not a reference but names a table", column.name);
            return false;
        }
        if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT) {
            if (!column.struct_type || !find_struct(schema, column.struct_type)) {
                format_error(error, "column '%s' references unknown struct type '%s'", column.name, column.struct_type ? column.struct_type : "");
                return false;
            }
            if (column.has_default) {
                format_error(error, "struct column '%s' cannot declare a default", column.name);
                return false;
            }
        }
        if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_ENUM) {
            const PulseDataTableEnumDesc* desc = column.enum_type ? find_enum(schema, column.enum_type) : nullptr;
            if (!desc) {
                format_error(error, "column '%s' references unknown enum type '%s'", column.name, column.enum_type ? column.enum_type : "");
                return false;
            }
            if (column.has_default && (!column.default_string || !enum_whitelist_contains(*desc, column.default_string))) {
                format_error(error, "column '%s' default value '%s' is not in enum '%s'", column.name, column.default_string ? column.default_string : "", column.enum_type);
                return false;
            }
        }
        if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_REF && !column.ref_type) {
            format_error(error, "column '%s' references no table", column.name);
            return false;
        }
        if (column.has_default && !numeric && column.type != PULSE_DATA_TABLE_COLUMN_TYPE_BOOL && column.type != PULSE_DATA_TABLE_COLUMN_TYPE_STRING && column.type != PULSE_DATA_TABLE_COLUMN_TYPE_ENUM && column.type != PULSE_DATA_TABLE_COLUMN_TYPE_REF) {
            format_error(error, "column '%s' cannot declare a default", column.name);
            return false;
        }
        const bool string_like = column.type == PULSE_DATA_TABLE_COLUMN_TYPE_STRING || column.type == PULSE_DATA_TABLE_COLUMN_TYPE_ENUM || column.type == PULSE_DATA_TABLE_COLUMN_TYPE_REF;
        if (column.has_default && string_like && !column.default_string) {
            format_error(error, "column '%s' default must be a string", column.name);
            return false;
        }
    }
    return true;
}

bool validate_schema(const PulseDataTableSchemaDesc* desc, std::string& error) {
    if (!name_is_valid(desc->name)) {
        format_error(error, "schema name '%s' is invalid", desc->name ? desc->name : "");
        return false;
    }
    if (desc->columns_count == 0) {
        format_error(error, "schema '%s' declares no columns", desc->name);
        return false;
    }
    if (desc->key_column >= desc->columns_count) {
        format_error(error, "schema '%s' key column is out of range", desc->name);
        return false;
    }
    const PulseDataTableColumnDesc& key = desc->p_columns[desc->key_column];
    if (key.type != PULSE_DATA_TABLE_COLUMN_TYPE_INT && key.type != PULSE_DATA_TABLE_COLUMN_TYPE_STRING) {
        format_error(error, "key column '%s' must be an int or a string", key.name);
        return false;
    }
    if (key.has_default || key.has_min || key.has_max) {
        format_error(error, "key column '%s' cannot declare a default or a range", key.name);
        return false;
    }
    if (desc->fill_row && desc->key_is_int != (key.type == PULSE_DATA_TABLE_COLUMN_TYPE_INT)) {
        format_error(error, "key flag of schema '%s' does not match its key column type", desc->name);
        return false;
    }
    for (size_t i = 0; i < desc->structs_count; ++i) {
        const PulseDataTableStructDesc& type = desc->p_structs[i];
        if (!name_is_valid(type.name)) {
            format_error(error, "schema '%s' declares a struct type with an invalid name", desc->name);
            return false;
        }
        if (type.columns_count == 0 || !type.p_columns) {
            format_error(error, "struct type '%s' declares no columns", type.name);
            return false;
        }
        for (size_t j = 0; j < i; ++j) {
            if (std::strcmp(desc->p_structs[j].name, type.name) == 0) {
                format_error(error, "schema '%s' declares struct type '%s' twice", desc->name, type.name);
                return false;
            }
        }
        if (!validate_column_set(desc, type.p_columns, type.columns_count, error)) {
            return false;
        }
    }
    for (size_t i = 0; i < desc->enums_count; ++i) {
        const PulseDataTableEnumDesc& type = desc->p_enums[i];
        if (!name_is_valid(type.name)) {
            format_error(error, "schema '%s' declares an enum type with an invalid name", desc->name);
            return false;
        }
        if (type.values_count == 0 || !type.p_values) {
            format_error(error, "enum type '%s' declares no values", type.name);
            return false;
        }
        for (size_t j = 0; j < i; ++j) {
            if (std::strcmp(desc->p_enums[j].name, type.name) == 0) {
                format_error(error, "schema '%s' declares enum type '%s' twice", desc->name, type.name);
                return false;
            }
        }
        for (size_t v = 0; v < type.values_count; ++v) {
            if (!type.p_values[v] || !type.p_values[v][0]) {
                format_error(error, "enum type '%s' declares an empty value", type.name);
                return false;
            }
            for (size_t w = 0; w < v; ++w) {
                if (std::strcmp(type.p_values[w], type.p_values[v]) == 0) {
                    format_error(error, "enum type '%s' declares duplicate value '%s'", type.name, type.p_values[v]);
                    return false;
                }
            }
        }
    }
    return validate_column_set(desc, desc->p_columns, desc->columns_count, error);
}

void copy_columns(OwnedSchema& store, const PulseDataTableColumnDesc* source, size_t count, std::vector<PulseDataTableColumnDesc>& out) {
    for (size_t i = 0; i < count; ++i) {
        PulseDataTableColumnDesc column = source[i];
        column.name = store.intern(column.name);
        column.default_string = column.default_string ? store.intern(column.default_string) : nullptr;
        column.struct_type = column.struct_type ? store.intern(column.struct_type) : nullptr;
        column.ref_type = column.ref_type ? store.intern(column.ref_type) : nullptr;
        column.enum_type = column.enum_type ? store.intern(column.enum_type) : nullptr;
        out.push_back(column);
    }
}

bool layout_struct(OwnedSchema& store, size_t index, size_t depth, std::string& error) {
    PulseDataTableStructDesc& type = store.structs[index];
    if (type.size > 0) {
        return true;
    }
    if (depth >= store.structs.size()) {
        format_error(error, "struct type '%s' contains itself", type.name);
        return false;
    }
    for (PulseDataTableColumnDesc& column : store.struct_columns[index]) {
        if (column.type != PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT) {
            continue;
        }
        size_t dependency = 0;
        for (; dependency < store.structs.size(); ++dependency) {
            if (std::strcmp(store.structs[dependency].name, column.struct_type) == 0) {
                break;
            }
        }
        if (dependency >= store.structs.size()) {
            format_error(error, "struct type '%s' references struct '%s' that is not embedded in the schema", type.name, column.struct_type);
            return false;
        }
        if (!layout_struct(store, dependency, depth + 1, error)) {
            return false;
        }
    }
    PulseDataTableSchemaDesc lookup{};
    lookup.p_structs = store.structs.empty() ? nullptr : store.structs.data();
    lookup.structs_count = store.structs.size();
    uint32_t cursor = 0;
    uint32_t max_align = 1;
    for (PulseDataTableColumnDesc& column : store.struct_columns[index]) {
        uint32_t offset = align_up(cursor, column_align(&lookup, column));
        column.offset = offset;
        cursor = offset + column_size(&lookup, column);
        max_align = std::max(max_align, column_align(&lookup, column));
    }
    type.size = std::max(align_up(cursor, max_align), 1u);
    type.align = max_align;
    return true;
}

} // namespace

Registry::Registry(std::pmr::memory_resource* resource)
    : resource_(resource),
      schemas_(resource) {
}

Registry::~Registry() {
    for (Table* table : table_order_) {
        delete table;
    }
    table_order_.clear();
    slots_.clear();
}

PulseAppId Registry::app() const {
    return app_;
}

PulseAssetSystemId Registry::asset_system() const {
    return asset_system_;
}

std::pmr::memory_resource* Registry::resource() const {
    return resource_;
}

void Registry::bind(PulseAppId app_id, PulseAssetSystemId asset_system_id) {
    app_ = app_id;
    asset_system_ = asset_system_id;
}

EPulseResult Registry::register_schema(const PulseDataTableSchemaDesc* desc, const char** out_error) {
    auto fail = [&](EPulseResult code, std::string_view message) {
        if (out_error) {
            *out_error = intern_error(message);
        }
        return code;
    };
    if (!desc || !desc->name || !desc->name[0]) {
        return fail(PULSE_RESULT_ERROR_INVALID_ARGUMENT, "schema descriptor or name is missing");
    }
    if (desc->struct_size != sizeof(PulseDataTableSchemaDesc) || desc->version != PULSE_DATA_TABLE_PLUGIN_DESC_VERSION) {
        return fail(PULSE_RESULT_ERROR_INVALID_ARGUMENT, "schema descriptor struct_size or version is invalid");
    }
    if ((desc->columns_count > 0 && !desc->p_columns) || (desc->structs_count > 0 && !desc->p_structs) || (desc->enums_count > 0 && !desc->p_enums)) {
        return fail(PULSE_RESULT_ERROR_INVALID_ARGUMENT, "schema descriptor declares an array without storage");
    }
    std::string error{};
    if (!validate_schema(desc, error)) {
        return fail(PULSE_RESULT_ERROR_INVALID_ARGUMENT, error);
    }
    std::pmr::string name(desc->name, resource_);
    if (schemas_.find(name) != schemas_.end()) {
        format_error(error, "schema '%s' is already registered", desc->name);
        return fail(PULSE_RESULT_ERROR_DUPLICATE_PLUGIN, error);
    }
    if (desc->fill_row) {
        schemas_.emplace(name, desc);
        if (out_error) {
            *out_error = nullptr;
        }
        return PULSE_RESULT_OK;
    }
    std::unique_ptr<OwnedSchema> store = std::make_unique<OwnedSchema>();
    copy_columns(*store, desc->p_columns, desc->columns_count, store->columns);
    for (size_t i = 0; i < desc->structs_count; ++i) {
        const PulseDataTableStructDesc& type = desc->p_structs[i];
        store->struct_columns.push_back({});
        copy_columns(*store, type.p_columns, type.columns_count, store->struct_columns.back());
        PulseDataTableStructDesc copy{};
        copy.name = store->intern(type.name);
        copy.columns_count = type.columns_count;
        store->structs.push_back(copy);
    }
    for (size_t i = 0; i < store->structs.size(); ++i) {
        store->structs[i].p_columns = store->struct_columns[i].data();
    }
    for (size_t i = 0; i < desc->enums_count; ++i) {
        const PulseDataTableEnumDesc& type = desc->p_enums[i];
        PulseDataTableEnumDesc copy{};
        copy.name = store->intern(type.name);
        copy.p_values = store->intern_values(type.p_values, type.values_count);
        copy.values_count = type.values_count;
        store->enums.push_back(copy);
    }
    store->desc.struct_size = sizeof(PulseDataTableSchemaDesc);
    store->desc.version = PULSE_DATA_TABLE_PLUGIN_DESC_VERSION;
    store->desc.name = store->intern(desc->name);
    store->desc.p_columns = store->columns.data();
    store->desc.columns_count = store->columns.size();
    store->desc.p_structs = store->structs.empty() ? nullptr : store->structs.data();
    store->desc.structs_count = store->structs.size();
    store->desc.p_enums = store->enums.empty() ? nullptr : store->enums.data();
    store->desc.enums_count = store->enums.size();
    store->desc.key_column = desc->key_column;
    store->desc.fill_row = nullptr;
    for (size_t i = 0; i < store->structs.size(); ++i) {
        if (!layout_struct(*store, i, 0, error)) {
            return fail(PULSE_RESULT_ERROR_INVALID_ARGUMENT, error);
        }
    }
    uint32_t cursor = 0;
    uint32_t max_align = 1;
    for (PulseDataTableColumnDesc& column : store->columns) {
        uint32_t alignment = column_align(&store->desc, column);
        column.offset = align_up(cursor, alignment);
        cursor = column.offset + column_size(&store->desc, column);
        max_align = std::max(max_align, alignment);
    }
    for (PulseDataTableColumnDesc& column : store->columns) {
        if (!column.has_default) {
            continue;
        }
        if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_INT) {
            column.default_float = static_cast<double>(column.default_int);
        }
        if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT) {
            column.default_int = static_cast<int64_t>(column.default_float);
        }
    }
    store->desc.key_is_int = store->columns[store->desc.key_column].type == PULSE_DATA_TABLE_COLUMN_TYPE_INT;
    schemas_.emplace(name, &store->desc);
    owned_.push_back(std::move(store));
    if (out_error) {
        *out_error = nullptr;
    }
    return PULSE_RESULT_OK;
}

const PulseDataTableSchemaDesc* Registry::find_schema(std::string_view name) const {
    if (name.empty()) {
        return nullptr;
    }
    for (const auto& entry : schemas_) {
        if (std::string_view(entry.first.c_str(), entry.first.size()) == name) {
            return entry.second;
        }
    }
    return nullptr;
}

void Table::reset(const PulseDataTableSchemaDesc* schema_desc, Registry* owner, TableSlot* owner_slot) {
    schema = schema_desc;
    registry = owner;
    slot = owner_slot;
    name.clear();
    rows.clear();
    row_count = 0;
    string_index.clear();
    int_index.clear();
    vault.bytes.clear();
    vault.used = 0;
    source_text.clear();
}

Table* Registry::create_table(PulseAssetHandle handle, const PulseDataTableSchemaDesc* schema) {
    Table* table = new Table{};
    table->reset(schema, this, nullptr);
    table_order_.push_back(table);
    slots_.emplace(handle.index, table);
    return table;
}

void Registry::destroy_table(Table* table) {
    if (!table) {
        return;
    }
    for (size_t i = 0; i < table_order_.size(); ++i) {
        if (table_order_[i] == table) {
            table_order_.erase(table_order_.begin() + static_cast<ptrdiff_t>(i));
            break;
        }
    }
    for (auto it = slots_.begin(); it != slots_.end(); ++it) {
        if (it->second == table) {
            slots_.erase(it);
            break;
        }
    }
    delete table;
}

void Registry::bind_slot(PulseAssetHandle handle, TableSlot* slot) {
    auto it = slots_.find(handle.index);
    if (it == slots_.end()) {
        return;
    }
    slot->table = it->second;
    slot->schema = it->second->schema;
    it->second->slot = slot;
    live_.emplace(slot, it->second);
}

void Registry::release_slot(TableSlot* slot) {
    if (!slot) {
        return;
    }
    auto it = live_.find(slot);
    if (it != live_.end()) {
        it->second->slot = nullptr;
        live_.erase(it);
    }
    slot->table = nullptr;
    slot->schema = nullptr;
    slot->rows = nullptr;
    slot->row_count = 0;
}

Table* Registry::find_table(std::string_view name) const {
    if (name.empty()) {
        return nullptr;
    }
    for (auto it = table_order_.rbegin(); it != table_order_.rend(); ++it) {
        if ((*it)->name == name) {
            return *it;
        }
    }
    return nullptr;
}

TableSlot* Registry::table_slot(std::string_view name) const {
    Table* table = find_table(name);
    return table ? table->slot : nullptr;
}

void Registry::add_load_record(PulseAssetHandle handle, LoadRecord&& record) {
    load_records_[handle.index] = std::move(record);
}

LoadRecord* Registry::find_load_record(PulseAssetHandle handle) {
    auto it = load_records_.find(handle.index);
    return it == load_records_.end() ? nullptr : &it->second;
}

void Registry::remove_load_record(PulseAssetHandle handle) {
    load_records_.erase(handle.index);
}

const char* Registry::intern_error(std::string_view message) {
    if (message.empty()) {
        return nullptr;
    }
    error_text_.emplace_back(message);
    return error_text_.back().c_str();
}

RowKey read_key(const Table& table, const void* row) {
    RowKey key{};
    const PulseDataTableColumnDesc& column = table.schema->p_columns[table.schema->key_column];
    const uint8_t* bytes = static_cast<const uint8_t*>(row) + column.offset;
    if (table.schema->key_is_int) {
        key.number = *reinterpret_cast<const int64_t*>(bytes);
    } else {
        key.is_string = true;
        key.text = *reinterpret_cast<const std::string_view*>(bytes);
    }
    return key;
}

} // namespace pulse::datatable
