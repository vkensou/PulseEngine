options gen2

require flecs
require pulse_ecs

[export]
def system1(var position:Position) {
    print("system from dascript\n")
}

[export]
def importModule(ctx:ModuleContext) {
    print("importModule(ModuleContext) from dascript\n")
}
