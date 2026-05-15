options gen2

module flecs_query

require daslib/typemacro_boost
require daslib/class_boost
require flecs_base

def private get_field(var iter_ptr; var t: type<auto(T)>; index: int) : T? {
    unsafe {
        static_if (typeinfo sizeof(type<T>) == 0) {
            return default<T?>
        } else {
            return reinterpret<T?> iter_field(iter_ptr, typeinfo sizeof(type<T>), index)
        }
    }
}

def private index_at(var fields: auto(T)?; index: int) : T? {
    unsafe {
        static_if (typeinfo sizeof(type<T>) == 0) {
            return default<T?>
        } else {
            return fields + index
        }
    }
}

[template_structure(T1)]
struct template Query1 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2)]
struct template Query2 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3)]
struct template Query3 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4)]
struct template Query4 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5)]
struct template Query5 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6)]
struct template Query6 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7)]
struct template Query7 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8)]
struct template Query8 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9)]
struct template Query9 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10)]
struct template Query10 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11)]
struct template Query11 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12)]
struct template Query12 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13)]
struct template Query13 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14)]
struct template Query14 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15)]
struct template Query15 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16)]
struct template Query16 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17)]
struct template Query17 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18)]
struct template Query18 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19)]
struct template Query19 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

[template_structure(T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20)]
struct template Query20 {
    @safe_when_uninitialized world : World
    query_: ecs_query_t?
}

