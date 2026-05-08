options gen2

module flecs_event_writer

require daslib/typemacro_boost
require daslib/class_boost
require flecs_base public

[template_structure(EVENT)]
struct template EventWriter {
    @safe_when_uninitialized world : World
    @safe_when_uninitialized singleton_holder : Entity
    @safe_when_uninitialized event_id : uint64
    @safe_when_uninitialized event_tag : Entity

    [class_method]
    def static const broadcast() {
        broadcast(world, event_id)
    }
}

def send(var writer: $EventWriter<auto(E)>, entity: Entity, component1: type<auto(T1)>) {
    send1(writer.world, writer.event_id, entity, ecs_component(writer.world, type<T1>))
}

def send(var writer: $EventWriter<auto(E)>, entity: Entity, component1: type<auto(T1)>, component2: type<auto(T2)>) {
    send2(writer.world, writer.event_id, entity, ecs_component(writer.world, type<T1>), ecs_component(writer.world, type<T2>))
}
