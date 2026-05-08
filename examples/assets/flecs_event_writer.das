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
}

def broadcast(var writer: $EventWriter<auto(E)>, event: auto(E)) {
    let ptr = typeinfo sizeof(type<E>) > 0 ? unsafe(addr(event)) : null
    broadcast_with_payload(writer.world, writer.event_id, ptr)
}

def send(var writer: $EventWriter<auto(E)>, component1: type<auto(T1)>, entity: Entity, event: auto(E)) {
    let ptr = typeinfo sizeof(type<E>) > 0 ? unsafe(addr(event)) : null
    send1_with_payload(writer.world, writer.event_id, ptr, entity, ecs_component(writer.world, type<T1>))
}
