options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

require pulse/flecs_boost public
require pulse/pulse_components public
require pulse public
require cgpu
require daslib/random
require daslib/option

// ============================================================
// Game state machine (PULSE_ECS_STATE_MACHINE)
// ============================================================

[state_machine(init="UnInitialized")]
enum SnakeGameState {
    UnInitialized
    Loading
    LoadFailed
    Gaming
    GameOver
}

enum Direction4W {
    Right
    Up
    Left
    Down
}

// ============================================================
// ECS components / singletons / events / resources
// ============================================================

struct Facing4W {
    value: Direction4W
}

struct SnakeMove {
    interval: float
    lastTime: float
}

struct SnakeInput {
    rightKey, upKey, leftKey, downKey: int
}

struct IsApple {
}

struct SnakeBody {
    position: HMM_Vec3
    entity: Entity
}

struct SnakeBodies {
    bodies: array<SnakeBody>
}

struct Border {
    up, bottom, left, right: int
}

struct Score {
    value: int
}

struct SnakeResources {
    quad: PulseMeshHandle
    appleMat, snakeHeadMat, snakeBodyMat: PulseMaterialHandle
    boardMat: PulseMaterialHandle
}

struct SnakeAssets {
    shader: PulseShaderRequest
    mesh: PulseMeshRequest
}

struct SnakeMoveIntentEvent {
    delta: HMM_Vec3
}

struct AppleEatenEvent {
    apple: Entity
}

struct GameOverEvent {}

struct RestartEvent {}

// ============================================================
// Helpers
// ============================================================

let HMM_Q_Identity = HMM_Quat(X = 0.0, Y = 0.0, Z = 0.0, W = 1.0)
let HMM_V3_One = HMM_V3(1.0, 1.0, 1.0)

def operator ==(a, b : HMM_Vec3) {
    return a.X == b.X && a.Y == b.Y && a.Z == b.Z
}

def operator+(a, b: HMM_Vec3) {
    return HMM_V3(a.X + b.X, a.Y + b.Y, a.Z + b.Z)
}

def toDelta(direction: Direction4W) {
    if (direction == Direction4W.Right) {
        return HMM_V3(1.0, 0.0, 0.0)
    }
    elif (direction == Direction4W.Up) {
        return HMM_V3(0.0, 1.0, 0.0)
    }
    elif (direction == Direction4W.Left) {
        return HMM_V3(-1.0, 0.0, 0.0)
    }
    else {
        return HMM_V3(0.0, -1.0, 0.0)
    }
}

def isParallel(left: Direction4W, right: Direction4W) {
    return ((int(left) % 2) == (int(right) % 2))
}

def isOpposite(left: Direction4W, right: Direction4W) {
    return (left != right) && isParallel(left, right)
}

def createRenderable(cmd: CommandBuffer, position: HMM_Vec3, mat: PulseMaterialHandle, mesh: PulseMeshHandle) {
    let ent = cmd.entity()
    cmd |> set_entity_component(ent, PulseLocalTransform(translation = position, rotation = HMM_Q_Identity, scale = HMM_V3_One))
    cmd |> set_entity_component(ent, PulseRenderable(mesh = mesh, material = mat))
    return ent
}

def createBoard(cmd: CommandBuffer, position: HMM_Vec3, scale: HMM_Vec3, mat: PulseMaterialHandle, mesh: PulseMeshHandle) {
    let ent = cmd.entity()
    cmd |> set_entity_component(ent, PulseLocalTransform(translation = position, rotation = HMM_Q_Identity, scale = scale))
    cmd |> set_entity_component(ent, PulseRenderable(mesh = mesh, material = mat))
}