def query(var world: World; var t1: type<auto(T1)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query1 = $Query1<T1>
    return _Query1(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query2 = $Query2<T1, T2>
    return _Query2(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query3 = $Query3<T1, T2, T3>
    return _Query3(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query4 = $Query4<T1, T2, T3, T4>
    return _Query4(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query5 = $Query5<T1, T2, T3, T4, T5>
    return _Query5(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query6 = $Query6<T1, T2, T3, T4, T5, T6>
    return _Query6(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query7 = $Query7<T1, T2, T3, T4, T5, T6, T7>
    return _Query7(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query8 = $Query8<T1, T2, T3, T4, T5, T6, T7, T8>
    return _Query8(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query9 = $Query9<T1, T2, T3, T4, T5, T6, T7, T8, T9>
    return _Query9(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query10 = $Query10<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10>
    return _Query10(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query11 = $Query11<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11>
    return _Query11(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query12 = $Query12<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12>
    return _Query12(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>; var t13: type<auto(T13)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    let inout13 = typeinfo is_const(type<T13>) ? int16(1) : int16(0)
    desc.terms[12] = ecs_term_t(id = ecs_component(world, type<T13>), inout = inout13)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query13 = $Query13<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13>
    return _Query13(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>; var t13: type<auto(T13)>; var t14: type<auto(T14)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    let inout13 = typeinfo is_const(type<T13>) ? int16(1) : int16(0)
    desc.terms[12] = ecs_term_t(id = ecs_component(world, type<T13>), inout = inout13)
    let inout14 = typeinfo is_const(type<T14>) ? int16(1) : int16(0)
    desc.terms[13] = ecs_term_t(id = ecs_component(world, type<T14>), inout = inout14)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query14 = $Query14<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14>
    return _Query14(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>; var t13: type<auto(T13)>; var t14: type<auto(T14)>; var t15: type<auto(T15)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    let inout13 = typeinfo is_const(type<T13>) ? int16(1) : int16(0)
    desc.terms[12] = ecs_term_t(id = ecs_component(world, type<T13>), inout = inout13)
    let inout14 = typeinfo is_const(type<T14>) ? int16(1) : int16(0)
    desc.terms[13] = ecs_term_t(id = ecs_component(world, type<T14>), inout = inout14)
    let inout15 = typeinfo is_const(type<T15>) ? int16(1) : int16(0)
    desc.terms[14] = ecs_term_t(id = ecs_component(world, type<T15>), inout = inout15)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query15 = $Query15<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15>
    return _Query15(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>; var t13: type<auto(T13)>; var t14: type<auto(T14)>; var t15: type<auto(T15)>; var t16: type<auto(T16)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    let inout13 = typeinfo is_const(type<T13>) ? int16(1) : int16(0)
    desc.terms[12] = ecs_term_t(id = ecs_component(world, type<T13>), inout = inout13)
    let inout14 = typeinfo is_const(type<T14>) ? int16(1) : int16(0)
    desc.terms[13] = ecs_term_t(id = ecs_component(world, type<T14>), inout = inout14)
    let inout15 = typeinfo is_const(type<T15>) ? int16(1) : int16(0)
    desc.terms[14] = ecs_term_t(id = ecs_component(world, type<T15>), inout = inout15)
    let inout16 = typeinfo is_const(type<T16>) ? int16(1) : int16(0)
    desc.terms[15] = ecs_term_t(id = ecs_component(world, type<T16>), inout = inout16)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query16 = $Query16<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16>
    return _Query16(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>; var t13: type<auto(T13)>; var t14: type<auto(T14)>; var t15: type<auto(T15)>; var t16: type<auto(T16)>; var t17: type<auto(T17)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    let inout13 = typeinfo is_const(type<T13>) ? int16(1) : int16(0)
    desc.terms[12] = ecs_term_t(id = ecs_component(world, type<T13>), inout = inout13)
    let inout14 = typeinfo is_const(type<T14>) ? int16(1) : int16(0)
    desc.terms[13] = ecs_term_t(id = ecs_component(world, type<T14>), inout = inout14)
    let inout15 = typeinfo is_const(type<T15>) ? int16(1) : int16(0)
    desc.terms[14] = ecs_term_t(id = ecs_component(world, type<T15>), inout = inout15)
    let inout16 = typeinfo is_const(type<T16>) ? int16(1) : int16(0)
    desc.terms[15] = ecs_term_t(id = ecs_component(world, type<T16>), inout = inout16)
    let inout17 = typeinfo is_const(type<T17>) ? int16(1) : int16(0)
    desc.terms[16] = ecs_term_t(id = ecs_component(world, type<T17>), inout = inout17)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query17 = $Query17<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17>
    return _Query17(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>; var t13: type<auto(T13)>; var t14: type<auto(T14)>; var t15: type<auto(T15)>; var t16: type<auto(T16)>; var t17: type<auto(T17)>; var t18: type<auto(T18)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    let inout13 = typeinfo is_const(type<T13>) ? int16(1) : int16(0)
    desc.terms[12] = ecs_term_t(id = ecs_component(world, type<T13>), inout = inout13)
    let inout14 = typeinfo is_const(type<T14>) ? int16(1) : int16(0)
    desc.terms[13] = ecs_term_t(id = ecs_component(world, type<T14>), inout = inout14)
    let inout15 = typeinfo is_const(type<T15>) ? int16(1) : int16(0)
    desc.terms[14] = ecs_term_t(id = ecs_component(world, type<T15>), inout = inout15)
    let inout16 = typeinfo is_const(type<T16>) ? int16(1) : int16(0)
    desc.terms[15] = ecs_term_t(id = ecs_component(world, type<T16>), inout = inout16)
    let inout17 = typeinfo is_const(type<T17>) ? int16(1) : int16(0)
    desc.terms[16] = ecs_term_t(id = ecs_component(world, type<T17>), inout = inout17)
    let inout18 = typeinfo is_const(type<T18>) ? int16(1) : int16(0)
    desc.terms[17] = ecs_term_t(id = ecs_component(world, type<T18>), inout = inout18)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query18 = $Query18<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18>
    return _Query18(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>; var t13: type<auto(T13)>; var t14: type<auto(T14)>; var t15: type<auto(T15)>; var t16: type<auto(T16)>; var t17: type<auto(T17)>; var t18: type<auto(T18)>; var t19: type<auto(T19)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    let inout13 = typeinfo is_const(type<T13>) ? int16(1) : int16(0)
    desc.terms[12] = ecs_term_t(id = ecs_component(world, type<T13>), inout = inout13)
    let inout14 = typeinfo is_const(type<T14>) ? int16(1) : int16(0)
    desc.terms[13] = ecs_term_t(id = ecs_component(world, type<T14>), inout = inout14)
    let inout15 = typeinfo is_const(type<T15>) ? int16(1) : int16(0)
    desc.terms[14] = ecs_term_t(id = ecs_component(world, type<T15>), inout = inout15)
    let inout16 = typeinfo is_const(type<T16>) ? int16(1) : int16(0)
    desc.terms[15] = ecs_term_t(id = ecs_component(world, type<T16>), inout = inout16)
    let inout17 = typeinfo is_const(type<T17>) ? int16(1) : int16(0)
    desc.terms[16] = ecs_term_t(id = ecs_component(world, type<T17>), inout = inout17)
    let inout18 = typeinfo is_const(type<T18>) ? int16(1) : int16(0)
    desc.terms[17] = ecs_term_t(id = ecs_component(world, type<T18>), inout = inout18)
    let inout19 = typeinfo is_const(type<T19>) ? int16(1) : int16(0)
    desc.terms[18] = ecs_term_t(id = ecs_component(world, type<T19>), inout = inout19)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query19 = $Query19<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19>
    return _Query19(world = world, query_ = query_)
}

def query(var world: World; var t1: type<auto(T1)>; var t2: type<auto(T2)>; var t3: type<auto(T3)>; var t4: type<auto(T4)>; var t5: type<auto(T5)>; var t6: type<auto(T6)>; var t7: type<auto(T7)>; var t8: type<auto(T8)>; var t9: type<auto(T9)>; var t10: type<auto(T10)>; var t11: type<auto(T11)>; var t12: type<auto(T12)>; var t13: type<auto(T13)>; var t14: type<auto(T14)>; var t15: type<auto(T15)>; var t16: type<auto(T16)>; var t17: type<auto(T17)>; var t18: type<auto(T18)>; var t19: type<auto(T19)>; var t20: type<auto(T20)>) {
    var desc = ecs_query_desc_t();
    let inout1 = typeinfo is_const(type<T1>) ? int16(1) : int16(0)
    desc.terms[0] = ecs_term_t(id = ecs_component(world, type<T1>), inout = inout1)
    let inout2 = typeinfo is_const(type<T2>) ? int16(1) : int16(0)
    desc.terms[1] = ecs_term_t(id = ecs_component(world, type<T2>), inout = inout2)
    let inout3 = typeinfo is_const(type<T3>) ? int16(1) : int16(0)
    desc.terms[2] = ecs_term_t(id = ecs_component(world, type<T3>), inout = inout3)
    let inout4 = typeinfo is_const(type<T4>) ? int16(1) : int16(0)
    desc.terms[3] = ecs_term_t(id = ecs_component(world, type<T4>), inout = inout4)
    let inout5 = typeinfo is_const(type<T5>) ? int16(1) : int16(0)
    desc.terms[4] = ecs_term_t(id = ecs_component(world, type<T5>), inout = inout5)
    let inout6 = typeinfo is_const(type<T6>) ? int16(1) : int16(0)
    desc.terms[5] = ecs_term_t(id = ecs_component(world, type<T6>), inout = inout6)
    let inout7 = typeinfo is_const(type<T7>) ? int16(1) : int16(0)
    desc.terms[6] = ecs_term_t(id = ecs_component(world, type<T7>), inout = inout7)
    let inout8 = typeinfo is_const(type<T8>) ? int16(1) : int16(0)
    desc.terms[7] = ecs_term_t(id = ecs_component(world, type<T8>), inout = inout8)
    let inout9 = typeinfo is_const(type<T9>) ? int16(1) : int16(0)
    desc.terms[8] = ecs_term_t(id = ecs_component(world, type<T9>), inout = inout9)
    let inout10 = typeinfo is_const(type<T10>) ? int16(1) : int16(0)
    desc.terms[9] = ecs_term_t(id = ecs_component(world, type<T10>), inout = inout10)
    let inout11 = typeinfo is_const(type<T11>) ? int16(1) : int16(0)
    desc.terms[10] = ecs_term_t(id = ecs_component(world, type<T11>), inout = inout11)
    let inout12 = typeinfo is_const(type<T12>) ? int16(1) : int16(0)
    desc.terms[11] = ecs_term_t(id = ecs_component(world, type<T12>), inout = inout12)
    let inout13 = typeinfo is_const(type<T13>) ? int16(1) : int16(0)
    desc.terms[12] = ecs_term_t(id = ecs_component(world, type<T13>), inout = inout13)
    let inout14 = typeinfo is_const(type<T14>) ? int16(1) : int16(0)
    desc.terms[13] = ecs_term_t(id = ecs_component(world, type<T14>), inout = inout14)
    let inout15 = typeinfo is_const(type<T15>) ? int16(1) : int16(0)
    desc.terms[14] = ecs_term_t(id = ecs_component(world, type<T15>), inout = inout15)
    let inout16 = typeinfo is_const(type<T16>) ? int16(1) : int16(0)
    desc.terms[15] = ecs_term_t(id = ecs_component(world, type<T16>), inout = inout16)
    let inout17 = typeinfo is_const(type<T17>) ? int16(1) : int16(0)
    desc.terms[16] = ecs_term_t(id = ecs_component(world, type<T17>), inout = inout17)
    let inout18 = typeinfo is_const(type<T18>) ? int16(1) : int16(0)
    desc.terms[17] = ecs_term_t(id = ecs_component(world, type<T18>), inout = inout18)
    let inout19 = typeinfo is_const(type<T19>) ? int16(1) : int16(0)
    desc.terms[18] = ecs_term_t(id = ecs_component(world, type<T19>), inout = inout19)
    let inout20 = typeinfo is_const(type<T20>) ? int16(1) : int16(0)
    desc.terms[19] = ecs_term_t(id = ecs_component(world, type<T20>), inout = inout20)
    var query_ = build_query_from_desc(world, desc)
    typedef _Query20 = $Query20<T1, T2, T3, T4, T5, T6, T7, T8, T9, T10, T11, T12, T13, T14, T15, T16, T17, T18, T19, T20>
    return _Query20(world = world, query_ = query_)
}

def for_each(query: $Query1<auto(T1)>, blk : block<(entity: Entity; var t1 : T1?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i))
            }
        }
    }
}

def for_each(query: $Query2<auto(T1), auto(T2)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i))
            }
        }
    }
}

def for_each(query: $Query3<auto(T1), auto(T2), auto(T3)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i))
            }
        }
    }
}

def for_each(query: $Query4<auto(T1), auto(T2), auto(T3), auto(T4)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i))
            }
        }
    }
}

