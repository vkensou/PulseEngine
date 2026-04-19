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

    build_system_from_query_expr(ctx.world, "move", ctx.updatePipeline, "Position, Velocity", @@system1)

    var query = build_query(ctx.world, "Position, Velocity")
    var iter = query_iter(query)
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
}
