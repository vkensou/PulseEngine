options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false

module flecs_resource

require flecs public
require flecs_base public
require daslib/static_let

// PULSE_ECS_RESOURCE: registers a resource component (Sparse + Singleton)
// whose component entity also carries the value. After this call, Res<T> and
// system_state_machine<T> style lookups resolve to the same component id.
def register_resource(var world: World; t: type<auto(T)>) {
    static_let(typeinfo fulltypename(type<T>)) {
        var index: uint64 = 0ul
    }
    if (index == 0ul) {
        index = register_resource_component(world, typeinfo stripped_typename(type<T>), typeinfo sizeof(type<T>), typeinfo alignof(type<T>))
    }
    return index
}
