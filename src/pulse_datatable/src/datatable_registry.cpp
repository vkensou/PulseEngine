#include "datatable_internal.h"

#include <cstdio>

namespace pulse::datatable {

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

EPulseResult Registry::register_schema(const PulseDataTableSchemaDesc* desc) {
    if (!desc || !desc->name || !desc->name[0]) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (desc->struct_size != sizeof(PulseDataTableSchemaDesc) || desc->version != PULSE_DATA_TABLE_PLUGIN_DESC_VERSION) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if ((desc->columns_count > 0 && !desc->p_columns) || (desc->structs_count > 0 && !desc->p_structs) || (desc->enums_count > 0 && !desc->p_enums)) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (desc->columns_count == 0 || desc->key_column >= desc->columns_count || !desc->fill_row) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    std::pmr::string name(desc->name, resource_);
    if (schemas_.find(name) != schemas_.end()) {
        return PULSE_RESULT_ERROR_DUPLICATE_PLUGIN;
    }
    schemas_.emplace(name, desc);
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