def for_each(query: $Query5<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i))
            }
        }
    }
}

def for_each(query: $Query6<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i))
            }
        }
    }
}

def for_each(query: $Query7<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i))
            }
        }
    }
}

def for_each(query: $Query8<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i))
            }
        }
    }
}

def for_each(query: $Query9<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i))
            }
        }
    }
}

def for_each(query: $Query10<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i))
            }
        }
    }
}

def for_each(query: $Query11<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i))
            }
        }
    }
}

def for_each(query: $Query12<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i))
            }
        }
    }
}

def for_each(query: $Query13<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?; var t13 : T13?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            var field13 = get_field(iter_ptr, type<T13>, 12)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i), index_at(field13, i))
            }
        }
    }
}

def for_each(query: $Query14<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?; var t13 : T13?; var t14 : T14?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            var field13 = get_field(iter_ptr, type<T13>, 12)
            var field14 = get_field(iter_ptr, type<T14>, 13)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i), index_at(field13, i), index_at(field14, i))
            }
        }
    }
}

def for_each(query: $Query15<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?; var t13 : T13?; var t14 : T14?; var t15 : T15?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            var field13 = get_field(iter_ptr, type<T13>, 12)
            var field14 = get_field(iter_ptr, type<T14>, 13)
            var field15 = get_field(iter_ptr, type<T15>, 14)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i), index_at(field13, i), index_at(field14, i), index_at(field15, i))
            }
        }
    }
}