def createSnake(cmd: CommandBuffer, quad: PulseMeshHandle, headMat: PulseMaterialHandle, bodyMat: PulseMaterialHandle, initPos: HMM_Vec3) {
    let snakeInitLength = 3

    let head = createRenderable(cmd, initPos, headMat, quad)

    var bodies: array<SnakeBody>
    var poses: array<HMM_Vec3>
    bodies |> push(SnakeBody(position = initPos, entity = head))
    poses |> push(initPos)
    for (i in 0..snakeInitLength) {
        let bodyPos = HMM_V3(initPos.X - 1.0 - float(i), initPos.Y, initPos.Z)
        let newBody = createRenderable(cmd, bodyPos, bodyMat, quad)
        bodies |> push(SnakeBody(position = bodyPos, entity = newBody), 0)
        poses |> push(bodyPos)
    }

    let snake = cmd.entity()
    cmd |> set_entity_component(snake, SnakeBodies(bodies <- bodies))
    cmd |> set_entity_component(snake, Facing4W(value = Direction4W.Right))
    cmd |> set_entity_component(snake, SnakeInput(rightKey = 79, upKey = 82, leftKey = 80, downKey = 81))
    cmd |> set_entity_component(snake, SnakeMove(interval = 1.0, lastTime = 0.0))
    return (snake, <- poses)
}

def random_range(var seed : int4&, min: int, max: int) {
    var value = random_int(seed)
    value %= (max - min)
    value += min
    return value
}

def getNewApplePosition(snakeBodyPoses: array<HMM_Vec3>, border: Border) {
    let left = border.left
    let right = border.right
    let bottom = border.bottom
    let up = border.up

    let maxCount = (right - left - 1) * (up - bottom - 1)

    if (length(snakeBodyPoses) >= maxCount) {
        return none(type<HMM_Vec3>)
    }

    var s1 = random_seed(13141516)

    var used: array<HMM_Vec3>
    for (i in 0..length(snakeBodyPoses)) {
        used |> push(snakeBodyPoses[i])
    }

    while (true) {
        let x = random_range(s1, left + 1, right - 1)
        let y = random_range(s1, bottom + 1, up - 1)
        let newPos = HMM_V3(float(x), float(y), 0.0)

        if (!(left < x && x < right && bottom < y && y < up)) {
            continue
        }

        var bodyOverlaped = false
        for (i in 0..length(used)) {
            if (used[i] == newPos) {
                bodyOverlaped = true
                continue
            }
        }

        if (!bodyOverlaped) {
            return some(newPos)
        }
    }

    return none(type<HMM_Vec3>)
}

def createApple(cmd: CommandBuffer, snakeBodyPoses: array<HMM_Vec3>, border: Border, quad: PulseMeshHandle, appleMat: PulseMaterialHandle) {
    let newPos = getNewApplePosition(snakeBodyPoses, border)
    if (newPos |> is_none) {
        return default<Entity>
    }

    let apple = createRenderable(cmd, unwrap(newPos), appleMat, quad)
    cmd |> set_entity_component(apple, IsApple())
    return apple
}

def createEntities(cmd: CommandBuffer, border: Border, resources: SnakeResources) {
    cmd |> set_singleton(Score(value = 0))
    let (_, poses) = createSnake(cmd, resources.quad, resources.snakeHeadMat, resources.snakeBodyMat, HMM_V3(0.0, 0.0, 0.0))
    createApple(cmd, poses, border, resources.quad, resources.appleMat)
}

def destructEntities(cmd: CommandBuffer, snakeQuery: Query<SnakeBodies const>, appleQuery: Query<IsApple const>) {
    snakeQuery |> for_each() $(entity, snake) {
        for (i in 0..length(snake.bodies)) {
            cmd |> destruct(snake.bodies[i].entity)
        }
        cmd |> destruct(entity)
    }

    appleQuery |> for_each() $(entity, apple) {
        cmd |> destruct(entity)
    }
}

def createBorder(cmd: CommandBuffer, quad: PulseMeshHandle, borderMat: PulseMaterialHandle, up: int, bottom: int, left: int, right: int) {
    let centerX = float(left + right) / 2.0
    let centerY = float(up + bottom) / 2.0
    let width = float(right - left) + 1.0
    let height = float(up - bottom) + 1.0
    createBoard(cmd, HMM_V3(float(right), centerY, 0.0), HMM_V3(1.0, height, 1.0), borderMat, quad)
    createBoard(cmd, HMM_V3(float(left), centerY, 0.0), HMM_V3(1.0, height, 1.0), borderMat, quad)
    createBoard(cmd, HMM_V3(centerX, float(up), 0.0), HMM_V3(width, 1.0, 1.0), borderMat, quad)
    createBoard(cmd, HMM_V3(centerX, float(bottom), 0.0), HMM_V3(width, 1.0, 1.0), borderMat, quad)

    let border = Border(up = up, bottom = bottom, left = left, right = right)
    cmd |> set_singleton(border)
    return border
}

