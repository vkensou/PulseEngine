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
}
