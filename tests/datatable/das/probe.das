options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

require pulse/flecs_boost public
require das_tables public

struct DasTableProbe {
    value : int
    power : double
    flag : bool
    text_code : int
    error_code : int
}

var probe_sent = false
var probe_frames = 0

[export]
def importModule(var ctx : ModuleContext) {
    ctx |> register_system(@@probeTableSystem)
}

[system(phase="update")]
def probeTableSystem(app : PulseAppId, cmd : CommandBuffer) {
    if (probe_sent) {
        return
    }

    probe_frames++

    let snake_request = PulseSnakeLoad(app)
    let num_request = PulseNumLoad(app)
    let deep_request = PulseDeepLoad(app)
    let refdefault_request = PulseRefdefaultLoad(app)
    let hero_request = PulseHeroLoad(app)

    if (!PulseSnakeIsReady(app, snake_request) || !PulseNumIsReady(app, num_request) || !PulseDeepIsReady(app, deep_request) || !PulseRefdefaultIsReady(app, refdefault_request) || !PulseHeroIsReady(app, hero_request)) {
        if (probe_frames < 300) {
            return
        }
        probe_sent = true
        let failed = cmd.entity()
        cmd |> set_entity_component(failed, DasTableProbe(value = -1, power = double(0.0), flag = false, text_code = 0, error_code = 2))
        return
    }

    let extra_request = PulseSnakeLoad(app, "extra.datatable")
    let failed_error = PulseSnakeGetError(app, extra_request)
    if (failed_error == "") {
        if (probe_frames < 300) {
            return
        }
    }

    probe_sent = true

    let green = PulseSnakeFindRow(app, "green")
    let blue = PulseSnakeRowAt(app, 1)
    let seven = PulseNumFindRowInt(app, 7)
    let deep_a = PulseDeepFindRow(app, "a")
    let deep_b = PulseDeepFindRow(app, "b")
    let mage = PulseHeroFindRow(app, "mage")

    var text_code = 0
    if (green == null || blue == null || seven == null || deep_a == null || deep_b == null || mage == null) {
        text_code = 1
    } elif (PulseSnakeRowAt(app, 99) != null) {
        text_code = 2
    } elif (PulseSnakeFindRow(app, "purple") != null) {
        text_code = 3
    } elif (green.id != "green") {
        text_code = 4
    } elif (blue.id != "blue") {
        text_code = 5
    } elif (PulseNumRowCount(app) != 2) {
        text_code = 6
    } elif (seven.title != "seven") {
        text_code = 7
    } elif (green.drop == null) {
        text_code = 8
    } elif (green.drop.id != "apple") {
        text_code = 9
    } elif (green.price != "cheap") {
        text_code = 10
    } elif (green.skill.power != double(3.5) || green.skill.radius != int64(2)) {
        text_code = 11
    } elif (PulseDeepRowAt(app, 0).inner.shell.tag != "deep") {
        text_code = 12
    } elif (deep_b.inner.shell.tag != "x") {
        text_code = 13
    } elif (PulseRefdefaultFindRow(app, "solo").link.id != "apple") {
        text_code = 14
    } elif (mage.element != "fire") {
        text_code = 15
    } elif (PulseHeroFindRow(app, "knight").element != "water") {
        text_code = 16
    } elif (green.id_length != uint64(5)) {
        text_code = 17
    }

    var error_code = 0
    if (failed_error == "") {
        error_code = 1
    }

    let probe = DasTableProbe(
        value = int(green.hp) * 1000 + int(green.skill.radius) * 10 + PulseNumRowCount(app) + int(deep_b.inner.shell.radius),
        power = green.interval + green.skill.power + blue.interval + deep_a.inner.power,
        flag = green.big && !blue.big,
        text_code = text_code,
        error_code = error_code
    )
    let ent = cmd.entity()
    cmd |> set_entity_component(ent, probe)
}
