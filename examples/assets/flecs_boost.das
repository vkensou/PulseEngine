options gen2

require flecs public
require flecs_systems public
require daslib/static_let
require daslib/class_boost

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

def system_desc(name : string; dependsOn : Entity; immediate : bool = false) {
    return SystemDesc(name = name, dependsOn = dependsOn, immediate = immediate)
}

def system_term_in(var world : World; t : type<auto(T)>) {
    return Term(id = ecs_component(world, type<T>), inout = int16(1)) // EcsIn
}

def system_term_inout(var world : World; t : type<auto(T)>) {
    return Term(id = ecs_component(world, type<T>), inout = int16(0)) // EcsInOutDefault
}

struct CommandBuffer {
    @safe_when_uninitialized world : World

    [class_method]
    def static entity() : Entity {
        return create_entity(self.world)
    }

    [class_method]
    def static destruct(entity: Entity) {
        destruct_entity(self.world, entity)
    }
}