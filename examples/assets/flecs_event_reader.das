options gen2

module flecs_event_reader

require daslib/typemacro_boost
require daslib/class_boost

[template_structure(T)]
struct template EventReader {
    value : T

    [class_method]
    def static const read() : T& {
        unsafe {
            return value
        }
    }
}
