options gen2

module flecs_singleton_query

require daslib/typemacro_boost
require daslib/class_boost
require flecs_base public

[template_structure(T)]
struct template SingletonQuery {
    @safe_when_uninitialized world : World
    @safe_when_uninitialized singleton_holder : Entity

    [class_method]
    def static const get() : T& {
        unsafe {
            let ptr = get_component(self.world, self.singleton_holder, ecs_component(self.world, type<T>))
            return *reinterpret<T?>(ptr)
        }
    }
}
