options gen2

module flecs_boost

require flecs public
require flecs_base public
require flecs_systems public
require flecs_singleton_query public
require flecs_res public
require flecs_event_writer public
require flecs_event_reader public
require flecs_query public
require daslib/class_boost

typedef Entity = uint64

def system_desc(name : string; dependsOn : Entity; immediate : bool = false) {
    return SystemDesc(name = name, dependsOn = dependsOn, immediate = immediate)
}

def system_term_in(var world : World; t : type<auto(T)>) {
    return ecs_term_t(id = ecs_component(world, type<T>), inout = int16(1)) // EcsIn
}

def system_term_inout(var world : World; t : type<auto(T)>) {
    return ecs_term_t(id = ecs_component(world, type<T>), inout = int16(0)) // EcsInOutDefault
}

struct CommandBuffer {
    @safe_when_uninitialized world : World
}

def entity(self: CommandBuffer) {
    return ecs_entity_create(self.world)
}

def destruct(self: CommandBuffer; entity: Entity) {
    ecs_entity_destruct(self.world, entity)
}

def set_singleton(self: CommandBuffer; component: auto(T)) {
    ecs_entity_set(self.world, get_single_holder(self.world), component)
}

def set_entity_component(self: CommandBuffer; entity: Entity; component: auto(T)) {
    ecs_entity_set(self.world, entity, component)
}

def private ecs_entity_create(var world: World) {
    return create_entity(world)
}

def private ecs_entity_destruct(var world: World, entity: Entity) {
    destruct_entity(world, entity)
}

def ecs_observe(var world: World, callback: function<(var iter: Iter?):void>, eventType: type<auto(EventType)>) {
    observe(world, callback, ecs_component(world, eventType), "pulse.EventTag")
}

def ecs_query_entity_set(var query: auto(Query), entity: Entity, component: auto(T)) {
    unsafe {
        let p = addr(component)
        set_component_from_query(query.query_, entity, ecs_component(query.world, component), typeinfo sizeof(type<T>), p)
    }
}

def ecs_query_entity_get(var query: auto(Query), entity: Entity, component: type<auto(T)>) {
    unsafe {
        let p = get_component_from_query(query.query_, entity, ecs_component(query.world, type<T>))
        return reinterpret<T?>(p)
    }
}