def for_each(query: $Query16<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?; var t13 : T13?; var t14 : T14?; var t15 : T15?; var t16 : T16?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            var field13 = get_field(iter_ptr, type<T13>, 12)
            var field14 = get_field(iter_ptr, type<T14>, 13)
            var field15 = get_field(iter_ptr, type<T15>, 14)
            var field16 = get_field(iter_ptr, type<T16>, 15)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i), index_at(field13, i), index_at(field14, i), index_at(field15, i), index_at(field16, i))
            }
        }
    }
}

def for_each(query: $Query17<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16), auto(T17)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?; var t13 : T13?; var t14 : T14?; var t15 : T15?; var t16 : T16?; var t17 : T17?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            var field13 = get_field(iter_ptr, type<T13>, 12)
            var field14 = get_field(iter_ptr, type<T14>, 13)
            var field15 = get_field(iter_ptr, type<T15>, 14)
            var field16 = get_field(iter_ptr, type<T16>, 15)
            var field17 = get_field(iter_ptr, type<T17>, 16)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i), index_at(field13, i), index_at(field14, i), index_at(field15, i), index_at(field16, i), index_at(field17, i))
            }
        }
    }
}

def for_each(query: $Query18<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16), auto(T17), auto(T18)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?; var t13 : T13?; var t14 : T14?; var t15 : T15?; var t16 : T16?; var t17 : T17?; var t18 : T18?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            var field13 = get_field(iter_ptr, type<T13>, 12)
            var field14 = get_field(iter_ptr, type<T14>, 13)
            var field15 = get_field(iter_ptr, type<T15>, 14)
            var field16 = get_field(iter_ptr, type<T16>, 15)
            var field17 = get_field(iter_ptr, type<T17>, 16)
            var field18 = get_field(iter_ptr, type<T18>, 17)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i), index_at(field13, i), index_at(field14, i), index_at(field15, i), index_at(field16, i), index_at(field17, i), index_at(field18, i))
            }
        }
    }
}