def getInputDirection(input: SnakeInput, keyboardState: PulseKeyboardInput) {
    let right = keyboardState.just_pressed[input.rightKey]
    let up = keyboardState.just_pressed[input.upKey]
    let left = keyboardState.just_pressed[input.leftKey]
    let down = keyboardState.just_pressed[input.downKey]

    if (right) {
        return some(Direction4W.Right)
    }
    elif (up) {
        return some(Direction4W.Up)
    }
    elif (left) {
        return some(Direction4W.Left)
    }
    elif (down) {
        return some(Direction4W.Down)
    }
    else {
        return none(type<Direction4W>)
    }
}

enum ObstacleType {
    None
    Apple
    SnakeBody
    Border
}

def queryCollideObstacle(nextPos: HMM_Vec3, snake: SnakeBodies&, border: Border, applePos: Option<HMM_Vec3>) : ObstacleType {
    let left = border.left
    let right = border.right
    let bottom = border.bottom
    let up = border.up

    if (nextPos.X <= float(left) || nextPos.X >= float(right) || nextPos.Y <= float(bottom) || nextPos.Y >= float(up))
        return ObstacleType.Border

    for (i in 1..length(snake.bodies) - 1) {
        if (snake.bodies[i].position == nextPos)
            return ObstacleType.SnakeBody
    }

    if (applePos |> is_some) {
        if (nextPos == unwrap(applePos))
            return ObstacleType.Apple
    }

    return ObstacleType.None
}

// ============================================================
// Systems (PULSE_ECS_SYSTEM + STATE)
// ============================================================

