options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

require pulse/flecs_boost public
require pulse/pulse_components public
require pulse public
require <game>_tables public
require math

[state_machine(init="UnInitialized")]
enum <game>GameState {
    UnInitialized
    Loading
    LoadFailed
    Gaming
    GameOver
}

struct <game>Prefabs {
    rect : PulsePrefabHandle
}

struct <game>Assets {
    rect : PulsePrefabRequest
    config : PulseAssetRequest
}

struct <game>Config {
    moveInterval : float
}

struct <game>Move {
    interval : float
    lastTime : float
}

struct <game>Block {
    position : HMM_Vec3
}

struct Field {
    centerX : float
    top : float
    bottom : float
}

struct GameOverEvent {}

struct RestartEvent {}

let HMM_Q_Identity = HMM_Quat(X = 0.0, Y = 0.0, Z = 0.0, W = 1.0)
let HMM_V3_One = HMM_V3(1.0, 1.0, 1.0)

def instantiatePrefab(cmd : CommandBuffer, prefab : PulsePrefabHandle, position : HMM_Vec3, scale : HMM_Vec3) {
    let ent = cmd |> instantiate(prefab)
    cmd |> set_entity_component(ent, PulseLocalTransform(translation = position, rotation = HMM_Q_Identity, scale = scale))
    return ent
}

def createBlock(cmd : CommandBuffer, prefabs : <game>Prefabs, position : HMM_Vec3, moveInterval : float) {
    let entity = instantiatePrefab(cmd, prefabs.rect, position, HMM_V3_One)
    cmd |> set_entity_component(entity, <game>Block(position = position))
    cmd |> set_entity_component(entity, <game>Move(interval = moveInterval, lastTime = 0.0))
}

def spawnPosition(field : Field) {
    return HMM_V3(field.centerX, field.top - 1.0, 0.0)
}

[system(phase="update", state="UnInitialized|Loading")]
def <game>LoadResourcesSystem(app : PulseAppId, assets : Res<<game>Assets>, state : SystemStateMachine<<game>GameState>, cmd : CommandBuffer, primaryWindowQuery : Query<PulseWindow const, PulsePrimaryWindow const>) {
    if (state.is_state(<game>GameState.UnInitialized)) {
        assets.get().rect = pulse_load_prefab(app, "assets/rect.prefab")
        assets.get().config = Pulse<game>ConfigLoad(app, "assets/<game>_config.datatable")
        state.to(<game>GameState.Loading)
        return
    }

    let request = assets.get().rect
    let asset_system = pulse_get_asset_system(app)
    let assetRequest = pulse_prefab_request_to_asset_request(request)
    if (pulse_asset_system_get_state(asset_system, assetRequest) == EPulseAssetState.PULSE_ASSET_STATE_FAILED) {
        print("Prefab load failed: assets/rect.prefab: {pulse_asset_system_get_error(asset_system, assetRequest)}\n")
        state.to(<game>GameState.LoadFailed)
        return
    }

    let config_error = Pulse<game>ConfigGetError(app, assets.get().config)
    if (config_error != "") {
        print("Config load failed: {config_error}\n")
        state.to(<game>GameState.LoadFailed)
        return
    }

    if (!pulse_prefab_is_ready(app, request)) {
        return
    }

    if (!Pulse<game>ConfigIsReady(app, assets.get().config)) {
        return
    }

    let configRow = Pulse<game>ConfigFindRow(app, "default")
    if (configRow == null) {
        print("Config row 'default' is missing\n")
        state.to(<game>GameState.LoadFailed)
        return
    }
    let moveInterval = float(configRow.move_interval)
    cmd |> set_singleton(<game>Config(moveInterval = moveInterval))

    let prefabs = <game>Prefabs(rect = pulse_prefab_get_handle(app, request))
    cmd |> set_singleton(prefabs)

    let (windowEntity, _, _) = primaryWindowQuery |> first()
    let centerX = 0.5
    let centerY = 0.5
    let cameraZ = -38.0
    let cameraFov = 45.0
    let halfHeight = -cameraZ * tan(cameraFov * 0.5 * PI / 180.0)
    let field = Field(centerX = centerX, top = centerY + halfHeight, bottom = centerY - halfHeight)
    cmd |> set_singleton(field)

    createBlock(cmd, prefabs, spawnPosition(field), moveInterval)

    let camera = cmd.entity()
    cmd |> set_entity_component(camera, PulseLocalTransform(translation = HMM_V3(centerX, centerY, cameraZ), rotation = HMM_Q_Identity, scale = HMM_V3_One))
    cmd |> set_entity_component(camera, PulseCamera(window_entity = windowEntity, fov = cameraFov, near_plane = 0.1, far_plane = 1000.0))

    state.to(<game>GameState.Gaming)
    print("<game> resources ready.\n")
}

