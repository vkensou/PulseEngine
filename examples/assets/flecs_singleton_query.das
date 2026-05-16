options gen2

module flecs_singleton_query

require daslib/typemacro_boost
require daslib/class_boost

[template_structure(T)]
struct template SingletonQuery {
    comp_ptr : T?

    [class_method]
    def static const get() : T const& {
        unsafe {
            return *comp_ptr
        }
    }

    [class_method]
    def static get() : T& {
        unsafe {
            return *comp_ptr
        }
    }
}
