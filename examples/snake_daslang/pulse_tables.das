options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module pulse_tables public

require pulse public

def PulseSnakeConfigLoad(app: PulseAppId; path: string = "snake_config.datatable") : PulseAssetRequest {
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

def PulseSnakeConfigRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "snake_config", int64(index))
}

def PulseSnakeConfigFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "snake_config", key)
}

def PulseSnakeConfigGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "snake_config", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseSnakeConfigGetMoveInterval(app: PulseAppId; row: void?) : double {
    return pulse_data_table_read_float(app, "snake_config", row, int64(1), int64(-1), int64(-1), int64(-1))
}

