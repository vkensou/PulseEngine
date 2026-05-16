options gen2

module flecs_singleton_query

require daslib/typemacro_boost
require daslib/class_boost
require flecs_base public

[template_structure(T)]
struct template SingletonQuery {
    comp_ptr : T?
    const_comp_ptr : T const?

    [class_method]
    def static const get() : T const? {
        return const_comp_ptr
    }

    [class_method]
    def static get() : T? {
        return comp_ptr
    }
}
