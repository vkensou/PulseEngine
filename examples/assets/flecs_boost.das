options gen2

module flecs_boost

require flecs public
require flecs_base public
require flecs_systems public
require flecs_systems_ex public
require flecs_singleton_query public
require flecs_res public
require flecs_event_writer public
require flecs_event_reader public
require daslib/class_boost

typedef Entity = uint64

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

def set_singleton(self: CommandBuffer; component: auto(T)) {
    ecs_entity_set(self.world, get_single_holder(self.world), component)
}

def set_entity_component(self: CommandBuffer; entity: Entity; component: auto(T)) {
    ecs_entity_set(self.world, entity, component)
}

def ecs_observe(var world: World, callback: function<(var iter: Iter?):void>, eventType: type<auto(EventType)>) {
    observe(world, callback, ecs_component(world, eventType), "pulse.EventTag")
}