[system(phase="update", state="UnInitialized|Loading")]
def loadSnakeResourcesSystem(app: PulseAppId, assets: Res<SnakeAssets>, state: SystemStateMachine<SnakeGameState>, cmd: CommandBuffer, primaryWindowQuery: Query<PulseWindow const, PulsePrimaryWindow const>) {
    if (state.is_state(SnakeGameState.UnInitialized)) {
        var blend_attachments = CGPUBlendAttachmentState(
            enable = false,
            src_factor = ECGPUBlendFactor.CGPU_BLEND_FACTOR_ONE,
            dst_factor = ECGPUBlendFactor.CGPU_BLEND_FACTOR_ZERO,
            src_alpha_factor = ECGPUBlendFactor.CGPU_BLEND_FACTOR_ONE,
            dst_alpha_factor = ECGPUBlendFactor.CGPU_BLEND_FACTOR_ZERO,
            blend_op = ECGPUBlendOp.CGPU_BLEND_OP_ADD,
            blend_alpha_op = ECGPUBlendOp.CGPU_BLEND_OP_ADD,
            color_mask = uint(ECGPUColorMaskFlagBits.CGPU_COLOR_MASK_RGBA))
        let blend_desc = CGPUBlendStateDescriptor(
            attachment_count = 1u,
            p_attachments = unsafe(addr(blend_attachments)),
            alpha_to_coverage = false,
            independent_blend = false)
        let depth_desc = CGPUDepthStateDescriptor(
            depth_test = true,
            depth_write = true,
            depth_op = ECGPUCompareOp.CGPU_COMPARE_OP_GREATER_EQUAL,
            stencil_test = false)
        let rasterizer_state = CGPURasterizerStateDescriptor(
            cull_mode = uint(ECGPUCullModeFlagBits.CGPU_CULL_MODE_BACK),
            front_face = ECGPUFrontFace.CGPU_FRONT_FACE_CLOCK_WISE)

        var shader_props: PulseShaderProperty[3]
        shader_props[0] = PulseShaderProperty(
            name = "vpMatrix",
            _type = EPulseShaderPropertyType.PULSE_SHADER_PROPERTY_TYPE_MAT4,
            role = EPulseShaderPropertyRole.PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL,
            set = 0u, binding = 0u, offset = 0u, size = 64u)
        shader_props[1] = PulseShaderProperty(
            name = "albedo",
            _type = EPulseShaderPropertyType.PULSE_SHADER_PROPERTY_TYPE_FLOAT4,
            role = EPulseShaderPropertyRole.PULSE_SHADER_PROPERTY_ROLE_MATERIAL,
            set = 1u, binding = 0u, offset = 0u, size = 16u)
        shader_props[2] = PulseShaderProperty(
            name = "wMatrix",
            _type = EPulseShaderPropertyType.PULSE_SHADER_PROPERTY_TYPE_MAT4,
            role = EPulseShaderPropertyRole.PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL,
            set = 2u, binding = 0u, offset = 0u, size = 64u)

        let shader_desc = PulseShaderCreateFromFileDesc(
            vert_path = "assets/color.vert.spv",
            frag_path = "assets/color.frag.spv",
            blend_desc = blend_desc,
            depth_desc = depth_desc,
            rasterizer_state = rasterizer_state,
            property_count = 3u,
            p_properties = unsafe(addr(shader_props[0])))

        assets.get().shader = pulse_create_shader_from_file(app, unsafe(addr(shader_desc)))
        assets.get().mesh = pulse_load_mesh(app, "assets/Quad.obj")
        state.to(SnakeGameState.Loading)
        return
    }

    let asset_system = pulse_get_asset_system(app)
    let shader_request = pulse_shader_request_to_asset_request(assets.get().shader)
    let mesh_request = pulse_mesh_request_to_asset_request(assets.get().mesh)
    let shader_state = pulse_asset_system_get_state(asset_system, shader_request)
    let mesh_state = pulse_asset_system_get_state(asset_system, mesh_request)
    if (shader_state == EPulseAssetState.PULSE_ASSET_STATE_FAILED || mesh_state == EPulseAssetState.PULSE_ASSET_STATE_FAILED) {
        if (shader_state == EPulseAssetState.PULSE_ASSET_STATE_FAILED) {
            print("Snake shader load failed.\n")
        }
        if (mesh_state == EPulseAssetState.PULSE_ASSET_STATE_FAILED) {
            print("Snake mesh load failed.\n")
        }
        state.to(SnakeGameState.LoadFailed)
        return
    }

    if (!pulse_shader_is_ready(app, assets.get().shader) || !pulse_mesh_is_ready(app, assets.get().mesh)) {
        return
    }

    var shader = pulse_shader_get_handle(app, assets.get().shader)
    var quad = pulse_mesh_get_handle(app, assets.get().mesh)

    var boardMat, appleMat, snakeHeadMat, snakeBodyMat: PulseMaterialHandle
    {
        let mat_desc = PulseMaterialCreateDesc(shader = shader)
        boardMat = pulse_create_material(app, unsafe(addr(mat_desc)))
        pulse_material_set_property_float4(app, boardMat, "albedo", HMM_V4(1.0, 1.0, 1.0, 1.0))
    }
    {
        let mat_desc = PulseMaterialCreateDesc(shader = shader)
        appleMat = pulse_create_material(app, unsafe(addr(mat_desc)))
        pulse_material_set_property_float4(app, appleMat, "albedo", HMM_V4(1.0, 0.0, 0.0, 0.0))
    }
    {
        let mat_desc = PulseMaterialCreateDesc(shader = shader)
        snakeHeadMat = pulse_create_material(app, unsafe(addr(mat_desc)))
        pulse_material_set_property_float4(app, snakeHeadMat, "albedo", HMM_V4(1.0, 1.0, 0.0, 1.0))
    }
    {
        let mat_desc = PulseMaterialCreateDesc(shader = shader)
        snakeBodyMat = pulse_create_material(app, unsafe(addr(mat_desc)))
        pulse_material_set_property_float4(app, snakeBodyMat, "albedo", HMM_V4(0.0, 1.0, 0.0, 1.0))
    }

    let resources = SnakeResources(
        quad = quad,
        appleMat = appleMat,
        snakeHeadMat = snakeHeadMat,
        snakeBodyMat = snakeBodyMat,
        boardMat = boardMat)
    cmd |> set_singleton(resources)

    let up = 16
    let bottom = -15
    let left = -20
    let right = 21

    let border = createBorder(cmd, quad, boardMat, up, bottom, left, right)
    createEntities(cmd, border, resources)

    let (windowEntity, _, _) = primaryWindowQuery |> first()
    let camera = cmd.entity()
    cmd |> set_entity_component(camera, PulseLocalTransform(translation = HMM_V3(0.5, 0.5, -38.0), rotation = HMM_Q_Identity, scale = HMM_V3_One))
    cmd |> set_entity_component(camera, PulseCamera(window_entity = windowEntity, fov = 45.0, near_plane = 0.1, far_plane = 1000.0))

    state.to(SnakeGameState.Gaming)
    print("Snake resources ready.\n")
}

