options gen2

require flecs_boost
require pulse
require cgpu
require daslib/random
require daslib/match
require daslib/option

enum Direction4W {
	Right
	Up
	Left
	Down
}

def toDelta(direction: Direction4W)
{
	match (direction) {
        if (Direction4W.Right) {
            return HMM_V3(1.0, 0.0, 0.0);
        }
        if (Direction4W.Up) {
            return HMM_V3(0.0, 1.0, 0.0);
        }
        if (Direction4W.Left) {
            return HMM_V3(-1.0, 0.0, 0.0);
        }
        if (Direction4W.Down) {
            return HMM_V3(0.0, -1.0, 0.0);
        }
        if (_) {
            return HMM_V3(0.0, 0.0, 0.0);
        }
	}
}

def isParallel(left: Direction4W, right: Direction4W)
{
	return ((int(left) % 2) == (int(right) % 2));
}

def isOpposite(left: Direction4W, right: Direction4W)
{
	return (left != right) && isParallel(left, right);
}

struct Facing4W {
	value: Direction4W
}

struct SnakeMove {
	interval: float
	lastTime: float
}

struct SnakeInput
{
	rightKey, upKey, leftKey, downKey: int
}

struct IsApple
{
}

struct Velocity {
    X: float
    Y: float
    Z: float
}

struct Comp2 {
    X: float
}

struct SnakeBody {
	position: HMM_Vec3
	entity: Entity
}

struct SnakeBodies {
	bodies: array<SnakeBody>
}

struct SnakeGame {
	playing: bool
}

struct Score {
	value: int
}

struct SnakeMoveIntentEvent {
	delta: HMM_Vec3
}

struct AppleEatenEvent {
    apple: Entity
}

struct GameOverEvent {}

var private run_probe_batches = 0
var private move_probe_hits = 0
var private spawned_entity = default<Entity>

[system(phase="update")]
def run_probe(var iter: ecs_iter_t?) {
    while (iter_next(iter)) {
        run_probe_batches += 1
        if (run_probe_batches == 1) {
            print("spawned_entity_id=")
            dump_entity(spawned_entity)
            print("run_probe batch={iter.count} tick={run_probe_batches}\n")
        }
    }
}

[system(phase="update")]
def move_probe(e: Entity; pos: Position const; var vel: Velocity) {
    move_probe_hits += 1
    vel.X += 1.0
    if (move_probe_hits == 1) {
        print("move_probe entity_id=")
        dump_entity(e)
        print("move_probe pos={pos.value.X} vel={vel.X} {vel.Y} {vel.Z}\n")
    }
}

[system(phase="init")]
def system2(var cmd: CommandBuffer; position : Position) {
    print("system2\n")
    let e2 = cmd.entity()
    dump_entity(e2)
}

[system(phase="init")]
def system3(var res: $Res<Position>; position : Position) {
    print("system3\n")
    let value = res.get()
    // print("res Position={value.value.X} {value.value.Y} {value.value.Z}\n")
}

[system(phase="init")]
def system4(var query: $SingletonQuery<Position>; position : Position) {
    print("system4\n")
    let value = query.get()
    print("singleton Position={value.value.X} {value.value.Y} {value.value.Z}\n")
}

[system(phase="init")]
def system5(var event_writer: $EventWriter<Event1>; position : Position) {
    print("system5\n")
    event_writer |> broadcast(Event1())
}

[system]
def system6(var event_reader: $EventReader<Event1>) {
    print("system receive event!\n")
}

def show_singleton_position(query: $SingletonQuery<Position>) {
    let value = query.get()
    print("singleton Position={value.value.X} {value.value.Y} {value.value.Z}\n")
}

struct Event1 {

}

struct MaterialData
{
    albedo: float4;
};

struct SnakeResources
{
	quad: uint64;
	appleMat, snakeHeadMat, snakeBodyMat: uint64;
	boardMat: uint64;
};

struct Border
{
	up, bottom, left, right: int;
};

def pulse_material_bind_buffer(var resourceManager: ResourceManager?, material: uint64, set: int, bind: int, buffer: auto(T))
{
    material_bind_buffer(resourceManager, material, set, bind, typeinfo sizeof(type<T>), unsafe(addr(buffer)))
}

