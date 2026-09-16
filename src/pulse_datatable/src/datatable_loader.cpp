#include "datatable_internal.h"

#include <cstdio>
#include <cstring>

namespace pulse::datatable {

namespace {

struct LoadState {
    bool dependencies_requested = false;
    std::vector<DependencyRef> dependencies{};
};

EPulseAssetLoaderStatus fail_reason(Registry& registry, EPulseDataTableError error, std::string_view detail, const char** out_error) {
    std::string message(data_table_error_text(error));
    if (!detail.empty()) {
        message.append(": ");
        message.append(detail);
    }
    *out_error = registry.intern_error(message);
    return PULSE_ASSET_LOADER_STATUS_FAILED;
}

uint64_t string_bytes_of(std::string_view text) {
    return text.size() + 1;
}

uint64_t nested_string_bytes(const PulseDataTableSchemaDesc* schema, const PulseDataTableStructDesc* desc, const PulseDatalist* node) {
    uint64_t total = 0;
    for (size_t i = 0; i < desc->columns_count; ++i) {
        const PulseDataTableColumnDesc& column = desc->p_columns[i];
        const PulseDatalist* value = pulse_datalist_value(node, column.name);
        if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_STRING || column.type == PULSE_DATA_TABLE_COLUMN_TYPE_ENUM) {
            std::string_view text{};
            if (value) {
                if (decode_string(value, nullptr, text)) {
                    total += string_bytes_of(text);
                }
            } else if (column.has_default && column.default_string) {
                total += string_bytes_of(std::string_view(column.default_string));
            }
        } else if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT && value) {
            const PulseDataTableStructDesc* nested = find_struct(schema, column.struct_type ? column.struct_type : "");
            if (nested) {
                total += nested_string_bytes(schema, nested, value);
            }
        }
    }
    return total;
}

uint64_t row_string_bytes(const PulseDataTableSchemaDesc* schema, const PulseDatalist* row) {
    uint64_t total = 0;
    for (size_t c = 0; c < schema->columns_count; ++c) {
        const PulseDataTableColumnDesc& column = schema->p_columns[c];
        const PulseDatalist* value = pulse_datalist_value(row, column.name);
        if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_STRING || column.type == PULSE_DATA_TABLE_COLUMN_TYPE_ENUM) {
            std::string_view text{};
            if (value) {
                if (decode_string(value, nullptr, text)) {
                    total += string_bytes_of(text);
                }
            } else if (column.has_default && column.default_string) {
                total += string_bytes_of(std::string_view(column.default_string));
            }
        } else if (column.type == PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT && value) {
            const PulseDataTableStructDesc* desc = find_struct(schema, column.struct_type ? column.struct_type : "");
            if (desc) {
                total += nested_string_bytes(schema, desc, value);
            }
        }
    }
    return total;
}

std::string schema_name_from_path(const char* path) {
    std::string text(path ? path : "");
    size_t slash = text.find_last_of("/\\");
    std::string file = slash == std::string::npos ? text : text.substr(slash + 1);
    size_t dot = file.find_last_of('.');
    return dot == std::string::npos ? file : file.substr(0, dot);
}

std::string sibling_path(const char* path, std::string_view schema) {
    std::string text(path ? path : "");
    size_t slash = text.find_last_of("/\\");
    std::string directory = slash == std::string::npos ? std::string() : text.substr(0, slash + 1);
    directory.append(schema);
    directory.append(".datatable");
    return directory;
}

PulseAssetHandle request_handle(PulseAssetRequest request) {
    PulseAssetHandle handle{};
    handle.type_id = request.type_id;
    handle.index = request.index;
    handle.generation = request.generation;
    return handle;
}

PulseAssetRequest request_of(PulseAssetHandle handle) {
    PulseAssetRequest request{};
    request.type_id = handle.type_id;
    request.index = handle.index;
    request.generation = handle.generation;
    return request;
}

