options gen2

module flecs_query

require daslib/typemacro_boost
require daslib/class_boost
require daslib/macro_boost
require daslib/strings_boost
require flecs_base

def get_field(var iter_ptr; var t: type<auto(T)>; index: int) : T? {
    unsafe {
        static_if (typeinfo sizeof(type<T>) == 0) {
            return default<T?>
        } else {
            return reinterpret<T?> iter_field(iter_ptr, typeinfo sizeof(type<T>), index)
        }
    }
}

def get_field_ro(var iter_ptr; var t: type<auto(T)>; index: int) : T const? {
    unsafe {
        static_if (typeinfo sizeof(type<T>) == 0) {
            return default<T const?>
        } else {
            return reinterpret<T const?> iter_field(iter_ptr, typeinfo sizeof(type<T>), index)
        }
    }
}

def index_at(var fields: auto(T)?; index: int) : T? {
    unsafe {
        static_if (typeinfo sizeof(type<T>) == 0) {
            return default<T?>
        } else {
            return fields + index
        }
    }
}

[macro_function]
def public query_component_types_from_type_macro(arg_type : TypeDeclPtr; var types : array<TypeDeclPtr>) : bool {
    types |> clear
    if (arg_type == null || arg_type.baseType != Type.typeMacro || length(arg_type.dimExpr) < 2) {
        return false
    }
    if (!(arg_type.dimExpr[0] is ExprConstString)) {
        return false
    }
    if (string((arg_type.dimExpr[0] as ExprConstString).value) != "Query") {
        return false
    }
    for (i in 1..length(arg_type.dimExpr)) {
        if (!(arg_type.dimExpr[i] is ExprTypeDecl)) {
            return false
        }
        types |> push <| clone_type((arg_type.dimExpr[i] as ExprTypeDecl).typeexpr)
    }
    return true
}

[macro_function]
def public query_component_types_from_structure(arg_type : TypeDeclPtr; var types : array<TypeDeclPtr>) : bool {
    types |> clear
    if (arg_type == null || arg_type.baseType != Type.tStructure || arg_type.structType == null) {
        return false
    }
    if (!string(arg_type.structType.name).starts_with("Query<")) {
        return false
    }
    var index = 0
    while (index < 256) {
        var alias = get_structure_alias(arg_type.structType, "T{index}")
        if (alias == null) {
            break
        }
        types |> push <| clone_type(alias)
        index += 1
    }
    return length(types) > 0
}

[macro_function]
def public query_component_types(arg_type : TypeDeclPtr; var types : array<TypeDeclPtr>) : bool {
    if (query_component_types_from_type_macro(arg_type, types)) {
        return true
    }
    return query_component_types_from_structure(arg_type, types)
}

[macro_function]
def private query_component_is_const(arg_type : TypeDeclPtr; component_index : int) : bool {
    if (arg_type == null) {
        return false
    }
    if (arg_type.baseType == Type.typeMacro && length(arg_type.dimExpr) > component_index + 1 && arg_type.dimExpr[component_index + 1] is ExprTypeDecl) {
        let component_type = (arg_type.dimExpr[component_index + 1] as ExprTypeDecl).typeexpr
        return component_type.flags.constant && !component_type.flags.removeConstant
    }
    if (arg_type.baseType == Type.tStructure && arg_type.structType != null) {
        if (get_structure_alias(arg_type.structType, "mutable{component_index}") != null) {
            return false
        }
        var alias = get_structure_alias(arg_type.structType, "T{component_index}")
        if (alias != null) {
            return true
        }
    }
    return false
}

[macro_function]
def private query_structure_name(var types : array<TypeDeclPtr>) : string {
    return build_string() $(builder) {
        builder.write("Query<")
        for (i in range(length(types))) {
            if (i > 0) {
                builder.write(",")
            }
            builder.write(describe(types[i], true, false, true))
        }
        builder.write(">")
    }
}

