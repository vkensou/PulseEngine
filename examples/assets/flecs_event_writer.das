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

    // [class_method]
    // def static const broadcast(payload : EVENT) {
    //     broadcast_with_payload(world, event_id, unsafe(addr(payload)))
    // }
}