[system(phase="update", state="Gaming")]
def handleSnakeInputSystem(keyboard: Res<PulseKeyboardInput const>, input: SnakeInput, var direction: Facing4W, var move: SnakeMove) {
    let keyInput = getInputDirection(input, keyboard.get())
    if (keyInput |> is_some) {
        let value = keyInput |> unwrap
        if (!isOpposite(direction.value, value)) {
            direction.value = value
            move.lastTime = move.interval
        }
    }
}

[system(phase="update", state="Gaming")]
def scheduleSnakeMoveSystem(timer: Res<PulseTimer const>, snakeMoveWriter: EventWriter<SnakeMoveIntentEvent>, entity: Entity, direction: Facing4W, var move: SnakeMove) {
    move.lastTime += timer.get().delta_time
    if (move.lastTime < move.interval) {
        return
    }

    while (move.lastTime > move.interval) {
        move.lastTime -= move.interval
    }

    let delta = toDelta(direction.value)
    snakeMoveWriter |> send(type<SnakeBodies>, entity, SnakeMoveIntentEvent(delta = delta))
}

[system(state="Gaming")]
def executeSnakeMoveSystem(snakeMoveIntentEvent: EventReader<SnakeMoveIntentEvent>, cmd: CommandBuffer, appleQuery: Query<IsApple const, PulseLocalTransform const>, borderQuery: SingletonQuery<Border const>, resources: SingletonQuery<SnakeResources const>, appleEatenEvent: EventWriter<AppleEatenEvent>, gameOverEvent: EventWriter<GameOverEvent>, var snake: SnakeBodies) {
    let event = snakeMoveIntentEvent.read()
    let delta = event.delta

    let headIdx = length(snake.bodies) - 1
    let headPosition = snake.bodies[headIdx].position
    let nextPos = headPosition + delta

    let (appleEnt, _, appleTransform) = appleQuery |> first()
    var applePos = some(appleTransform.translation)

    let border = borderQuery.get()
    let obstacle = queryCollideObstacle(nextPos, snake, border, applePos)

    if (obstacle == ObstacleType.None) {
        for (i in 0..length(snake.bodies) - 1) {
            snake.bodies[i].position = snake.bodies[i + 1].position
        }
        snake.bodies[headIdx].position = nextPos
    }
    elif (obstacle == ObstacleType.Apple) {
        snake.bodies[headIdx].position = nextPos
        let newBody = createRenderable(cmd, headPosition, resources.get().snakeBodyMat, resources.get().quad)
        snake.bodies |> push(SnakeBody(position = headPosition, entity = newBody), length(snake.bodies) - 1)

        appleEatenEvent |> broadcast(AppleEatenEvent(apple = appleEnt))
    }
    else {
        gameOverEvent |> broadcast(GameOverEvent())
    }
}