[system(phase="update", state="Gaming")]
def <game>MoveSystem(timer : Res<PulseTimer const>, var move : <game>Move, var rect : <game>Block, entity : Entity, cmd : CommandBuffer, fieldQuery : SingletonQuery<Field const>, gameOverEvent : EventWriter<GameOverEvent>) {
    move.lastTime += timer.get().delta_time
    if (move.lastTime < move.interval) {
        return
    }

    while (move.lastTime > move.interval) {
        move.lastTime -= move.interval
    }

    rect.position = HMM_V3(rect.position.X, rect.position.Y - 1.0, rect.position.Z)
    cmd |> set_entity_component(entity, PulseLocalTransform(translation = rect.position, rotation = HMM_Q_Identity, scale = HMM_V3_One))

    if (rect.position.Y <= fieldQuery.get().bottom) {
        gameOverEvent |> broadcast(GameOverEvent())
    }
}

[system(state="Gaming")]
def <game>GameOverSystem(gameOverEvent : EventReader<GameOverEvent>, cmd : CommandBuffer, state : SystemStateMachine<<game>GameState>, blockQuery : Query<<game>Block const>) {
    blockQuery |> for_each() $(entity, rect) {
        cmd |> destruct(entity)
    }
    state.to(<game>GameState.GameOver)
}

[system(phase="imgui", state="Gaming|GameOver")]
def <game>UISystem(state : SystemStateMachine<<game>GameState>, restartEvent : EventWriter<RestartEvent>) {
    if (state.is_state(<game>GameState.GameOver)) {
        if (Button("Restart")) {
            restartEvent |> broadcast(RestartEvent())
        }
    }
}

[system(phase="imgui")]
def <game>FpsUISystem(timer : Res<PulseTimer const>) {
    Text("FPS: {timer.get().fps}")
}

[system(state="GameOver")]
def <game>RestartSystem(restartEvent : EventReader<RestartEvent>, cmd : CommandBuffer, state : SystemStateMachine<<game>GameState>, prefabs : SingletonQuery<<game>Prefabs const>, fieldQuery : SingletonQuery<Field const>, configQuery : SingletonQuery<<game>Config const>) {
    createBlock(cmd, prefabs.get(), spawnPosition(fieldQuery.get()), configQuery.get().moveInterval)
    state.to(<game>GameState.Gaming)
}

[export]
def importModule(var ctx : ModuleContext) {
    state_machine_init(ctx.world, ctx.updatePipeline, type<<game>GameState>, <game>GameState.UnInitialized)
    register_resource(ctx.world, type<<game>Assets>)

    ctx |> register_system(@@<game>LoadResourcesSystem)
    ctx |> register_system(@@<game>MoveSystem)
    ctx |> register_system(@@<game>GameOverSystem)
    ctx |> register_system(@@<game>UISystem)
    ctx |> register_system(@@<game>FpsUISystem)
    ctx |> register_system(@@<game>RestartSystem)
}
