options gen2

require flecs public
require daslib/static_let

def ecs_component(var world: World; t: type<auto(T)>) {
    static_let(typeinfo fulltypename(type<T>)) {
        var indices: table<uint64; uint64>
    }
    let world_key = intptr(unsafe(reinterpret<void?> world))
    var index: uint64 = 0ul
    indices |> get(world_key) $(cached_index) {
        index = cached_index
    }
    if (index == 0ul) {
        index = register_component(world, typeinfo fulltypename(type<T>), typeinfo sizeof(type<T>), typeinfo alignof(type<T>))
        indices |> insert(world_key, index)
    }
    return index
}

def ecs_entity_set(var world: World, entity: Entity, component: auto(T)) {
    unsafe {
        let p = addr(component)
        set_component(world, entity, ecs_component(world, component), typeinfo sizeof(type<T>), p)
    }
}

def system_desc(name : string; dependsOn : Entity; immediate : bool = false) {
    return make_system_desc(name, dependsOn, immediate)
}

def private init_term(id : uint64; inout : int16) {
    var term <- default<Term>
    term.id = id
    term.inout = inout
    return term
}

def system_term_in(var world : World; t : type<auto(T)>) {
    return init_term(ecs_component(world, type<T>), int16(1)) // EcsIn
}

def system_term_inout(var world : World; t : type<auto(T)>) {
    return init_term(ecs_component(world, type<T>), int16(0)) // EcsInOutDefault
}