[typemacro_function]
def public Query(macroArgument, passArgument : TypeDeclPtr) : TypeDeclPtr {
    var types : array<TypeDeclPtr>
    if (!query_component_types_from_type_macro(macroArgument, types)) {
        macro_error(compiling_program(), macroArgument.at, "Query<T...> expects one or more type parameters")
        return null
    }

    if (passArgument != null) {
        var pass_types : array<TypeDeclPtr>
        if (!query_component_types_from_structure(passArgument, pass_types) || length(pass_types) != length(types)) {
            return null
        }
        if (string(passArgument.structType.name) != query_structure_name(types)) {
            return null
        }
        for (i in range(length(types))) {
            return null if (!pass_types[i].is_same_type(types[i], RefMatters.no, ConstMatters.yes, TemporaryMatters.yes))
        }
        var resT = clone_type(passArgument)
        resT.flags &= ~TypeDeclFlags.ref
        return <- resT
    }

    var concrete_arguments : array<TypeMacroTemplateArgument>
    for (i in range(length(types))) {
        concrete_arguments |> push <| TypeMacroTemplateArgument(
            name = "T{i}",
            argument_type = clone_type(types[i]))
    }
    if (!verify_arguments(concrete_arguments)) {
        return null
    }

    let struct_name = query_structure_name(types)
    var existing_struct = compiling_program().find_unique_structure(struct_name)
    if (existing_struct != null) {
        return new TypeDecl(baseType = Type.tStructure, structType = existing_struct, at = macroArgument.at)
    }

    var st = new Structure(name := struct_name)
    let world_field = st |> add_structure_field("world", qmacro_type(type<World>), default<ExpressionPtr>)
    st.fields[world_field].annotation |> add_annotation_argument("safe_when_uninitialized", true)
    st |> add_structure_field("query_", qmacro_type(type<ecs_query_t?>), default<ExpressionPtr>)
    for (i, arg in iter_range(concrete_arguments), concrete_arguments) {
        var arg_alias = clone_type(arg.argument_type)
        st |> add_structure_alias(arg.name, arg_alias)
        if (!arg.argument_type.flags.constant) {
            var mutable_alias = qmacro_type(type<int>)
            st |> add_structure_alias("mutable{i}", mutable_alias)
        }
    }
    compiling_module() |> add_structure(st)
    return new TypeDecl(baseType = Type.tStructure, structType = st, at = macroArgument.at)
}

[macro_function]
def private make_query_type(var expr : ExprCallMacro?) : TypeDeclPtr {
    var query_type = new TypeDecl(baseType = Type.typeMacro, at = expr.at)
    query_type.dimExpr |> push <| new ExprConstString(at = expr.at, value := "Query")
    for (i in 1..length(expr.arguments)) {
        query_type.dimExpr |> push <| clone_expression(expr.arguments[i])
    }
    return query_type
}

[macro_function]
def private make_var(name : string; at : LineInfo) : ExpressionPtr {
    var res = new ExprVar(at = at, name := name)
    res.force_generated(true)
    return res
}

[macro_function]
def private make_call_expr(name : string; at : LineInfo; var args : array<ExpressionPtr>) : ExpressionPtr {
    var call = new ExprCall(at = at, name := name)
    for (arg in args) {
        call.arguments |> push(arg)
    }
    call.force_generated(true)
    return call
}

[macro_function]
def private make_type_expr(var typ : TypeDeclPtr; at : LineInfo) : ExpressionPtr {
    var res = new ExprTypeDecl(at = at, typeexpr <- clone_type(typ))
    res.force_generated(true)
    return res
}

[macro_function]
def private make_component_type(var typ : TypeDeclPtr) : TypeDeclPtr {
    var res = clone_type(typ)
    res.flags &= ~TypeDeclFlags.constant
    res.flags &= ~TypeDeclFlags.ref
    res.flags |= TypeDeclFlags.removeConstant
    return res
}

