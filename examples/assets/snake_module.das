options gen2

require flecs_boost
require pulse_ecs

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
    print("res Position={value.value.X} {value.value.Y} {value.value.Z}\n")
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

def show_res(res: $Res<Position>) {
    let value = res.get()
    print("res Position={value.value.X} {value.value.Y} {value.value.Z}\n")
}

def read_event(reader: $EventReader<int>) {
    let e = reader.read()
    print("{e}\n")
}

struct Event1 {

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

    ecs_entity_set(ctx.world, ent, Position(value=make_vec3(10.0, 20.0, 30.0)))
    ecs_entity_set(ctx.world, ent, Velocity(X=1.0, Y=2.0, Z=3.0))

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

    {
        var cmd = CommandBuffer(world = ctx.world)
        var ent2 = cmd.entity()
        dump_entity(ent2)
        cmd.destruct(ent2)
        cmd |> set_singleton(Position(value=make_vec3(1.0, 4.0, 1.0)))
    }
}