def createBoard(var cmd: CommandBuffer, matrix: HMM_Mat4, mat: uint64, mesh: uint64) {
	let ent = cmd.entity();
    cmd |> set_entity_component(ent, WorldTransform(value = matrix))
    cmd |> set_entity_component(ent, Rendable(material = int(mat), mesh = int(mesh)))
    cmd |> set_entity_component(ent, ShowMatrix(model = matrix))
}

let HMM_Q_Identity = HMM_Quat(X = 0.0, Y = 0.0, Z = 0.0, W = 1.0)
let HMM_M4_Identity = HMM_Mat4(Columns = fixed_array<HMM_Vec4>(HMM_V4(1.0, 0.0, 0.0, 0.0), HMM_V4(0.0, 1.0, 0.0, 0.0), HMM_V4(0.0, 0.0, 1.0, 0.0), HMM_V4(0.0, 0.0, 0.0, 1.0)))

def TRS(Translate: HMM_Vec3, Rotation: HMM_Quat, Scale: HMM_Vec3) {
    var mat: HMM_Mat4
    HMM_TRS(Translate, Rotation, Scale, mat)
    return mat
}

def operator ==(a, b : HMM_Vec3) {
    return a.X == b.X && a.Y == b.Y && a.Z == b.Z
}

def operator+(a, b: HMM_Vec3) {
    return HMM_V3(a.X + b.X, a.Y + b.Y, a.Z + b.Z)
}

def createRenderable(var cmd: CommandBuffer, position: HMM_Vec3, mat: uint64, mesh: uint64) {
	let ent = cmd.entity();
    cmd |> set_entity_component(ent, LocalTransform(model = HMM_M4_Identity))
    cmd |> set_entity_component(ent, WorldTransform(value = HMM_M4_Identity))
    cmd |> set_entity_component(ent, Position(value = position))
    cmd |> set_entity_component(ent, Rendable(material = int(mat), mesh = int(mesh)))
    cmd |> set_entity_component(ent, ShowMatrix(model = HMM_M4_Identity))
	return ent;
}

def createSnake(var cmd: CommandBuffer, quad: uint64, headMat: uint64, bodyMat: uint64, initPos: HMM_Vec3) {
	let snakeInitLength = 3;

	let head = createRenderable(cmd, initPos, headMat, quad);

	var bodies: array<SnakeBody>;
    var poses: array<HMM_Vec3>;
	bodies |> push(SnakeBody(position = initPos, entity = head));
    poses |> push(initPos)
	for (i in 0..snakeInitLength)
	{
		let bodyPos = HMM_V3(initPos.X - 1.0 - float(i), initPos.Y, initPos.Z);
		let newBody = createRenderable(cmd, bodyPos, bodyMat, quad);
		bodies |> push(SnakeBody(position = bodyPos, entity = newBody), 0);
        poses |> push(bodyPos)
	}

	let snake = cmd.entity();
    cmd |> set_entity_component(snake, SnakeBodies(bodies <- bodies))
    cmd |> set_entity_component(snake, Facing4W(value = Direction4W.Right))
    cmd |> set_entity_component(snake, SnakeInput(rightKey = 79, upKey = 82, leftKey = 80, downKey = 81))
    cmd |> set_entity_component(snake, SnakeMove(interval = 1.0, lastTime = 0.0))
	return (snake, <- poses);
}

def random_range(var seed : int4&, min: int, max: int) {
    var value = random_int(seed)
    value %= (max - min)
    value += min
    return value
}