[system(phase="update", state="Gaming")]
def syncSnakeBodyPositionSystem(var snake: SnakeBodies, cmd: CommandBuffer) {
    for (i in 0..length(snake.bodies)) {
        let body = snake.bodies[i]
        cmd |> set_entity_component(body.entity, PulseLocalTransform(translation = body.position, rotation = HMM_Q_Identity, scale = HMM_V3_One))
    }
}

[system(state="Gaming")]
def eatAppleSystem(eventAppleEat: EventReader<AppleEatenEvent>, cmd: CommandBuffer) {
    cmd.destruct(eventAppleEat.read().apple)
}

[system(state="Gaming")]
def increaseScoreSystem(eventAppleEat: EventReader<AppleEatenEvent>, scoreQuery: SingletonQuery<Score>) {
    scoreQuery.get().value += 1
}

[system(state="Gaming")]
def spawnAppleSystem(eventAppleEat: EventReader<AppleEatenEvent>, cmd: CommandBuffer, snakeQuery: Query<SnakeBodies const>, borderQuery: SingletonQuery<Border const>, resources: SingletonQuery<SnakeResources const>) {
    let (_, snakeBodies) = snakeQuery.first()
    var poses: array<HMM_Vec3>
    for (i in 0..length(snakeBodies.bodies)) {
        poses |> push(snakeBodies.bodies[i].position)
    }

    createApple(cmd, poses, borderQuery.get(), resources.get().quad, resources.get().appleMat)
}

[system(state="Gaming")]
def onGameOverSystem(eventGameOver: EventReader<GameOverEvent>, cmd: CommandBuffer, state: SystemStateMachine<SnakeGameState>, snakeQuery: Query<SnakeBodies const>, appleQuery: Query<IsApple const>) {
    state.to(SnakeGameState.GameOver)
    destructEntities(cmd, snakeQuery, appleQuery)
}

[system(phase="imgui", state="Gaming|GameOver")]
def snakeUISystem(score: Score, primaryWindowQuery: Query<PulseWindow const, PulsePrimaryWindow const>, state: SystemStateMachine<SnakeGameState>, keyboard: Res<PulseKeyboardInput const>, restartEvent: EventWriter<RestartEvent>) {
    if (state.is_state(SnakeGameState.Gaming)) {
        Text("{score.value}")
    }
    else {
        if (Button("Restart")) {
            restartEvent |> broadcast(RestartEvent())
        }
    }
}

[system(phase="imgui")]
def snakeFpsUISystem(timer: Res<PulseTimer const>) {
    Text("FPS: {timer.get().fps}")
}

[system(state="GameOver")]
def restartSystem(restartEvent: EventReader<RestartEvent>, cmd: CommandBuffer, state: SystemStateMachine<SnakeGameState>, borderQuery: SingletonQuery<Border const>, resources: SingletonQuery<SnakeResources const>) {
    // 事件观察器运行期间 world 处于 deferred/staging 状态，
    // 与 C++ snake 保持一致：临时挂起 defer，让实体立即创建。
    cmd.defer_suspend()
    createEntities(cmd, borderQuery.get(), resources.get())
    cmd.defer_resume()
    state.to(SnakeGameState.Gaming)
}

// ============================================================
// Module entry (DLL plugin protocol name)
// ============================================================

[export]
def importModule(var ctx: ModuleContext) {
    state_machine_init(ctx.world, ctx.updatePipeline, type<SnakeGameState>, SnakeGameState.UnInitialized)
    register_resource(ctx.world, type<SnakeAssets>)

    ctx |> register_system(@@loadSnakeResourcesSystem)
    ctx |> register_system(@@handleSnakeInputSystem)
    ctx |> register_system(@@scheduleSnakeMoveSystem)
    ctx |> register_system(@@executeSnakeMoveSystem)
    ctx |> register_system(@@syncSnakeBodyPositionSystem)
    ctx |> register_system(@@eatAppleSystem)
    ctx |> register_system(@@increaseScoreSystem)
    ctx |> register_system(@@spawnAppleSystem)
    ctx |> register_system(@@onGameOverSystem)
    ctx |> register_system(@@snakeUISystem)
    ctx |> register_system(@@snakeFpsUISystem)
    ctx |> register_system(@@restartSystem)
}
