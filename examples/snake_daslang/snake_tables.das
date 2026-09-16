options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module snake_tables public

require pulse public

struct PulseSnakeConfigRow {
    id : string
    id_length : uint64
    move_interval : double
}

var pulse_tables_registered : bool = false

def private PulseSnakeConfigRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "move_interval", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT, has_default = true, default_float = double(1.0), has_min = true, min_value = double(0.05), has_max = true, max_value = double(10.0))
    var desc = PulseDataTableSchemaDesc(name = "snake_config", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'snake_config' failed to register: {error}\n")
        return false
    }
    return true
}

def PulseTablesRegisterSchemas(app: PulseAppId) {
    static_assert(typeinfo sizeof(type<PulseSnakeConfigRow>) == 24)
    if (pulse_tables_registered) {
        return
    }
    var ok = PulseSnakeConfigRegisterSchema(app)
    pulse_tables_registered = ok
}

def PulseSnakeConfigLoad(app: PulseAppId; path: string = "snake_config.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "snake_config", path)
}

def PulseSnakeConfigIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseSnakeConfigGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseSnakeConfigRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "snake_config"))
}

def PulseSnakeConfigRowAt(app: PulseAppId; index: int) : PulseSnakeConfigRow? {
    unsafe {
        return reinterpret<PulseSnakeConfigRow?>(pulse_data_table_row_at(app, "snake_config", int64(index)))
    }
}

def PulseSnakeConfigFindRow(app: PulseAppId; key: string) : PulseSnakeConfigRow? {
    unsafe {
        return reinterpret<PulseSnakeConfigRow?>(pulse_data_table_find_row(app, "snake_config", key))
    }
}
