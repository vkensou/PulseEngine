options gen2

module flecs_res

require daslib/typemacro_boost
require daslib/class_boost

[template_structure(T)]
struct template Res {
    value_ptr : T?

    [class_method]
    def static const get() : T? {
        unsafe {
            return reinterpret<T?>(value_ptr)
        }
    }
}
