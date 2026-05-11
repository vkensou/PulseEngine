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

[template_structure(T1, T2, T3)]
struct template Query3 {
    query_: ecs_query_t?
}

def query(var world: World; t1: type<auto(T1)>) {
    var desc = ecs_query_desc_t();
    let inout = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query1 = $Query1<T1>
    return _Query1(query_ = query_)
}

def query(var world: World; t1: type<auto(T1)>; t2: type<auto(T2)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query2 = $Query2<T1, T2>
    return _Query2(query_ = query_)
}

def query(var world: World; t1: type<auto(T1)>; t2: type<auto(T2)>; t3: type<auto(T3)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query3 = $Query3<T1, T2, T3>
    return _Query3(query_ = query_)
}

def for_each(query: $Query1<auto(T1)>, blk : block<(entity: Entity; t1 : T1) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = reinterpret<T1 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T1>), 0);
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), field1[i])
            }
        }
    }
}

def for_each(query: $Query2<auto(T1), auto(T2)>, blk : block<(entity: Entity; t1 : T1; t2: T2) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = reinterpret<T1 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T1>), 0);
            var field2 = reinterpret<T2 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T2>), 0);
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), field1[i], field2[i])
            }
        }
    }
}

def for_each(query: $Query3<auto(T1), auto(T2), auto(T3)>, blk : block<(entity: Entity; t1 : T1; t2: T2; t3: T3) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = reinterpret<T1 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T1>), 0);
            var field2 = reinterpret<T2 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T2>), 0);
            var field3 = reinterpret<T3 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T3>), 0);
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), field1[i], field2[i], field3[i])
            }
        }
    }
}

def first(query: $Query1<auto(T1)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = reinterpret<T1 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T1>), 0);
            return (iter_entity(iter_ptr, 0), field1[0])
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("ther is no entity matched by this query!")
    return (default<Entity>, default<T1>)
}

def first(query: $Query2<auto(T1), auto(T2)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = reinterpret<T1 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T1>), 0);
            var field2 = reinterpret<T2 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T2>), 0);
            return (iter_entity(iter_ptr, 0), field1[0], field2[0])
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("ther is no entity matched by this query!")
    return (default<Entity>, default<T1>, default<T2>)
}

def first(query: $Query3<auto(T1), auto(T2), auto(T3)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = reinterpret<T1 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T1>), 0);
            var field2 = reinterpret<T2 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T2>), 0);
            var field3 = reinterpret<T3 -const?> iter_field(iter_ptr, typeinfo sizeof(type<T3>), 0);
            return (iter_entity(iter_ptr, 0), field1[0], field2[0], field3[0])
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("ther is no entity matched by this query!")
    return (default<Entity>, default<T1>, default<T2>, default<T3>)
}