def for_each(query: $Query19<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16), auto(T17), auto(T18), auto(T19)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?; var t13 : T13?; var t14 : T14?; var t15 : T15?; var t16 : T16?; var t17 : T17?; var t18 : T18?; var t19 : T19?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            var field13 = get_field(iter_ptr, type<T13>, 12)
            var field14 = get_field(iter_ptr, type<T14>, 13)
            var field15 = get_field(iter_ptr, type<T15>, 14)
            var field16 = get_field(iter_ptr, type<T16>, 15)
            var field17 = get_field(iter_ptr, type<T17>, 16)
            var field18 = get_field(iter_ptr, type<T18>, 17)
            var field19 = get_field(iter_ptr, type<T19>, 18)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i), index_at(field13, i), index_at(field14, i), index_at(field15, i), index_at(field16, i), index_at(field17, i), index_at(field18, i), index_at(field19, i))
            }
        }
    }
}

def for_each(query: $Query20<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16), auto(T17), auto(T18), auto(T19), auto(T20)>, blk : block<(entity: Entity; var t1 : T1?; var t2 : T2?; var t3 : T3?; var t4 : T4?; var t5 : T5?; var t6 : T6?; var t7 : T7?; var t8 : T8?; var t9 : T9?; var t10 : T10?; var t11 : T11?; var t12 : T12?; var t13 : T13?; var t14 : T14?; var t15 : T15?; var t16 : T16?; var t17 : T17?; var t18 : T18?; var t19 : T19?; var t20 : T20?) : void>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        while (iter_next(iter_ptr)) {
            var field1 = get_field(iter_ptr, type<T1>, 0)
            var field2 = get_field(iter_ptr, type<T2>, 1)
            var field3 = get_field(iter_ptr, type<T3>, 2)
            var field4 = get_field(iter_ptr, type<T4>, 3)
            var field5 = get_field(iter_ptr, type<T5>, 4)
            var field6 = get_field(iter_ptr, type<T6>, 5)
            var field7 = get_field(iter_ptr, type<T7>, 6)
            var field8 = get_field(iter_ptr, type<T8>, 7)
            var field9 = get_field(iter_ptr, type<T9>, 8)
            var field10 = get_field(iter_ptr, type<T10>, 9)
            var field11 = get_field(iter_ptr, type<T11>, 10)
            var field12 = get_field(iter_ptr, type<T12>, 11)
            var field13 = get_field(iter_ptr, type<T13>, 12)
            var field14 = get_field(iter_ptr, type<T14>, 13)
            var field15 = get_field(iter_ptr, type<T15>, 14)
            var field16 = get_field(iter_ptr, type<T16>, 15)
            var field17 = get_field(iter_ptr, type<T17>, 16)
            var field18 = get_field(iter_ptr, type<T18>, 17)
            var field19 = get_field(iter_ptr, type<T19>, 18)
            var field20 = get_field(iter_ptr, type<T20>, 19)
            for (i in 0..iter_ptr.count) {
                blk(iter_entity(iter_ptr, i), index_at(field1, i), index_at(field2, i), index_at(field3, i), index_at(field4, i), index_at(field5, i), index_at(field6, i), index_at(field7, i), index_at(field8, i), index_at(field9, i), index_at(field10, i), index_at(field11, i), index_at(field12, i), index_at(field13, i), index_at(field14, i), index_at(field15, i), index_at(field16, i), index_at(field17, i), index_at(field18, i), index_at(field19, i), index_at(field20, i))
            }
        }
    }
}

