options gen2

require flecs_boost
require pulse_ecs
require res_define

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

// [system(phase="update")]
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

def move_probe_wrapper(var iter: Iter?) {
    while (iter_next(iter)) {
        for (i in 0..iter.count) {
            unsafe {
                var entity = iter_entity(iter, i)
                var positions = reinterpret<Position?>(iter_field(iter, typeinfo sizeof(type<Position>), 0))
                var velocities = reinterpret<Velocity?>(iter_field(iter, typeinfo sizeof(type<Velocity>), 1))
                move_probe(entity, positions[i], velocities[i])
            }
        }
    }
}

// [system(phase="update")]
def move_probe(e: Entity; pos: Position const; var vel: Velocity) {
    move_probe_hits += 1
    vel.X += 1.0
    if (move_probe_hits == 1) {
        print("move_probe entity_id=")
        dump_entity(e)
        print("move_probe pos={pos.value.X} vel={vel.X} {vel.Y} {vel.Z}\n")
    }
}

def get(r : $Res<auto(T)>) : T {
    return r.value
}

def set(var r : $Res<auto(T)>; v : T){
    r.value = v
}

def make_res(var v : auto(T)) : $Res<T>{
    var r : $Res<T>
    r.value = v
    return r
}

def show_singleton_position(query: $SingletonQuery<Position>) {
    let value = query.get()
    print("singleton Position={value.value.X} {value.value.Y} {value.value.Z}\n")
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

    var sd1 = SystemDesc(name = "run_probe", dependsOn = ctx.updatePipeline)
    ctx.world |> register_system(sd1, @@run_probe)

    var sd2 = SystemDesc(name = "move_probe", dependsOn = ctx.updatePipeline)
    ctx.world |> register_system(sd2, @@move_probe)

    var a = 10
    var r = make_res(a)
    // var r: res(type<int>)
    r.value = 0

    print("r = {get(r)}\n")

    set(r, 42)
    r.value = 42
    print("r = {get(r)}\n")

    typedef ResInt = $Res<int?>
    var r2: ResInt = ResInt(value = unsafe(addr(a)))

    print("{r2.value}\n")
    a = 32
    print("{r2.value}\n")

    {
        var cmd = CommandBuffer(world = ctx.world)
        var ent2 = cmd.entity()
        dump_entity(ent2)
        cmd.destruct(ent2)
        cmd |> set_singleton(Position(value=make_vec3(1.0, 4.0, 1.0)))

        typedef PositionQuery = $SingletonQuery<Position>
        let q = PositionQuery(world = ctx.world, singleton_holder = get_single_holder(ctx.world))
        show_singleton_position(q)
    }
}
