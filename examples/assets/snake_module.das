options gen2

require flecs
// Minimal daslang program used by the C++ integration tutorial.
// The C++ host compiles this file, finds the "test" function, and calls it.

def add(a:int, b:int) -> int {
    return a + b
}

def w(var world:World) {
}

[export]
def importModule(var world:World) {
    print("Hello from daslang! {add(1,2)}\n")
    dump_world(world)
    var ent = create_entity(world)
    //print("{ent}")
    dump_world(world)
    dump_entity(ent)
}