def first(query: $Query1<auto(T1)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>)
}

def first(query: $Query2<auto(T1), auto(T2)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>)
}

def first(query: $Query3<auto(T1), auto(T2), auto(T3)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>)
}

def first(query: $Query4<auto(T1), auto(T2), auto(T3), auto(T4)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>)
}

def first(query: $Query5<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>)
}

def first(query: $Query6<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>)
}

def first(query: $Query7<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>)
}

def first(query: $Query8<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>)
}

def first(query: $Query9<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>)
}

def first(query: $Query10<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>)
}

def first(query: $Query11<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>)
}

def first(query: $Query12<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>)
}

def first(query: $Query13<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            var field13 = get_field(iter_ptr, type<T13>, 12);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0), index_at(field13, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>, default<T13?>)
}

def first(query: $Query14<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            var field13 = get_field(iter_ptr, type<T13>, 12);
            var field14 = get_field(iter_ptr, type<T14>, 13);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0), index_at(field13, 0), index_at(field14, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>, default<T13?>, default<T14?>)
}

def first(query: $Query15<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            var field13 = get_field(iter_ptr, type<T13>, 12);
            var field14 = get_field(iter_ptr, type<T14>, 13);
            var field15 = get_field(iter_ptr, type<T15>, 14);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0), index_at(field13, 0), index_at(field14, 0), index_at(field15, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>, default<T13?>, default<T14?>, default<T15?>)
}

def first(query: $Query16<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            var field13 = get_field(iter_ptr, type<T13>, 12);
            var field14 = get_field(iter_ptr, type<T14>, 13);
            var field15 = get_field(iter_ptr, type<T15>, 14);
            var field16 = get_field(iter_ptr, type<T16>, 15);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0), index_at(field13, 0), index_at(field14, 0), index_at(field15, 0), index_at(field16, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>, default<T13?>, default<T14?>, default<T15?>, default<T16?>)
}

def first(query: $Query17<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16), auto(T17)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            var field13 = get_field(iter_ptr, type<T13>, 12);
            var field14 = get_field(iter_ptr, type<T14>, 13);
            var field15 = get_field(iter_ptr, type<T15>, 14);
            var field16 = get_field(iter_ptr, type<T16>, 15);
            var field17 = get_field(iter_ptr, type<T17>, 16);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0), index_at(field13, 0), index_at(field14, 0), index_at(field15, 0), index_at(field16, 0), index_at(field17, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>, default<T13?>, default<T14?>, default<T15?>, default<T16?>, default<T17?>)
}

def first(query: $Query18<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16), auto(T17), auto(T18)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            var field13 = get_field(iter_ptr, type<T13>, 12);
            var field14 = get_field(iter_ptr, type<T14>, 13);
            var field15 = get_field(iter_ptr, type<T15>, 14);
            var field16 = get_field(iter_ptr, type<T16>, 15);
            var field17 = get_field(iter_ptr, type<T17>, 16);
            var field18 = get_field(iter_ptr, type<T18>, 17);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0), index_at(field13, 0), index_at(field14, 0), index_at(field15, 0), index_at(field16, 0), index_at(field17, 0), index_at(field18, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>, default<T13?>, default<T14?>, default<T15?>, default<T16?>, default<T17?>, default<T18?>)
}

