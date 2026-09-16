#include "tables_generated.h"

#include "pulse_datatable_schema.h"

#include <array>

namespace
{

constexpr const PulseDataTableSchemaDesc* pulse_table_snake_config_schema_ptr();

const PulseDataTableColumnDesc pulse_table_snake_config_columns_0_0 = pulse::datatable::ColumnDescBuilder{}.name("id").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<std::string_view>>::value).offset(0).build();
const PulseDataTableColumnDesc pulse_table_snake_config_columns_0_1 = pulse::datatable::ColumnDescBuilder{}.name("move_interval").column_type(pulse::datatable::ColumnType<pulse::datatable::Bare<double>>::value).offset(16).min(0.050000000000000003).max(10.0).default_float(1.0).build();
const std::array<PulseDataTableColumnDesc, 2> pulse_table_snake_config_columns_0 = { pulse_table_snake_config_columns_0_0, pulse_table_snake_config_columns_0_1 };

bool pulse_table_snake_config_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error);

constexpr const PulseDataTableSchemaDesc* pulse_table_snake_config_schema_ptr();
const PulseDataTableSchemaDesc pulse_table_snake_config_schema{
    sizeof(PulseDataTableSchemaDesc),
    PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
    "snake_config",
    pulse_table_snake_config_columns_0.data(),
    2u,
    nullptr,
    0u,
    nullptr,
    0u,
    0u,
    false,
    pulse_table_snake_config_fill_0
};

constexpr const PulseDataTableSchemaDesc* pulse_table_snake_config_schema_ptr() { return &pulse_table_snake_config_schema; }

bool pulse_table_snake_config_fill_0(const void* context, void* vault_data, const PulseDatalist* node, void* out, int32_t* error_line, EPulseDataTableError* error_code, const char** out_error) {
    const PulseDataTableSchemaDesc* owner = pulse_table_snake_config_schema_ptr();
    auto* vault = static_cast<pulse::datatable::StringVault*>(vault_data);
    (void)vault;
    (void)context;
    auto* row = static_cast<::pulse_tables::PulseSnakeConfigRow*>(out);
    {
        const PulseDatalist* value = pulse_datalist_value(node, "id");
        if (!value) {
            *error_line = pulse_datalist_line(node);
            *error_code = PULSE_DATA_TABLE_ERROR_MISSING_COLUMN;
            *out_error = "column 'id' is missing";
            return false;
        }
        if (value && pulse_datalist_get_type(value, nullptr) != PULSE_DATALIST_TYPE_STRING) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
            *out_error = "column 'id' expects a string";
            return false;
        }
        std::string_view decoded = value ? vault->append(std::string_view(pulse_datalist_get_string(value, nullptr, ""))) : std::string_view("");
        pulse_data_table_field_set_string(row, &pulse_table_snake_config_columns_0[0], &decoded, out_error);
    }
    {
        const PulseDatalist* value = pulse_datalist_value(node, "move_interval");
        double decoded = value ? pulse_datalist_get_double(value, nullptr, 0.0) : 1.0;
        if (value) {
            EPulseDatalistType value_type = pulse_datalist_get_type(value, nullptr);
            if (value_type != PULSE_DATALIST_TYPE_DOUBLE && value_type != PULSE_DATALIST_TYPE_INT) {
                *error_line = pulse_datalist_line(value);
                *error_code = PULSE_DATA_TABLE_ERROR_TYPE_MISMATCH;
                *out_error = "column 'move_interval' expects a float";
                return false;
            }
        }
        if (!pulse_data_table_field_set_float(row, &pulse_table_snake_config_columns_0[1], decoded, out_error)) {
            *error_line = pulse_datalist_line(value);
            *error_code = PULSE_DATA_TABLE_ERROR_OUT_OF_RANGE;
            return false;
        }
    }
    (void)error_line;
    (void)node;
    return true;
}


} // namespace

namespace pulse_tables
{

const char* pulse_table_snake_config_path = nullptr;

const char* PulseSnakeConfigRowTable::DefaultPath() {
    return "snake_config.datatable";
}

PulseAssetRequest PulseSnakeConfigRowTable::Load(PulseAppId app, const char* path) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return pulse_asset_request_make_invalid();
    }
    if (path) {
        pulse_table_snake_config_path = path;
    }
    const char* resolved = pulse_table_snake_config_path ? pulse_table_snake_config_path : DefaultPath();
    pulse_table_snake_config_path = resolved;
    return pulse_data_table_system_load(system, "snake_config", resolved);
}

bool PulseSnakeConfigRowTable::IsReady(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system && pulse_data_table_system_is_ready(system, Load(app, nullptr));
}

const char* PulseSnakeConfigRowTable::GetError(PulseAppId app) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    return system ? pulse_data_table_system_get_error(system, Load(app, nullptr)) : nullptr;
}

const PulseSnakeConfigRow* PulseSnakeConfigRowTable::Rows(PulseAppId app, uint32_t& out_count) {
    out_count = 0;
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    if (!table) {
        return nullptr;
    }
    return static_cast<const PulseSnakeConfigRow*>(pulse_data_table_rows(table, &out_count));
}

const PulseSnakeConfigRow* PulseSnakeConfigRowTable::GetRow(PulseAppId app, const char* key) {
    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system) {
        return nullptr;
    }
    PulseDataTableId table = pulse_data_table_system_get(system, Load(app, nullptr));
    return table ? static_cast<const PulseSnakeConfigRow*>(pulse_data_table_find_row(table, key)) : nullptr;
}

EPulseResult RegisterSchemas(PulseDataTableSystemId system) {
    if (!system) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    {
        EPulseResult result = pulse_data_table_system_register_schema(system, &pulse_table_snake_config_schema, nullptr);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    return PULSE_RESULT_OK;
}
} // namespace pulse_tables