PulseAssetRequest request_path(PulseAssetSystemId asset_system, const std::string& path) {
    PulseAssetHandle loaded = pulse_asset_system_find_loaded(asset_system, PULSE_TYPE_DATA_TABLE, path.c_str());
    if (pulse_asset_handle_is_valid(loaded)) {
        return request_of(loaded);
    }
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = PULSE_TYPE_DATA_TABLE;
    desc.path = path.c_str();
    return pulse_asset_system_load(asset_system, &desc);
}

} // namespace

EPulseAssetLoaderStatus step_data_table_load(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    auto* load_state = static_cast<LoadState*>(state);
    auto* registry = static_cast<Registry*>(ctx->user_data);
    if (!registry) {
        *out_error = "data table loader: registry is not available";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    std::string path_schema = schema_name_from_path(ctx->path);
    const PulseDataTableSchemaDesc* schema = registry->find_schema(path_schema);
    if (!schema) {
        return fail_reason(*registry, PULSE_DATA_TABLE_ERROR_NO_SCHEMA_FOR_TYPE, path_schema, out_error);
    }

    if (!load_state->dependencies_requested) {
        for (size_t c = 0; c < schema->columns_count; ++c) {
            const PulseDataTableColumnDesc& column = schema->p_columns[c];
            if (column.type != PULSE_DATA_TABLE_COLUMN_TYPE_REF || !column.ref_type) {
                continue;
            }
            if (!registry->find_schema(column.ref_type)) {
                return fail_reason(*registry, PULSE_DATA_TABLE_ERROR_SCHEMA_IS_NOT_REGISTERED, column.ref_type, out_error);
            }
            std::string target_path = sibling_path(ctx->path, column.ref_type);
            PulseAssetRequest dependency_request = request_path(ctx->asset_system, target_path);
            if (!pulse_asset_request_is_valid(dependency_request)) {
                return fail_reason(*registry, PULSE_DATA_TABLE_ERROR_SCHEMA_IS_NOT_REGISTERED, column.ref_type, out_error);
            }
            DependencyRef entry{};
            entry.schema = column.ref_type;
            entry.handle = request_handle(dependency_request);
            load_state->dependencies.push_back(std::move(entry));
        }
        load_state->dependencies_requested = true;
    }

    for (const DependencyRef& dependency : load_state->dependencies) {
        if (!pulse_asset_handle_is_valid(dependency.handle)) {
            return fail_reason(*registry, PULSE_DATA_TABLE_ERROR_MISSING_REFERENCE, dependency.schema, out_error);
        }
        EPulseAssetState dependency_state = pulse_asset_system_get_state(ctx->asset_system, request_of(dependency.handle));
        if (dependency_state == PULSE_ASSET_STATE_FAILED || dependency_state == PULSE_ASSET_STATE_EMPTY || dependency_state == PULSE_ASSET_STATE_PENDING_DELETE) {
            return fail_reason(*registry, PULSE_DATA_TABLE_ERROR_MISSING_REFERENCE, dependency.schema, out_error);
        }
        if (dependency_state != PULSE_ASSET_STATE_LOADED) {
            return PULSE_ASSET_LOADER_STATUS_PENDING;
        }
    }

    PulseDatalist* root = pulse_datalist_create_from_text(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
    if (!root) {
        return fail_reason(*registry, PULSE_DATA_TABLE_ERROR_PARSE_FAILED, pulse_datalist_last_error() ? pulse_datalist_last_error() : "", out_error);
    }

    EPulseAssetLoaderStatus result = PULSE_ASSET_LOADER_STATUS_FAILED;
    Table* table = nullptr;
    std::string detail{};
    char buffer[384];

    do {
        if (pulse_datalist_get_type(root, nullptr) != PULSE_DATALIST_TYPE_MAP) {
            result = fail_reason(*registry, PULSE_DATA_TABLE_ERROR_ROOT_IS_NOT_TABLE, "", out_error);
            break;
        }
        const char* declared = pulse_datalist_get_string(root, "schema", nullptr);
        if (!declared) {
            result = fail_reason(*registry, PULSE_DATA_TABLE_ERROR_SCHEMA_FIELD_MISSING, "", out_error);
            break;
        }
        if (std::strcmp(declared, schema->name) != 0) {
            result = fail_reason(*registry, PULSE_DATA_TABLE_ERROR_SCHEMA_IS_NOT_REGISTERED, declared, out_error);
            break;
        }
        PulseDatalist* rows = pulse_datalist_get_obj(root, "rows");
        if (!rows || pulse_datalist_get_type(rows, nullptr) != PULSE_DATALIST_TYPE_LIST) {
            result = fail_reason(*registry, PULSE_DATA_TABLE_ERROR_ROWS_FIELD_MISSING, "", out_error);
            break;
        }

        size_t row_count = pulse_datalist_count(rows);
        bool invalid = false;
        for (size_t r = 0; r < row_count && !invalid; ++r) {
            PulseDatalist* row = pulse_datalist_get(rows, r);
            if (!row || pulse_datalist_get_type(row, nullptr) != PULSE_DATALIST_TYPE_MAP) {
                std::snprintf(buffer, sizeof(buffer), "row %zu is not a table", r);
                detail.assign(buffer);
                invalid = true;
                break;
            }
            for (size_t i = 0; i < pulse_datalist_object_count(row); ++i) {
                const char* key = pulse_datalist_object_key(row, i);
                if (!key || find_column(schema, key)) {
                    continue;
                }
                std::snprintf(buffer, sizeof(buffer), "line %d: row declares unknown column '%s'", pulse_datalist_line(pulse_datalist_object_value(row, i)), key);
                detail.assign(buffer);
                invalid = true;
                break;
            }
            for (size_t c = 0; c < schema->columns_count && !invalid; ++c) {
                const PulseDataTableColumnDesc& column = schema->p_columns[c];
                PulseDatalist* value = pulse_datalist_value(row, column.name);
                if (!value) {
                    continue;
                }
                EPulseDataTableError error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
                const char* message = nullptr;
                if (!check_column(schema, column, value, error, message)) {
                    std::snprintf(buffer, sizeof(buffer), "line %d: column '%s': %s", pulse_datalist_line(value), column.name, message ? message : data_table_error_text(error));
                    detail.assign(buffer);
                    invalid = true;
                    break;
                }
            }
        }
        if (invalid) {
            result = fail_reason(*registry, PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH, detail, out_error);
            break;
        }

        uint64_t vault_bytes = 0;
        for (size_t r = 0; r < row_count; ++r) {
            vault_bytes += row_string_bytes(schema, pulse_datalist_get(rows, r));
        }

        table = registry->create_table(request_handle(ctx->request), schema);
        table->name = path_schema;
        table->source_text.assign(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
        table->row_count = static_cast<uint32_t>(row_count);
        table->vault.reserve(static_cast<size_t>(vault_bytes));
        uint32_t row_size = table_row_size(schema);
        table->rows.resize(static_cast<size_t>(row_size) * table->row_count);

        bool fill_failed = false;
        for (uint32_t r = 0; r < table->row_count; ++r) {
            PulseDatalist* row_node = pulse_datalist_get(rows, r);
            void* destination = table->rows.data() + static_cast<size_t>(r) * row_size;
            EPulseDataTableError fill_error = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            const char* fill_message = nullptr;
            int32_t error_line = 0;
            FillContext fill{};
            fill.owner = table;
            fill.row = r;
            fill.dependencies = &load_state->dependencies;
            bool filled = schema->fill_row ? schema->fill_row(&fill, &table->vault, row_node, destination, &error_line, &fill_error, &fill_message) : generic_fill_row(schema, &fill, &table->vault, row_node, destination, &error_line, &fill_error, &fill_message);
            if (!filled) {
                std::string text = fill_message ? fill_message : data_table_error_text(fill_error);
                std::snprintf(buffer, sizeof(buffer), "line %d: %s", error_line > 0 ? error_line : pulse_datalist_line(row_node), text.c_str());
                result = fail_reason(*registry, fill_error, buffer, out_error);
                fill_failed = true;
                break;
            }
        }
        if (fill_failed) {
            registry->destroy_table(table);
            table = nullptr;
            break;
        }

        const PulseDataTableColumnDesc& key_column = schema->p_columns[schema->key_column];
        bool key_failed = false;
        for (uint32_t r = 0; r < table->row_count; ++r) {
            const void* row_data = table->rows.data() + static_cast<size_t>(r) * row_size;
            RowKey key = read_key(*table, row_data);
            if (key.is_string) {
                auto inserted = table->string_index.emplace(std::string(key.text), r);
                if (!inserted.second) {
                    uint32_t first = inserted.first->second;
                    std::snprintf(buffer, sizeof(buffer), "column '%s' value '%s': row %u (line %d) and row %u (line %d)",
                        key_column.name, std::string(key.text).c_str(), first, pulse_datalist_line(pulse_datalist_get(rows, first)), r, pulse_datalist_line(pulse_datalist_get(rows, r)));
                    result = fail_reason(*registry, PULSE_DATA_TABLE_ERROR_DUPLICATE_KEY, buffer, out_error);
                    key_failed = true;
                    break;
                }
            } else {
                auto inserted = table->int_index.emplace(key.number, r);
                if (!inserted.second) {
                    uint32_t first = inserted.first->second;
                    std::snprintf(buffer, sizeof(buffer), "column '%s' value %lld: row %u (line %d) and row %u (line %d)",
                        key_column.name, static_cast<long long>(key.number), first, pulse_datalist_line(pulse_datalist_get(rows, first)), r, pulse_datalist_line(pulse_datalist_get(rows, r)));
                    result = fail_reason(*registry, PULSE_DATA_TABLE_ERROR_DUPLICATE_KEY, buffer, out_error);
                    key_failed = true;
                    break;
                }
            }
        }
        if (key_failed) {
            registry->destroy_table(table);
            table = nullptr;
            break;
        }

        auto* slot = static_cast<TableSlot*>(ctx->out_asset);
        registry->bind_slot(request_handle(ctx->request), slot);
        slot->rows = table->rows.data();
        slot->row_count = table->row_count;
        result = PULSE_ASSET_LOADER_STATUS_DONE;
    } while (false);

    if (result == PULSE_ASSET_LOADER_STATUS_FAILED && !*out_error) {
        *out_error = registry->intern_error("data table load failed");
    }

    pulse_datalist_release(root);
    return result;
}

void dtor_data_table_load(void* state, const PulseAssetLoadTask* ctx) {
    (void)ctx;
    new (state) LoadState{};
}

void destroy_data_table_asset(void* ptr, void* user_data) {
    auto* registry = static_cast<Registry*>(user_data);
    auto* slot = static_cast<TableSlot*>(ptr);
    if (registry && slot) {
        registry->release_slot(slot);
    }
}

EPulseResult register_data_table_type(PulseAssetSystemId asset_system, Registry* registry) {
    PulseAssetTypeDesc desc{};
    desc.struct_size = sizeof(PulseAssetTypeDesc);
    desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    desc.type_id = PULSE_TYPE_DATA_TABLE;
    desc.size = sizeof(TableSlot);
    desc.align = alignof(TableSlot);
    desc.destroy = destroy_data_table_asset;
    desc.user_data = registry;
    return pulse_asset_system_register_type(asset_system, &desc);
}

EPulseResult register_data_table_loader(PulseAssetSystemId asset_system, Registry* registry) {
    PulseAssetLoaderDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoaderDesc);
    desc.version = PULSE_ASSET_LOADER_DESC_VERSION;
    desc.type_id = PULSE_TYPE_DATA_TABLE;
    desc.extensions = "datatable";
    desc.ctor = nullptr;
    desc.dtor = dtor_data_table_load;
    desc.step = step_data_table_load;
    desc.loader_size = sizeof(LoadState);
    desc.loader_align = alignof(LoadState);
    desc.settings_size = 0;
    desc.settings_align = 0;
    desc.user_data = registry;
    return pulse_asset_system_register_loader(asset_system, &desc);
}

} // namespace pulse::datatable