def first(query: $Query19<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16), auto(T17), auto(T18), auto(T19)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            var field13 = get_field(iter_ptr, type<T13>, 12);
            var field14 = get_field(iter_ptr, type<T14>, 13);
            var field15 = get_field(iter_ptr, type<T15>, 14);
            var field16 = get_field(iter_ptr, type<T16>, 15);
            var field17 = get_field(iter_ptr, type<T17>, 16);
            var field18 = get_field(iter_ptr, type<T18>, 17);
            var field19 = get_field(iter_ptr, type<T19>, 18);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0), index_at(field13, 0), index_at(field14, 0), index_at(field15, 0), index_at(field16, 0), index_at(field17, 0), index_at(field18, 0), index_at(field19, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>, default<T13?>, default<T14?>, default<T15?>, default<T16?>, default<T17?>, default<T18?>, default<T19?>)
}

def first(query: $Query20<auto(T1), auto(T2), auto(T3), auto(T4), auto(T5), auto(T6), auto(T7), auto(T8), auto(T9), auto(T10), auto(T11), auto(T12), auto(T13), auto(T14), auto(T15), auto(T16), auto(T17), auto(T18), auto(T19), auto(T20)>) {
    unsafe {
        var iter = query_iter(query.query_)
        var iter_ptr = addr(iter)
        if (iter_next(iter_ptr) && iter_ptr.count > 0) {
            var field1 = get_field(iter_ptr, type<T1>, 0);
            var field2 = get_field(iter_ptr, type<T2>, 1);
            var field3 = get_field(iter_ptr, type<T3>, 2);
            var field4 = get_field(iter_ptr, type<T4>, 3);
            var field5 = get_field(iter_ptr, type<T5>, 4);
            var field6 = get_field(iter_ptr, type<T6>, 5);
            var field7 = get_field(iter_ptr, type<T7>, 6);
            var field8 = get_field(iter_ptr, type<T8>, 7);
            var field9 = get_field(iter_ptr, type<T9>, 8);
            var field10 = get_field(iter_ptr, type<T10>, 9);
            var field11 = get_field(iter_ptr, type<T11>, 10);
            var field12 = get_field(iter_ptr, type<T12>, 11);
            var field13 = get_field(iter_ptr, type<T13>, 12);
            var field14 = get_field(iter_ptr, type<T14>, 13);
            var field15 = get_field(iter_ptr, type<T15>, 14);
            var field16 = get_field(iter_ptr, type<T16>, 15);
            var field17 = get_field(iter_ptr, type<T17>, 16);
            var field18 = get_field(iter_ptr, type<T18>, 17);
            var field19 = get_field(iter_ptr, type<T19>, 18);
            var field20 = get_field(iter_ptr, type<T20>, 19);
            return (iter_entity(iter_ptr, 0), index_at(field1, 0), index_at(field2, 0), index_at(field3, 0), index_at(field4, 0), index_at(field5, 0), index_at(field6, 0), index_at(field7, 0), index_at(field8, 0), index_at(field9, 0), index_at(field10, 0), index_at(field11, 0), index_at(field12, 0), index_at(field13, 0), index_at(field14, 0), index_at(field15, 0), index_at(field16, 0), index_at(field17, 0), index_at(field18, 0), index_at(field19, 0), index_at(field20, 0))
        } finally {
            while(iter_next(iter_ptr)) {}
        }
    }
    panic("there is no entity matched by this query!")
    return (default<Entity>, default<T1?>, default<T2?>, default<T3?>, default<T4?>, default<T5?>, default<T6?>, default<T7?>, default<T8?>, default<T9?>, default<T10?>, default<T11?>, default<T12?>, default<T13?>, default<T14?>, default<T15?>, default<T16?>, default<T17?>, default<T18?>, default<T19?>, default<T20?>)
}

