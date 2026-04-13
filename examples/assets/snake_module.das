options gen2

require flecs
require pulse_ecs

[export]
def system1(var position:Position) {
    // print("system from dascript\n")
}

[export]
def importModule(var ctx:ModuleContext) {
    print("importModule(ModuleContext) from dascript\n")
    dump_world(ctx.world)
    var ent = create_entity(ctx.world)
    dump_entity(ent)
    dump_world(ctx.world)

    var query = build_query(ctx.world, "Position")
    var iter = query_iter(query)
    while (query_next(iter)) {
        print("query iter:{iter.count}\n")
        unsafe {
            var positions = reinterpret<Position?>(iter_field(iter, typeinfo sizeof(type<Position>), 0))
            for (i in 0..iter.count) {
                print("{i}\n")
                print("{positions[i].value.X} {positions[i].value.Y} {positions[i].value.Z}\n")
            }
        }
    }
}
