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
    let drop = PulseSnakeGetDrop(app, green)
    let seven = PulseNumFindRowInt(app, 7)
    let deep_a = PulseDeepFindRow(app, "a")
    let deep_b = PulseDeepFindRow(app, "b")
    let mage = PulseHeroFindRow(app, "mage")

    var text_code = 0
    if (PulseSnakeGetId(app, green) != "green") {
        text_code = 1
    } elif (PulseSnakeGetId(app, blue) != "blue") {
        text_code = 2
    } elif (PulseItemGetId(app, drop) != "apple") {
        text_code = 3
    } elif (PulseSnakeGetPrice(app, green) != "cheap") {
        text_code = 4
    } elif (PulseNumGetLabel(app, seven) != "seven") {
        text_code = 5
    } elif (PulseSnakeGetId(app, PulseSnakeRowAt(app, 0)) != "green") {
        text_code = 6
    } elif (PulseDeepGetInnerShellTag(app, deep_a) != "deep") {
        text_code = 7
    } elif (PulseDeepGetInnerShellTag(app, deep_b) != "x") {
        text_code = 8
    } elif (PulseItemGetId(app, PulseRefdefaultGetLink(app, PulseRefdefaultFindRow(app, "solo"))) != "apple") {
        text_code = 9
    } elif (PulseHeroGetElement(app, mage) != "fire") {
        text_code = 10
    } elif (PulseHeroGetElement(app, PulseHeroFindRow(app, "knight")) != "water") {
        text_code = 11
    }

    var error_code = 0
    if (failed_error == "") {
        error_code = 1
    }

    let probe = DasTableProbe(
        value = int(PulseSnakeGetHp(app, green)) * 1000 + int(PulseSnakeGetSkillRadius(app, green)) * 10 + PulseNumRowCount(app) + int(PulseDeepGetInnerShellRadius(app, deep_b)),
        power = PulseSnakeGetInterval(app, green) + PulseSnakeGetSkillPower(app, green) + PulseSnakeGetInterval(app, blue) + PulseDeepGetInnerPower(app, deep_a),
        flag = PulseSnakeGetBig(app, green) && !PulseSnakeGetBig(app, blue),
        text_code = text_code,
        error_code = error_code
    )
    let ent = cmd.entity()
    cmd |> set_entity_component(ent, probe)
}
