options gen2

module flecs_event_writer

require daslib/typemacro_boost
require daslib/class_boost
require flecs_base public

[template_structure(EVENT)]
struct template EventWriter {
    @safe_when_uninitialized world : World
    @safe_when_uninitialized event_id : uint64
}

def broadcast(writer: $EventWriter<auto(E)>, event: auto(E)) {
    let ptr = typeinfo sizeof(type<E>) > 0 ? unsafe(addr(event)) : null
    var desc = EventDesc(event = writer.event_id, payload = ptr)
    emit_broadcast(writer.world, desc)
}

def send(writer: $EventWriter<auto(E)>, component1: type<auto(T1)>, entity: Entity, event: auto(E)) {
    let ptr = typeinfo sizeof(type<E>) > 0 ? unsafe(addr(event)) : null
    var desc = EventDesc(event = writer.event_id, termCount = 1, entity = entity, payload = ptr)
    desc.terms[0] = ecs_component(writer.world, type<T1>)
    emit_entity(writer.world, desc)
}

def send(writer: $EventWriter<auto(E)>, component1: type<auto(T1)>, component2: type<auto(T2)>, entity: Entity, event: auto(E)) {
    let ptr = typeinfo sizeof(type<E>) > 0 ? unsafe(addr(event)) : null
    var desc = EventDesc(event = writer.event_id, termCount = 2, entity = entity, payload = ptr)
    desc.terms[0] = ecs_component(writer.world, type<T1>)
    desc.terms[1] = ecs_component(writer.world, type<T2>)
    emit_entity(writer.world, desc)
}

def send(writer: $EventWriter<auto(E)>, component1: type<auto(T1)>, component2: type<auto(T2)>, component3: type<auto(T3)>, entity: Entity, event: auto(E)) {
    let ptr = typeinfo sizeof(type<E>) > 0 ? unsafe(addr(event)) : null
    var desc = EventDesc(event = writer.event_id, termCount = 3, entity = entity, payload = ptr)
    desc.terms[0] = ecs_component(writer.world, type<T1>)
    desc.terms[1] = ecs_component(writer.world, type<T2>)
    desc.terms[2] = ecs_component(writer.world, type<T3>)
    emit_entity(writer.world, desc)
}