[call_macro(name="query")]
class FlecsQueryMacro : AstCallMacro {
    def override canVisitArgument(expr : ExprCallMacro?; argIndex : int) : bool {
        return argIndex == 0
    }

    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        if (length(expr.arguments) < 2) {
            return null
        }
        for (i in 1..length(expr.arguments)) {
            if (!(expr.arguments[i] is ExprTypeDecl)) {
                return null
            }
        }

        var body : array<ExpressionPtr>
        let name_suffix = "_{expr.at.line}_{expr.at.column}"
        let world_name = "flecs_query_world{name_suffix}"
        let desc_name = "flecs_query_desc{name_suffix}"
        let query_name = "flecs_query_query{name_suffix}"
        var world_expr = clone_expression(expr.arguments[0])
        body |> push <| qmacro_expr(${ var $i(world_name) = $e(world_expr); })
        body |> push <| qmacro_expr(${ var $i(desc_name) = ecs_query_desc_t(); })
        for (i in 1..length(expr.arguments)) {
            let term_index = i - 1
            let inout_name = "flecs_query_inout{term_index}{name_suffix}"
            let component_type = (expr.arguments[i] as ExprTypeDecl).typeexpr
            var const_type = clone_type(component_type)
            var component_id_type = clone_type(component_type)
            component_id_type.flags &= ~TypeDeclFlags.constant
            component_id_type.flags &= ~TypeDeclFlags.ref
            body |> push <| qmacro_expr(${ let $i(inout_name) = typeinfo is_const(type<$t(const_type)>) ? int16(1) : int16(0); })
            body |> push <| qmacro_expr(${ $i(desc_name).terms[$v(term_index)] = ecs_term_t(id = ecs_component($i(world_name), type<$t(component_id_type)>), inout = $i(inout_name)); })
        }
        body |> push <| qmacro_expr(${ var $i(query_name) = build_query_from_desc($i(world_name), $i(desc_name)); })

        var query_type = make_query_type(expr)
        body |> push <| qmacro_expr(${ return struct<$t(query_type)>(world = $i(world_name), query_ = $i(query_name)); })
        return qmacro(invoke(${
            $b(body)
        }))
    }
}

[call_macro(name="for_each")]
class FlecsForEachMacro : AstCallMacro {
    def override canVisitArgument(expr : ExprCallMacro?; argIndex : int) : bool {
        return argIndex == 0
    }

    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        if (length(expr.arguments) == 0) {
            return null
        }

        var types : array<TypeDeclPtr>
        let query_type = expr.arguments[0]._type
        if (query_type == null) {
            return null
        }
        if (!query_component_types(query_type, types)) {
            return null
        }
        macro_verify(length(expr.arguments) == 2, prog, expr.at,
            "for_each expects query and block with entity plus component arguments")
        macro_verify(expr.arguments[1] is ExprMakeBlock, prog, expr.arguments[1].at,
            "for_each second argument must be a block, got {describe(expr.arguments[1])}")

        let block_expr = expr.arguments[1] as ExprMakeBlock
        var _block = block_expr._block as ExprBlock
        let expected_arity = 1 + length(types)
        macro_verify(_block != null && length(_block.arguments) == expected_arity, prog, expr.arguments[1].at,
            "for_each block expects {expected_arity} arguments (entity plus {length(types)} components), got {_block != null ? length(_block.arguments) : 0}")

        _block.arguments[0]._type = qmacro_type(type<Entity>)
        for (i in range(length(types))) {
            _block.arguments[i + 1]._type = new TypeDecl(baseType = Type.tPointer, firstType <- clone_type(types[i]), at = _block.arguments[i + 1].at)
        }

        let name_suffix = "_{expr.at.line}_{expr.at.column}"
        let query_name = "flecs_for_each_query{name_suffix}"
        let iter_name = "flecs_for_each_iter{name_suffix}"
        let iter_ptr_name = "flecs_for_each_iter_ptr{name_suffix}"
        let index_name = "flecs_for_each_i{name_suffix}"

