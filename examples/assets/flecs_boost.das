options gen2

require flecs public
require daslib/static_let

def ecs_component(var world: World; t: type<auto(T)>) {
    static_let(typeinfo fulltypename(type<T>)) {
        var index: uint64 = 0ul
    }
    if (index == 0ul){ index = register_component(world, typeinfo stripped_typename(type<T>), typeinfo sizeof(type<T>), typeinfo alignof(type<T>)); }
    return index
}

def ecs_entity_set(var world: World, entity: Entity, component: auto(T)) {
    unsafe {
        let p = addr(component)
        set_component(world, entity, ecs_component(world, component), typeinfo sizeof(type<T>), p)
    }
}
