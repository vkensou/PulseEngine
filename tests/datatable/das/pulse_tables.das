options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module pulse_tables public

require pulse public

def PulseDeclaresOtherLoad(app: PulseAppId; path: string = "declares_other.datatable") : PulseAssetRequest {
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

def PulseItemLoad(app: PulseAppId; path: string = "item.datatable") : PulseAssetRequest {
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

def PulseReqLoad(app: PulseAppId; path: string = "req.datatable") : PulseAssetRequest {
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

