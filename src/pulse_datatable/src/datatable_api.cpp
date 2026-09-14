#include "datatable_internal.h"

#include <cstdio>

#include <cstring>

using namespace pulse::datatable;

namespace {

Registry* to_registry(PulseDataTableSystemId system) {
    return system ? static_cast<PulseDataTableSystem*>(system)->registry_ptr() : nullptr;
}

const TableSlot* to_table(PulseDataTableId table) {
    return reinterpret_cast<const TableSlot*>(table);
}

} // namespace

extern "C" {

EPulseResult pulse_data_table_system_register_schema(PulseDataTableSystemId _this, const PulseDataTableSchemaDesc* desc) {
    Registry* registry = to_registry(_this);
    return registry ? registry->register_schema(desc) : PULSE_RESULT_ERROR_INVALID_ARGUMENT;
}

PulseAssetRequest pulse_data_table_system_load(PulseDataTableSystemId _this, const char* schema, const char* path) {
    PulseAssetRequest invalid = pulse_asset_request_make_invalid();
    Registry* registry = to_registry(_this);
    if (!registry || !schema || !schema[0] || !path || !path[0]) {
        return invalid;
    }
    if (!registry->find_schema(schema)) {
        return invalid;
    }
    PulseAssetLoadDesc load_desc{};
    load_desc.struct_size = sizeof(PulseAssetLoadDesc);
    load_desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    load_desc.type_id = PULSE_TYPE_DATA_TABLE;
    load_desc.path = path;
    return pulse_asset_system_load(registry->asset_system(), &load_desc);
}

bool pulse_data_table_system_is_ready(Const_PulseDataTableSystemId _this, PulseAssetRequest request) {
    Registry* registry = to_registry(const_cast<PulseDataTableSystemId>(_this));
    return registry ? pulse_asset_system_is_ready(registry->asset_system(), request) : false;
}

bool pulse_data_table_system_is_alive(Const_PulseDataTableSystemId _this, PulseAssetRequest request) {
    Registry* registry = to_registry(const_cast<PulseDataTableSystemId>(_this));
    return registry ? pulse_asset_system_is_alive(registry->asset_system(), request) : false;
}

const char* pulse_data_table_system_get_error(Const_PulseDataTableSystemId _this, PulseAssetRequest request) {
    Registry* registry = to_registry(const_cast<PulseDataTableSystemId>(_this));
    return registry ? pulse_asset_system_get_error(registry->asset_system(), request) : nullptr;
}

PulseDataTableId pulse_data_table_system_get(PulseDataTableSystemId _this, PulseAssetRequest request) {
    Registry* registry = to_registry(_this);
    if (!registry || !pulse_asset_system_is_ready(registry->asset_system(), request)) {
        return nullptr;
    }
    void* ptr = nullptr;
    PulseAssetHandle handle = pulse_asset_system_get_handle(registry->asset_system(), request);
    if (!pulse_asset_system_borrow(registry->asset_system(), handle, &ptr, nullptr)) {
        return nullptr;
    }
    return static_cast<PulseDataTableId>(ptr);
}

PulseDataTableId pulse_data_table_system_get_by_name(Const_PulseDataTableSystemId _this, const char* schema) {
    Registry* registry = to_registry(const_cast<PulseDataTableSystemId>(_this));
    return registry ? reinterpret_cast<PulseDataTableId>(registry->table_slot(schema ? schema : "")) : nullptr;
}

const PulseDataTableSchemaDesc* pulse_data_table_system_get_schema(Const_PulseDataTableSystemId _this, const char* schema) {
    Registry* registry = to_registry(const_cast<PulseDataTableSystemId>(_this));
    return registry ? registry->find_schema(schema ? schema : "") : nullptr;
}

const char* pulse_data_table_get_name(PulseDataTableId table) {
    const TableSlot* self = to_table(table);
    return self && self->table ? self->table->name.c_str() : nullptr;
}
uint32_t pulse_data_table_row_count(PulseDataTableId table) {
    const TableSlot* self = to_table(table);
    return self && self->table ? self->table->row_count : 0;
}

const void* pulse_data_table_rows(PulseDataTableId table, uint32_t* out_count) {
    const TableSlot* self = to_table(table);
    const Table* owner = self ? self->table : nullptr;
    if (out_count) {
        *out_count = owner ? owner->row_count : 0;
    }
    return owner && owner->row_count > 0 ? owner->rows.data() : nullptr;
}

const void* pulse_data_table_find_row(PulseDataTableId table, const char* key) {
    const TableSlot* self = to_table(table);
    if (!self || !self->table || !key || self->schema->key_is_int) {
        return nullptr;
    }
    const Table* owner = self->table;
    auto it = owner->string_index.find(key);
    if (it == owner->string_index.end()) {
        return nullptr;
    }
    return owner->rows.data() + static_cast<size_t>(it->second) * table_row_size(self->schema);
}

const void* pulse_data_table_find_row_int(PulseDataTableId table, int64_t key) {
    const TableSlot* self = to_table(table);
    if (!self || !self->table || !self->schema->key_is_int) {
        return nullptr;
    }
    const Table* owner = self->table;
    auto it = owner->int_index.find(key);
    if (it == owner->int_index.end()) {
        return nullptr;
    }
    return owner->rows.data() + static_cast<size_t>(it->second) * table_row_size(self->schema);
}

bool pulse_data_table_field_set_int(void* row, const PulseDataTableColumnDesc* column, int64_t value, const char** out_error) {
    EPulseDataTableError error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
    const char* message = nullptr;
    if (!check_range(*column, static_cast<double>(value), error, message)) {
        if (out_error) {
            *out_error = message;
        }
        return false;
    }
    *reinterpret_cast<int64_t*>(static_cast<uint8_t*>(row) + column->offset) = value;
    return true;
}

bool pulse_data_table_field_set_float(void* row, const PulseDataTableColumnDesc* column, double value, const char** out_error) {
    EPulseDataTableError error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
    const char* message = nullptr;
    if (!check_range(*column, value, error, message)) {
        if (out_error) {
            *out_error = message;
        }
        return false;
    }
    *reinterpret_cast<double*>(static_cast<uint8_t*>(row) + column->offset) = value;
    return true;
}

bool pulse_data_table_field_set_bool(void* row, const PulseDataTableColumnDesc* column, bool value, const char** out_error) {
    (void)out_error;
    *reinterpret_cast<bool*>(static_cast<uint8_t*>(row) + column->offset) = value;
    return true;
}

bool pulse_data_table_field_set_string(void* row, const PulseDataTableColumnDesc* column, const void* value, const char** out_error) {
    if (!value) {
        if (out_error) {
            *out_error = "string value is null";
        }
        return false;
    }
    *reinterpret_cast<std::string_view*>(static_cast<uint8_t*>(row) + column->offset) = *static_cast<const std::string_view*>(value);
    return true;
}

const void* pulse_data_table_fill_context_resolve_ref(const void* context, const PulseDataTableColumnDesc* column, const PulseDatalist* node, const char** out_error) {
    const auto* fill = static_cast<const FillContext*>(context);
    if (!fill || !fill->owner || !column || !node || !fill->dependencies) {
        if (out_error) {
            *out_error = "fill context is not available";
        }
        return nullptr;
    }
    auto* owner = static_cast<Table*>(fill->owner);
    if (!owner->schema) {
        if (out_error) {
            *out_error = "fill context schema is not available";
        }
        return nullptr;
    }
    std::string_view key{};
    if (!decode_string(node, nullptr, key) || key.empty()) {
        if (out_error) {
            *out_error = "reference column expects a string key";
        }
        return nullptr;
    }
    Registry* registry = owner->registry;
    if (!registry) {
        if (out_error) {
            *out_error = "data table registry is not available";
        }
        return nullptr;
    }
    const PulseDataTableSchemaDesc* target_schema = registry->find_schema(column->ref_type ? column->ref_type : "");
    if (!target_schema) {
        if (out_error) {
            *out_error = "referenced schema is not registered";
        }
        return nullptr;
    }
    Table* target = registry->find_table(column->ref_type ? column->ref_type : "");
    if (!target) {
        if (out_error) {
            *out_error = "referenced table is not loaded";
        }
        return nullptr;
    }
    const void* found = pulse_data_table_find_row(reinterpret_cast<PulseDataTableId>(target->slot), std::string(key).c_str());
    if (!found) {
        if (out_error) {
            *out_error = "referenced row does not exist";
        }
        return nullptr;
    }
    return found;
}

int32_t pulse_data_table_enum_lookup(const PulseDataTableSchemaDesc* schema, const PulseDataTableColumnDesc* column, const char* value) {
    if (!schema || !column || !value) {
        return -1;
    }
    const PulseDataTableEnumDesc* desc = find_enum(schema, column->enum_type ? column->enum_type : "");
    if (!desc) {
        return -1;
    }
    for (size_t i = 0; i < desc->values_count; ++i) {
        const char* candidate = desc->p_values[i];
        if (candidate && std::strcmp(candidate, value) == 0) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

} // extern "C"
