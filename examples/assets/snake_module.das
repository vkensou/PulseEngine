options gen2

require flecs_boost
require pulse
require cgpu

[cpp_layout]
struct Position {
    value : HMM_Vec3
}

struct Velocity {
    X: float
    Y: float
    Z: float
}

struct Comp2 {
    X: float
}

def system1(var iter: Iter?) {
    print("hello system\n")
    while (iter_next(iter)) {
    }
}

var private run_probe_batches = 0
var private move_probe_hits = 0
var private spawned_entity = default<Entity>

[system(phase="update")]
def run_probe(var iter: Iter?) {
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
    event_writer.broadcast()
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

def TRS(Translate: HMM_Vec3, Rotation: HMM_Quat, Scale: HMM_Vec3) {
    var mat: HMM_Mat4
    HMM_TRS(Translate, Rotation, Scale, mat)
    return mat
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

    register_system_from_query_expr(ctx.world, "move", ctx.initPipeline, "Position, Velocity", @@system1)

    var query = build_query(ctx.world, "Position, Velocity")
    var query_iter = query_iter(query)
    var iter = unsafe(addr(query_iter))
    while (iter_next(iter)) {
        print("query iter:{iter.count}\n")
        unsafe {
            var positions = reinterpret<Position?>(iter_field(iter, typeinfo sizeof(type<Position>), 0))
            var velocities = reinterpret<Velocity?>(iter_field(iter, typeinfo sizeof(type<Velocity>), 1))
            for (i in 0..iter.count) {
                print("{i}\n")
                print("{positions[i].value.X} {positions[i].value.Y} {positions[i].value.Z}\n")
                print("{velocities[i].X} {velocities[i].Y} {velocities[i].Z}\n")
            }
        }
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

    {
        var cmd = CommandBuffer(world = ctx.world)
        var ent2 = cmd.entity()
        dump_entity(ent2)
        cmd.destruct(ent2)
        cmd |> set_singleton(Position(value=HMM_V3(1.0, 4.0, 1.0)))
    }
}
