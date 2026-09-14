options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module das_tables public

require pulse public

var pulse_tables_registered : bool = false

def private PulseDeclaresOtherRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    var desc = PulseDataTableSchemaDesc(name = "declares_other", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'declares_other' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseDeepRegisterSchema(app: PulseAppId) : bool {
    var shell_columns : array<PulseDataTableColumnDesc>
    shell_columns |> push <| PulseDataTableColumnDesc(name = "radius", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_INT, has_default = true, default_int = int64(2))
    shell_columns |> push <| PulseDataTableColumnDesc(name = "tag", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING, has_default = true, default_string = "x")
    var struct_shell = PulseDataTableStructDesc(name = "shell", columns_count = uint64(length(shell_columns)))
    unsafe {
        struct_shell.p_columns = reinterpret<void?>(addr(shell_columns[0]))
    }
    var inner_columns : array<PulseDataTableColumnDesc>
    inner_columns |> push <| PulseDataTableColumnDesc(name = "power", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT, has_default = true, default_float = double(1.0))
    inner_columns |> push <| PulseDataTableColumnDesc(name = "shell", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT, struct_type = "shell")
    var struct_inner = PulseDataTableStructDesc(name = "inner", columns_count = uint64(length(inner_columns)))
    unsafe {
        struct_inner.p_columns = reinterpret<void?>(addr(inner_columns[0]))
    }
    var structs : array<PulseDataTableStructDesc>
    structs |> push <| struct_inner
    structs |> push <| struct_shell
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "inner", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT, struct_type = "inner")
    var desc = PulseDataTableSchemaDesc(name = "deep", columns_count = uint64(length(columns)), structs_count = uint64(length(structs)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
        desc.p_structs = reinterpret<void?>(addr(structs[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'deep' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseDupRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    var desc = PulseDataTableSchemaDesc(name = "dup", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'dup' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseEnmRegisterSchema(app: PulseAppId) : bool {
    var kind_values : array<string>
    kind_values |> push <| "alpha"
    kind_values |> push <| "beta"
    var enum_kind = PulseDataTableEnumDesc(name = "kind_values", values_count = uint64(length(kind_values)))
    unsafe {
        enum_kind.p_values = reinterpret<void?>(addr(kind_values[0]))
    }
    var enums : array<PulseDataTableEnumDesc>
    enums |> push <| enum_kind
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "kind", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_ENUM, has_default = true, default_string = "alpha", enum_type = "kind_values")
    var desc = PulseDataTableSchemaDesc(name = "enm", columns_count = uint64(length(columns)), enums_count = uint64(length(enums)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
        desc.p_enums = reinterpret<void?>(addr(enums[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'enm' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseExtraRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    var desc = PulseDataTableSchemaDesc(name = "extra", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'extra' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseHeroRegisterSchema(app: PulseAppId) : bool {
    var element : array<string>
    element |> push <| "fire"
    element |> push <| "water"
    element |> push <| "earth"
    var enum_element = PulseDataTableEnumDesc(name = "element", values_count = uint64(length(element)))
    unsafe {
        enum_element.p_values = reinterpret<void?>(addr(element[0]))
    }
    var enums : array<PulseDataTableEnumDesc>
    enums |> push <| enum_element
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "element", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_ENUM, has_default = true, default_string = "water", enum_type = "element")
    columns |> push <| PulseDataTableColumnDesc(name = "power", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT, has_default = true, default_float = double(1.0))
    var desc = PulseDataTableSchemaDesc(name = "hero", columns_count = uint64(length(columns)), enums_count = uint64(length(enums)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
        desc.p_enums = reinterpret<void?>(addr(enums[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'hero' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseItemRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "name", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING, has_default = true, default_string = "unnamed")
    columns |> push <| PulseDataTableColumnDesc(name = "weight", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT, has_default = true, default_float = double(0.5), has_min = true, min_value = double(0.0), has_max = true, max_value = double(5.0))
    var desc = PulseDataTableSchemaDesc(name = "item", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'item' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseNumRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_INT)
    columns |> push <| PulseDataTableColumnDesc(name = "label", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING, has_default = true, default_string = "none")
    var desc = PulseDataTableSchemaDesc(name = "num", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'num' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseRefRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "link", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_REF, has_default = true, default_string = "apple", ref_type = "item")
    var desc = PulseDataTableSchemaDesc(name = "ref", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'ref' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseRefdefaultRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "link", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_REF, has_default = true, default_string = "apple", ref_type = "item")
    var desc = PulseDataTableSchemaDesc(name = "refdefault", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'refdefault' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseReqRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "required", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_INT)
    var desc = PulseDataTableSchemaDesc(name = "req", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'req' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseRngRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "value", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT, has_default = true, default_float = double(1.0), has_min = true, min_value = double(0.0), has_max = true, max_value = double(10.0))
    var desc = PulseDataTableSchemaDesc(name = "rng", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'rng' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseSnakeRegisterSchema(app: PulseAppId) : bool {
    var skill_columns : array<PulseDataTableColumnDesc>
    skill_columns |> push <| PulseDataTableColumnDesc(name = "power", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT, has_default = true, default_float = double(1.0))
    skill_columns |> push <| PulseDataTableColumnDesc(name = "radius", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_INT, has_default = true, default_int = int64(1))
    var struct_skill = PulseDataTableStructDesc(name = "skill", columns_count = uint64(length(skill_columns)))
    unsafe {
        struct_skill.p_columns = reinterpret<void?>(addr(skill_columns[0]))
    }
    var structs : array<PulseDataTableStructDesc>
    structs |> push <| struct_skill
    var price_values : array<string>
    price_values |> push <| "cheap"
    price_values |> push <| "normal"
    price_values |> push <| "pricey"
    var enum_price = PulseDataTableEnumDesc(name = "price_values", values_count = uint64(length(price_values)))
    unsafe {
        enum_price.p_values = reinterpret<void?>(addr(price_values[0]))
    }
    var enums : array<PulseDataTableEnumDesc>
    enums |> push <| enum_price
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "interval", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_FLOAT, has_default = true, default_float = double(1.0), has_min = true, min_value = double(0.1), has_max = true, max_value = double(10.0))
    columns |> push <| PulseDataTableColumnDesc(name = "hp", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_INT, has_default = true, default_int = int64(100), has_min = true, min_value = double(1.0))
    columns |> push <| PulseDataTableColumnDesc(name = "big", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_BOOL, has_default = true, default_bool = false)
    columns |> push <| PulseDataTableColumnDesc(name = "drop", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_REF, ref_type = "item")
    columns |> push <| PulseDataTableColumnDesc(name = "skill", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRUCT, struct_type = "skill")
    columns |> push <| PulseDataTableColumnDesc(name = "price", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_ENUM, enum_type = "price_values")
    var desc = PulseDataTableSchemaDesc(name = "snake", columns_count = uint64(length(columns)), structs_count = uint64(length(structs)), enums_count = uint64(length(enums)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
        desc.p_structs = reinterpret<void?>(addr(structs[0]))
        desc.p_enums = reinterpret<void?>(addr(enums[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'snake' failed to register: {error}\n")
        return false
    }
    return true
}

def private PulseTypRegisterSchema(app: PulseAppId) : bool {
    var columns : array<PulseDataTableColumnDesc>
    columns |> push <| PulseDataTableColumnDesc(name = "id", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_STRING)
    columns |> push <| PulseDataTableColumnDesc(name = "count", column_type = EPulseDataTableColumnType.PULSE_DATA_TABLE_COLUMN_TYPE_INT, has_default = true, default_int = int64(0))
    var desc = PulseDataTableSchemaDesc(name = "typ", columns_count = uint64(length(columns)), key_column = uint(0))
    unsafe {
        desc.p_columns = reinterpret<void?>(addr(columns[0]))
    }
    let error = pulse_data_table_register_schema(app, desc)
    if (error != "") {
        print("Data table schema 'typ' failed to register: {error}\n")
        return false
    }
    return true
}

def PulseTablesRegisterSchemas(app: PulseAppId) {
    if (pulse_tables_registered) {
        return
    }
    var ok = PulseDeclaresOtherRegisterSchema(app)
    ok = ok && PulseDeepRegisterSchema(app)
    ok = ok && PulseDupRegisterSchema(app)
    ok = ok && PulseEnmRegisterSchema(app)
    ok = ok && PulseExtraRegisterSchema(app)
    ok = ok && PulseHeroRegisterSchema(app)
    ok = ok && PulseItemRegisterSchema(app)
    ok = ok && PulseNumRegisterSchema(app)
    ok = ok && PulseRefRegisterSchema(app)
    ok = ok && PulseRefdefaultRegisterSchema(app)
    ok = ok && PulseReqRegisterSchema(app)
    ok = ok && PulseRngRegisterSchema(app)
    ok = ok && PulseSnakeRegisterSchema(app)
    ok = ok && PulseTypRegisterSchema(app)
    pulse_tables_registered = ok
}

def PulseDeclaresOtherLoad(app: PulseAppId; path: string = "declares_other.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "declares_other", path)
}

def PulseDeclaresOtherIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseDeclaresOtherGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseDeclaresOtherRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "declares_other"))
}

def PulseDeclaresOtherRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "declares_other", int64(index))
}

def PulseDeclaresOtherFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "declares_other", key)
}

def PulseDeclaresOtherGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "declares_other", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseDeepLoad(app: PulseAppId; path: string = "deep.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "deep", path)
}

def PulseDeepIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseDeepGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseDeepRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "deep"))
}

def PulseDeepRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "deep", int64(index))
}

def PulseDeepFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "deep", key)
}

def PulseDeepGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "deep", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseDeepGetInnerPower(app: PulseAppId; row: void?) : double {
    return pulse_data_table_read_float(app, "deep", row, int64(1), int64(0), int64(-1), int64(-1))
}

def PulseDeepGetInnerShellRadius(app: PulseAppId; row: void?) : int64 {
    return pulse_data_table_read_int(app, "deep", row, int64(1), int64(1), int64(0), int64(-1))
}

def PulseDeepGetInnerShellTag(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "deep", row, int64(1), int64(1), int64(1), int64(-1))
}

def PulseDupLoad(app: PulseAppId; path: string = "dup.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "dup", path)
}

def PulseDupIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseDupGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseDupRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "dup"))
}

def PulseDupRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "dup", int64(index))
}

def PulseDupFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "dup", key)
}

def PulseDupGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "dup", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseEnmLoad(app: PulseAppId; path: string = "enm.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "enm", path)
}

def PulseEnmIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseEnmGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseEnmRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "enm"))
}

def PulseEnmRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "enm", int64(index))
}

def PulseEnmFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "enm", key)
}

def PulseEnmGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "enm", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseEnmGetKind(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "enm", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseExtraLoad(app: PulseAppId; path: string = "extra.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "extra", path)
}

def PulseExtraIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseExtraGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseExtraRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "extra"))
}

def PulseExtraRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "extra", int64(index))
}

def PulseExtraFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "extra", key)
}

def PulseExtraGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "extra", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseHeroLoad(app: PulseAppId; path: string = "hero.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "hero", path)
}

def PulseHeroIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseHeroGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseHeroRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "hero"))
}

def PulseHeroRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "hero", int64(index))
}

def PulseHeroFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "hero", key)
}

def PulseHeroGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "hero", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseHeroGetElement(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "hero", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseHeroGetPower(app: PulseAppId; row: void?) : double {
    return pulse_data_table_read_float(app, "hero", row, int64(2), int64(-1), int64(-1), int64(-1))
}

def PulseItemLoad(app: PulseAppId; path: string = "item.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "item", path)
}

def PulseItemIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseItemGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseItemRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "item"))
}

def PulseItemRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "item", int64(index))
}

def PulseItemFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "item", key)
}

def PulseItemGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "item", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseItemGetName(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "item", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseItemGetWeight(app: PulseAppId; row: void?) : double {
    return pulse_data_table_read_float(app, "item", row, int64(2), int64(-1), int64(-1), int64(-1))
}

def PulseNumLoad(app: PulseAppId; path: string = "num.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "num", path)
}

def PulseNumIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseNumGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseNumRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "num"))
}

def PulseNumRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "num", int64(index))
}

def PulseNumFindRowInt(app: PulseAppId; key: int) : void? {
    return pulse_data_table_find_row_int(app, "num", int64(key))
}

def PulseNumGetId(app: PulseAppId; row: void?) : int64 {
    return pulse_data_table_read_int(app, "num", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseNumGetLabel(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "num", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseRefLoad(app: PulseAppId; path: string = "ref.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "ref", path)
}

def PulseRefIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseRefGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseRefRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "ref"))
}

def PulseRefRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "ref", int64(index))
}

def PulseRefFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "ref", key)
}

def PulseRefGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "ref", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseRefGetLink(app: PulseAppId; row: void?) : void? {
    return pulse_data_table_read_ref(app, "ref", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseRefdefaultLoad(app: PulseAppId; path: string = "refdefault.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "refdefault", path)
}

def PulseRefdefaultIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseRefdefaultGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseRefdefaultRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "refdefault"))
}

def PulseRefdefaultRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "refdefault", int64(index))
}

def PulseRefdefaultFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "refdefault", key)
}

def PulseRefdefaultGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "refdefault", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseRefdefaultGetLink(app: PulseAppId; row: void?) : void? {
    return pulse_data_table_read_ref(app, "refdefault", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseReqLoad(app: PulseAppId; path: string = "req.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "req", path)
}

def PulseReqIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseReqGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseReqRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "req"))
}

def PulseReqRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "req", int64(index))
}

def PulseReqFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "req", key)
}

def PulseReqGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "req", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseReqGetRequired(app: PulseAppId; row: void?) : int64 {
    return pulse_data_table_read_int(app, "req", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseRngLoad(app: PulseAppId; path: string = "rng.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "rng", path)
}

def PulseRngIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseRngGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseRngRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "rng"))
}

def PulseRngRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "rng", int64(index))
}

def PulseRngFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "rng", key)
}

def PulseRngGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "rng", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseRngGetValue(app: PulseAppId; row: void?) : double {
    return pulse_data_table_read_float(app, "rng", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseSnakeLoad(app: PulseAppId; path: string = "snake.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "snake", path)
}

def PulseSnakeIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseSnakeGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseSnakeRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "snake"))
}

def PulseSnakeRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "snake", int64(index))
}

def PulseSnakeFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "snake", key)
}

def PulseSnakeGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "snake", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseSnakeGetInterval(app: PulseAppId; row: void?) : double {
    return pulse_data_table_read_float(app, "snake", row, int64(1), int64(-1), int64(-1), int64(-1))
}

def PulseSnakeGetHp(app: PulseAppId; row: void?) : int64 {
    return pulse_data_table_read_int(app, "snake", row, int64(2), int64(-1), int64(-1), int64(-1))
}

def PulseSnakeGetBig(app: PulseAppId; row: void?) : bool {
    return pulse_data_table_read_bool(app, "snake", row, int64(3), int64(-1), int64(-1), int64(-1))
}

def PulseSnakeGetDrop(app: PulseAppId; row: void?) : void? {
    return pulse_data_table_read_ref(app, "snake", row, int64(4), int64(-1), int64(-1), int64(-1))
}

def PulseSnakeGetSkillPower(app: PulseAppId; row: void?) : double {
    return pulse_data_table_read_float(app, "snake", row, int64(5), int64(0), int64(-1), int64(-1))
}

def PulseSnakeGetSkillRadius(app: PulseAppId; row: void?) : int64 {
    return pulse_data_table_read_int(app, "snake", row, int64(5), int64(1), int64(-1), int64(-1))
}

def PulseSnakeGetPrice(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "snake", row, int64(6), int64(-1), int64(-1), int64(-1))
}

def PulseTypLoad(app: PulseAppId; path: string = "typ.datatable") : PulseAssetRequest {
    PulseTablesRegisterSchemas(app)
    return pulse_data_table_load(app, "typ", path)
}

def PulseTypIsReady(app: PulseAppId; request: PulseAssetRequest) : bool {
    return pulse_data_table_is_ready(app, request)
}

def PulseTypGetError(app: PulseAppId; request: PulseAssetRequest) : string {
    return pulse_data_table_get_error(app, request)
}

def PulseTypRowCount(app: PulseAppId) : int {
    return int(pulse_data_table_row_count(app, "typ"))
}

def PulseTypRowAt(app: PulseAppId; index: int) : void? {
    return pulse_data_table_row_at(app, "typ", int64(index))
}

def PulseTypFindRow(app: PulseAppId; key: string) : void? {
    return pulse_data_table_find_row(app, "typ", key)
}

def PulseTypGetId(app: PulseAppId; row: void?) : string {
    return pulse_data_table_read_string(app, "typ", row, int64(0), int64(-1), int64(-1), int64(-1))
}

def PulseTypGetCount(app: PulseAppId; row: void?) : int64 {
    return pulse_data_table_read_int(app, "typ", row, int64(1), int64(-1), int64(-1), int64(-1))
}