def getNewApplePosition(snakeBodyPoses: array<HMM_Vec3>, border: Border) {
	let left = border.left;
	let right = border.right;
	let bottom = border.bottom;
	let up = border.up;

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

		var bodyOverlaped = false;
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

def createApple(var cmd: CommandBuffer, snakeBodyPoses: array<HMM_Vec3>, border: Border, quad: uint64, appleMat: uint64) {
	let newPos = getNewApplePosition(snakeBodyPoses, border);
    if (newPos |> is_none) {
        return default<Entity>;
    }

	let apple = createRenderable(cmd, unwrap(newPos), appleMat, quad);
    cmd |> set_entity_component(apple, IsApple())
	return apple;
}

def createEntities(var cmd: CommandBuffer, border: Border, resources: SnakeResources) {
    cmd |> set_singleton(SnakeGame(playing = true))
	cmd |> set_singleton(Score(value = 0))
	let (snake, poses) = createSnake(cmd, resources.quad, resources.snakeHeadMat, resources.snakeBodyMat, HMM_V3(0.0, 0.0, 0.0));
	let apple = createApple(cmd, poses, border, resources.quad, resources.appleMat);
}

def createBorder(var cmd: CommandBuffer, quad: uint64, borderMat: uint64, up: int, bottom: int, left: int, right: int) {
	let centerX = float(left + right) / 2.0;
	let centerY = float(up + bottom) / 2.0;
	let width = float(right - left) + 1.0;
	let height = float(up - bottom) + 1.0;
	createBoard(cmd, TRS(HMM_V3(float(right), centerY, 0.0), HMM_Q_Identity, HMM_V3(1.0, height, 1.0)), borderMat, quad);
	createBoard(cmd, TRS(HMM_V3(float(left), centerY, 0.0), HMM_Q_Identity, HMM_V3(1.0, height, 1.0)), borderMat, quad);
	createBoard(cmd, TRS(HMM_V3(centerX, float(up), 0.0), HMM_Q_Identity, HMM_V3(width, 1.0, 1.0)), borderMat, quad);
	createBoard(cmd, TRS(HMM_V3(centerX, float(bottom), 0.0), HMM_Q_Identity, HMM_V3(width, 1.0, 1.0)), borderMat, quad);

	let border = Border(up = up, bottom = bottom, left = left,  right = right);
	cmd |> set_singleton(border);
	return border;
}

def getInputDirection(input: SnakeInput, keyboardState: KeyboardState&) {
	let lastRight = keyboardState.lastKeys[input.rightKey] != uint8(0)
	let lastUp = keyboardState.lastKeys[input.upKey] != uint8(0)
	let lastLeft = keyboardState.lastKeys[input.leftKey] != uint8(0)
	let lastDown = keyboardState.lastKeys[input.downKey] != uint8(0)

	let right = keyboardState.currentKeys[input.rightKey] != uint8(0)
	let up = keyboardState.currentKeys[input.upKey] != uint8(0)
	let left = keyboardState.currentKeys[input.leftKey] != uint8(0)
	let down = keyboardState.currentKeys[input.downKey] != uint8(0)

	if (!lastRight && right) {
        return some(Direction4W.Right)
    }
	elif (!lastUp && up) {
        return some(Direction4W.Up)
    }
	elif (!lastLeft && left) {
        return some(Direction4W.Left)
    }
	elif (!lastDown && down) {
        return some(Direction4W.Down)
    }
	else {
        return none(type<Direction4W>)
    }
}

[system(phase="init")]
def loadSnakeResources(var res: $Res<ResourceManager>; var cmd: CommandBuffer) {
    print("load resource!\n")

	var blend_attachments = CGPUBlendAttachmentState(
		enable = false,
		src_factor = ECGPUBlendFactor.CGPU_BLEND_FACTOR_ONE,
		dst_factor = ECGPUBlendFactor.CGPU_BLEND_FACTOR_ZERO,
		src_alpha_factor = ECGPUBlendFactor.CGPU_BLEND_FACTOR_ONE,
		dst_alpha_factor = ECGPUBlendFactor.CGPU_BLEND_FACTOR_ZERO,
		blend_op = ECGPUBlendOp.CGPU_BLEND_OP_ADD,
		blend_alpha_op = ECGPUBlendOp.CGPU_BLEND_OP_ADD,
		color_mask = uint(ECGPUColorMaskFlagBits.CGPU_COLOR_MASK_RGBA),
	);
	let blend_desc = CGPUBlendStateDescriptor(
		attachment_count = 1u,
		p_attachments = unsafe(addr(blend_attachments)),
		alpha_to_coverage = false,
		independent_blend = false,
	);
	let depth_desc = CGPUDepthStateDescriptor(
		depth_test = true,
		depth_write = true,
		depth_op = ECGPUCompareOp.CGPU_COMPARE_OP_GREATER_EQUAL,
		stencil_test = false,
	);
	let rasterizer_state = CGPURasterizerStateDescriptor(
		cull_mode = uint(ECGPUCullModeFlagBits.CGPU_CULL_MODE_BACK),
		front_face	= ECGPUFrontFace.CGPU_FRONT_FACE_CLOCK_WISE,
	);

    var resMan = res.get()
    let shader = load_shader(resMan, "shaderbin/color.vert.spv", "shaderbin/color.frag.spv", blend_desc, depth_desc, rasterizer_state)

	let quad = load_mesh(resMan, "media/models/Quad.obj");

    let boardMat = create_material(resMan, shader)
    let appleMat = create_material(resMan, shader)
    let snakeHeadMat = create_material(resMan, shader)
    let snakeBodyMat = create_material(resMan, shader)
    
    {
        let materialData = MaterialData(albedo = float4(1, 1, 1, 1))
        pulse_material_bind_buffer(resMan, boardMat, 1, 0, materialData)
    }
    
    {
        let materialData = MaterialData(albedo = float4(1, 0, 0, 0))
        pulse_material_bind_buffer(resMan, appleMat, 1, 0, materialData)
    }
    
    {
        let materialData = MaterialData(albedo = float4(1, 1, 0, 1))
        pulse_material_bind_buffer(resMan, snakeHeadMat, 1, 0, materialData)
    }
    
    {
        let materialData = MaterialData(albedo = float4(0, 1, 0, 1))
        pulse_material_bind_buffer(resMan, snakeBodyMat, 1, 0, materialData)
    }

    cmd |> set_singleton(SnakeResources(quad = quad, boardMat = boardMat, appleMat = appleMat, snakeHeadMat = snakeHeadMat, snakeBodyMat = snakeBodyMat))
}

[system(phase="init")]
def initSnakeGameSystem(var cmd: CommandBuffer, resources_query: $SingletonQuery<SnakeResources>) {
    print("resource loaded!\n")
    let resources = resources_query.get()
    let up: int = 16
	let bottom: int = -15
	let left: int = -20
	let right: int = 21

	let border = createBorder(cmd, resources.quad, resources.boardMat, up, bottom, left, right);
    createEntities(cmd, border, resources)
}

[system(phase="update")]
def handleSnakeInputSystem(var resKeyboardState: $Res<KeyboardState>; input: SnakeInput, var direction: Facing4W, var move: SnakeMove) {
    let keyInput = getInputDirection(input, deref(resKeyboardState.get()))
    if (keyInput |> is_some) {
        let value = keyInput |> unwrap
        if (!isOpposite(direction.value, value)) {
            direction.value = value
            move.lastTime = move.interval;
        }
    }
}

[system(phase="update")]
def scheduleSnakeMoveSystem(var context: $Res<UpdateContext>, var snakeMoveWriter: $EventWriter<SnakeMoveIntentEvent> , entity: Entity, direction: Facing4W, var move: SnakeMove) {	
	move.lastTime += context.get().value.delta_time
	if (move.lastTime < move.interval) {
        return
    }

	while (move.lastTime > move.interval) {
        move.lastTime -= move.interval
    }

	let delta = toDelta(direction.value)
	snakeMoveWriter |> send(type<SnakeBodies>, entity, SnakeMoveIntentEvent(delta = delta))
}

enum ObstacleType {
    None
    Apple
    SnakeBody
    Border
}

def queryCollideObstacle(nextPos: HMM_Vec3, snake: SnakeBodies&, border: Border, applePos: $Option<HMM_Vec3>) : ObstacleType {
    let left = border.left
    let right = border.right
    let bottom = border.bottom
    let up = border.up

    if (nextPos.X <= float(left) || nextPos.X >= float(right) || nextPos.Y <= float(bottom) || nextPos.Y >= float(up))
        return ObstacleType.Border

    assume bodies = snake.bodies
    for (i in 1..length(bodies) - 1) {
        if (bodies[i].position == nextPos)
            return ObstacleType.SnakeBody
    }

    if (applePos |> is_some) {
        if (nextPos == (unwrap(applePos)))
            return ObstacleType.Apple
    }

    return ObstacleType.None
}

[system]
def executeSnakeMoveSystem(snakeMoveIntentEvent: $EventReader<SnakeMoveIntentEvent>, var cmd: CommandBuffer, var appleQuery: $Query2<IsApple, Position>, borderQuery: $SingletonQuery<Border>, resources: $SingletonQuery<SnakeResources>, var gameOverEvent: $EventWriter<GameOverEvent>, var appleEatenEvent: $EventWriter<AppleEatenEvent>, var snake: SnakeBodies) {
    let event = snakeMoveIntentEvent.read()
    let delta = event.delta

    let headIdx = length(snake.bodies) - 1
    let headPosition = snake.bodies[headIdx].position
    let nextPos = headPosition + delta

    let (appleEnt, _, applePosComp) = appleQuery |> first()
    var applePos = some(applePosComp.value)

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
    else    {
        gameOverEvent |> broadcast(GameOverEvent())
    }
}

[system(phase="update")]
def syncSnakeBodyPositionSystem(var snake: SnakeBodies, var positionQuery: $Query1<Position>) {
	for (i in 0..length(snake.bodies))
	{
		assume body = snake.bodies[i]
        ecs_query_entity_set(positionQuery, body.entity, Position(value = body.position))
	}
}

[system]
def eatAppleSystem(eventAppleEat: $EventReader<AppleEatenEvent>, var cmd: CommandBuffer) {
	cmd.destruct(eventAppleEat.read().apple)
}

[system]
def spawnAppleSystem(eventAppleEat: $EventReader<AppleEatenEvent>, var cmd: CommandBuffer, var snakeQuery: $Query1<SnakeBodies>, borderQuery: SingletonQuery<Border>, resources: SingletonQuery<SnakeResources>) {
	let (snakeEnt, snakeBodies) = snakeQuery.first()
    var poses: array<HMM_Vec3>;
	for (i in 0..length(snakeBodies.bodies))
	{
        let bodyPos = snakeBodies.bodies[i].position
        poses |> push(bodyPos)
	}

	createApple(cmd, poses, borderQuery.get(), resources.get().quad, resources.get().appleMat);
}


[export]
def importModule(var ctx:ModuleContext) {
    print("importModule(ModuleContext) from dascript\n")
    dump_world(ctx.world)
    var ent = create_entity(ctx.world)
    dump_entity(ent)
    dump_world(ctx.world)

    print("{ecs_component(ctx.world, type<Position>)}\n")
    print("{ecs_component(ctx.world, type<Position>)}\n")
    print("{ecs_component(ctx.world, type<Position>)}\n")
    print("{ecs_component(ctx.world, type<Velocity>)}\n")
    print("{ecs_component(ctx.world, type<Comp2>)}\n")

    var query1 = ctx.world |> query(type<Position>)
    let (first_of_query1_entity, first_of_query1_pos) = query1 |> first()
    print("query1 first: {first_of_query1_entity}, pos: {first_of_query1_pos.value}\n")

    print("query for_each\n")
    query1 |> for_each() <| $(entity, pos) {
        print("{entity}: {pos.value.X} {pos.value.Y} {pos.value.Z}\n")
    }

    run_probe_batches = 0
    move_probe_hits = 0
    spawned_entity = ent

    ctx |> register_system(@@run_probe)

    ctx |> register_system(@@move_probe)

    ctx |> register_system(@@system2)

    ctx |> register_system(@@system3)

    ctx |> register_system(@@system4)

    ctx |> register_system(@@system5)

    ctx |> register_system(@@system6)

    ctx |> register_system(@@loadSnakeResources)
    ctx |> register_system(@@initSnakeGameSystem)
    ctx |> register_system(@@handleSnakeInputSystem)
    ctx |> register_system(@@scheduleSnakeMoveSystem)
    ctx |> register_system(@@executeSnakeMoveSystem)
    ctx |> register_system(@@syncSnakeBodyPositionSystem)
    ctx |> register_system(@@eatAppleSystem)
    ctx |> register_system(@@spawnAppleSystem)

    {
        var cmd = CommandBuffer(world = ctx.world)
        var ent2 = cmd.entity()
        dump_entity(ent2)
        cmd.destruct(ent2)
        cmd |> set_singleton(Position(value=HMM_V3(1.0, 4.0, 1.0)))
    }
}
