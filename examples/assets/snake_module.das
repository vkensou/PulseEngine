options gen2

require flecs_boost
require pulse_ecs

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

def system2(var cmd: CommandBuffer; position : Position) {
    print("system2\n")
    let e2 = cmd.entity()
    dump_entity(e2)
}

def system2_wrapper(var iter: Iter?) {
        var cmd_:flecs_boost::CommandBuffer = struct<flecs_boost::CommandBuffer>(uninitialized world = get_world(iter));
        unsafe {
            while ( iter_next(iter) ) {
                    var field_1:pulse_ecs::Position? = reinterpret<pulse_ecs::Position?> iter_field(iter,12,0);
                    for ( i in interval(0,iter.count) ) {
                            _::system2(cmd_,field_1[i]);
                    }
            }

        }
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

def event_listener(var iter: Iter?) {
    print("receive event!\n")
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
    ctx.world |> register_system_ex(sd1, @@run_probe)

    var sd2 = SystemDesc(name = "move_probe", dependsOn = ctx.updatePipeline)
    ctx.world |> register_system_ex(sd2, @@move_probe)

    var sd3 = SystemDesc(name = "system2", dependsOn = ctx.initPipeline)
    ctx.world |> register_system_ex(sd3, @@system2)

    {
        var cmd = CommandBuffer(world = ctx.world)
        var ent2 = cmd.entity()
        dump_entity(ent2)
        cmd.destruct(ent2)
        cmd |> set_singleton(Position(value=make_vec3(1.0, 4.0, 1.0)))

        typedef PositionQuery = $SingletonQuery<Position>
        let q = PositionQuery(world = ctx.world, singleton_holder = get_single_holder(ctx.world))
        show_singleton_position(q)

        var res_ptr = ecs_get_mut_res(ctx.world, type<Position>)
        typedef PositionRes = $Res<Position>
        var res = PositionRes(value_ptr = res_ptr)
        show_res(res)

        typedef Event1Writer = $EventWriter<Event1>
        var eventWriter = Event1Writer(world = ctx.world, 
            singleton_holder = get_single_holder(ctx.world),
            event_id = ecs_component(ctx.world, type<Event1>))
        eventWriter.broadcast()
        //eventWriter.broadcast(default<Event1>)

        ctx.world |> ecs_observe(@@event_listener, default<Event1>, "Position")

        typedef IntReader = $EventReader<int>
        let i1 = 24
        var intReader = IntReader(value = i1)
        read_event(intReader)

        var terms : uint64[32]
        for (i in 0..32) {
            terms[i] = 0ul
        }
        terms[0] = ecs_component(ctx.world, default<Position>)
        let edesc = EventDesc(event = ecs_component(ctx.world, default<Event1>), terms = terms)
        ctx.world |> emit(edesc)
    }
}