        var body : array<ExpressionPtr>
        var query_arg = clone_expression(expr.arguments[0])
        body |> push <| qmacro_expr(${ let $i(query_name) = $e(query_arg); })

        var invoke_args : array<ExpressionPtr>
        var entity_args : array<ExpressionPtr>
        entity_args |> push <| make_var(iter_ptr_name, expr.at)
        entity_args |> push <| make_var(index_name, expr.at)
        invoke_args |> push <| make_call_expr("iter_entity", expr.at, entity_args)
        for (i in range(length(types))) {
            var field_type = make_component_type(types[i])
            var desired_type = clone_type(_block.arguments[i + 1]._type.firstType)
            invoke_args |> push <| qmacro(unsafe(reinterpret<$t(desired_type)?>(index_at(get_field($i(iter_ptr_name), type<$t(field_type)>, $v(i)), $i(index_name)))))
        }

        var invoke_block = clone_expression(expr.arguments[1])
        body |> push <| qmacro_block() {
            unsafe {
                var $i(iter_name) = query_iter($i(query_name).query_)
                var $i(iter_ptr_name) = addr($i(iter_name))
                while (iter_next($i(iter_ptr_name))) {
                    for ($i(index_name) in 0..$i(iter_ptr_name).count) {
                        invoke($e(invoke_block), $a(invoke_args))
                    }
                }
            }
        }
        return qmacro(invoke(${
            $b(body)
        }))
    }
}

[call_macro(name="first")]
class FlecsFirstMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        if (length(expr.arguments) == 0) {
            return null
        }

        var types : array<TypeDeclPtr>
        let query_type = expr.arguments[0]._type
        if (query_type == null) {
            return null
        }
        if (!query_component_types(query_type, types)) {
            return null
        }
        macro_verify(length(expr.arguments) == 1, prog, expr.at,
            "first(query) expects exactly one argument")

        let name_suffix = "_{expr.at.line}_{expr.at.column}"
        let query_name = "flecs_first_query{name_suffix}"
        let iter_name = "flecs_first_iter{name_suffix}"
        let iter_ptr_name = "flecs_first_iter_ptr{name_suffix}"

        var body : array<ExpressionPtr>
        var query_arg = clone_expression(expr.arguments[0])
        body |> push <| qmacro_expr(${ let $i(query_name) = $e(query_arg); })

        var matched_tuple = new ExprMakeTuple(at = expr.at)
        var entity_args : array<ExpressionPtr>
        entity_args |> push <| make_var(iter_ptr_name, expr.at)
        entity_args |> push <| quote(0)
        matched_tuple.values |> push <| make_call_expr("iter_entity", expr.at, entity_args)
        var default_tuple = new ExprMakeTuple(at = expr.at)
        default_tuple.values |> push <| qmacro(default<Entity>)
        for (i in range(length(types))) {
            let get_field_name = query_component_is_const(query_type, i) ? "get_field_ro" : "get_field"
            var field_type = make_component_type(types[i])
            var desired_type = clone_type(types[i])
            matched_tuple.values |> push <| qmacro(unsafe(reinterpret<$t(desired_type)?>(index_at($c(get_field_name)($i(iter_ptr_name), type<$t(field_type)>, $v(i)), 0))))

            var default_type = clone_type(types[i])
            default_tuple.values |> push <| qmacro(default<$t(default_type)?>)
        }

        body |> push <| qmacro_block() {
            unsafe {
                var $i(iter_name) = query_iter($i(query_name).query_)
                var $i(iter_ptr_name) = addr($i(iter_name))
                if (iter_next($i(iter_ptr_name)) && $i(iter_ptr_name).count > 0) {
                    return $e(matched_tuple)
                } finally {
                    while (iter_next($i(iter_ptr_name))) {}
                }
            }
            panic("there is no entity matched by this query!")
            return $e(default_tuple)
        }
        return qmacro(invoke(${
            $b(body)
        }))
    }
}
