options gen2

module flecs_res

require daslib/typemacro_boost
require daslib/class_boost

[template_structure(T)]
struct template Res {
    value_ptr : T?
    const_value_ptr : T const?

    [class_method]
    def static const get() : T const& {
        unsafe {
            return *const_value_ptr
        }
    }

    [class_method]
    def static get() : T& {
        unsafe {
            return *value_ptr
        }
    }
}
