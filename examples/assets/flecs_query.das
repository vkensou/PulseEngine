options gen2

module flecs_query

require daslib/typemacro_boost
require daslib/class_boost
require flecs_base

[template_structure(T1)]
struct template Query1 {
    query_: ecs_query_t?
}

[template_structure(T1, T2)]
struct template Query2 {
    query_: ecs_query_t?
}

def query(var world: World; t: type<auto(T1)>) {
    var desc = ecs_query_desc_t();
    let inout = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    print("query: {typeinfo fulltypename(type<T1>)} {inout}\n")
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query1 = $Query1<T1>
    return _Query1(query_ = query_)
}
