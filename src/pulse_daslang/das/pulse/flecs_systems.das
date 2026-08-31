options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module flecs_systems private

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost
require daslib/strings_boost
require flecs_state_machine

[function_macro(name="system")]
class FlecsSystemMacro : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        let phase_field = resolve_phase_field(args, errors)

        var state_type : TypeDeclPtr
        var state_values : array<int64>
        resolve_state_field(args, state_type, state_values, errors)
        if (errors != "") {
            return false
        }

        if (is_run_mode(func)) {
            return add_register_helper(func, string(func.name), phase_field, state_type, state_values)
        }

        return generate_functions(func, phase_field, state_type, state_values, errors)
    }

    def generate_functions(var func : FunctionPtr; phase_field : string; state_type : TypeDeclPtr; state_values : array<int64>; var errors : das_string)  : bool {
        let wrapper_name = "{func.name}`system_wrapper"

        var wrapper_fn_batch_exprs : array<ExpressionPtr>
        var wrapper_fn_call_args : array<ExpressionPtr>
        var wrapper_fn_body : array<ExpressionPtr>
        let register_fn_name = "{func.name}`register"
        var register_fn_body_exprs : array<ExpressionPtr>
        var event_register_fn_body_exprs : array<ExpressionPtr>
        var system_state_class_name = "__{func.name}_state"
        var system_state_class <- new Structure(name := system_state_class_name)
        system_state_class.flags |= StructureFlags.privateStructure
        let desc_name = "__system_desc"
        register_fn_body_exprs |> push <| qmacro_expr(${ var $i(desc_name) = system_desc($v(string(func.name)), ctx.$f(phase_field)); })
        event_register_fn_body_exprs |> push <| qmacro_expr(${ var $i(desc_name) = EventSystemDesc(); })

        wrapper_fn_body |> push <| qmacro_expr(${ var world = get_world(iter); })
        wrapper_fn_body |> push <| qmacro_expr(${ var real_world = get_real_world(iter); })
        var is_event_system = false
        var is_entity_system = false
        var term_index = 0
        for (arg, arg_index in func.arguments, count()) {
            if (is_entity_type(arg._type)) {
                wrapper_fn_call_args |> push <| qmacro(iter_entity(iter, i))
                is_entity_system = true
                continue
            }

            if (is_command_buffer_type(arg._type)) {
                wrapper_fn_body |> push <| qmacro_expr(${ var cmd = CommandBuffer(world = world); })                                                                                                                                                              
                wrapper_fn_call_args |> push <| qmacro(cmd)
                continue
            }

            if (is_app_id_type(arg._type)) {
                let app_name = "app{arg_index}"
                wrapper_fn_body |> push <| qmacro_expr(${ var $i(app_name) = pulse_get_app_from_world(world); })
                wrapper_fn_call_args |> push <| qmacro($i(app_name))
                continue
            }

            let (state_machine_result, state_machine_error) = is_state_machine_type(arg)
            if (state_machine_error != "") {
                errors := state_machine_error
                return false
            }
            if (state_machine_result) {
                let state_machine_name = "state_machine{arg_index}"
                wrapper_fn_body |> push <| qmacro_expr(${ var $i(state_machine_name) = struct<$t(arg._type)>(world = world); })
                wrapper_fn_call_args |> push <| qmacro($i(state_machine_name))
                continue
            }

            let (res_result, res_generic_type, res_error_message) = is_res_type(arg)
            if (res_error_message != "") {
                errors := res_error_message
                return false
            }
            if (res_result) {
                let res_ptr_name = "res_ptr{arg_index}"
                let res_name = "res{arg_index}"
                let is_const = arg._type.flags.constant
                if (is_const) {
                    wrapper_fn_body |> push <| qmacro_expr(${ let $i(res_ptr_name) = ecs_get_res(world, type<$t(res_generic_type)>); })
                    wrapper_fn_body |> push <| qmacro_expr(${ let $i(res_name): $t(arg._type) = struct<$t(arg._type)>(value_ptr = unsafe(reinterpret<$t(res_generic_type)?>($i(res_ptr_name)))); })
                } else {
                    wrapper_fn_body |> push <| qmacro_expr(${ var $i(res_ptr_name) = ecs_get_mut_res(world, type<$t(res_generic_type)>); })
                    wrapper_fn_body |> push <| qmacro_expr(${ var $i(res_name): $t(arg._type) = struct<$t(arg._type)>(value_ptr = $i(res_ptr_name)); })
                }
                wrapper_fn_call_args |> push <| qmacro($i(res_name))
                continue
            }

            let (single_query_result, single_query_generic_type, single_query_error_message) = is_single_query_type(arg)
            if (single_query_error_message != "") {
                errors := single_query_error_message
                return false
            }
            if (single_query_result) {
                let single_query_ptr_name = "single_query_ptr{arg_index}"
                let single_query_name = "single_query{arg_index}"
                let is_const = arg._type.flags.constant
                if (is_const) {
                    wrapper_fn_body |> push <| qmacro_expr(${ let $i(single_query_ptr_name) = ecs_entity_get(world, get_single_holder(world), type<$t(single_query_generic_type)>); })
                    wrapper_fn_body |> push <| qmacro_expr(${ let $i(single_query_name) = struct<$t(arg._type)>(comp_ptr = unsafe(reinterpret<$t(single_query_generic_type)?>($i(single_query_ptr_name)))); })
                } else {
                    wrapper_fn_body |> push <| qmacro_expr(${ var $i(single_query_ptr_name) = ecs_entity_get_mut(world, get_single_holder(world), type<$t(single_query_generic_type)>); })
                    wrapper_fn_body |> push <| qmacro_expr(${ var $i(single_query_name) = struct<$t(arg._type)>(comp_ptr = $i(single_query_ptr_name)); })
                }
                wrapper_fn_call_args |> push <| qmacro($i(single_query_name))
                continue
            }

            let (event_writer_result, event_writer_generic_type, event_writer_error_message) = is_event_writer_type(arg)
            if (event_writer_error_message != "") {
                errors := event_writer_error_message
                return false
            }
            if (event_writer_result) {
                let event_writer_name = "event_writer{arg_index}"
                wrapper_fn_body |> push <| qmacro_expr(${ var $i(event_writer_name) = struct<$t(arg._type)>(world = real_world, event_id = ecs_component(real_world, type<$t(event_writer_generic_type)>)); })
                wrapper_fn_call_args |> push <| qmacro($i(event_writer_name))
                continue
            }

            let (event_reader_result, event_reader_generic_type, event_reader_error_message) = is_event_reader_type(arg)
            if (event_reader_error_message != "") {
                errors := event_reader_error_message
                return false
            }
            if (event_reader_result) {
                is_event_system = true
                wrapper_fn_body |> push <| qmacro_expr(${ let event_reader = struct<$t(arg._type)>(value = iter.param != null ? deref(unsafe(reinterpret<$t(event_reader_generic_type)?>(iter.param))) : default<$t(event_reader_generic_type)>); })
                wrapper_fn_call_args |> push <| qmacro(event_reader)
                event_register_fn_body_exprs |> push <| qmacro_expr(${ $i(desc_name).event_id = ecs_component(ctx.world, type<$t(event_reader_generic_type)>); })
                continue
            }

            let (query_result, query_error_message) = is_query_type(arg)
            if (query_error_message != "") {
                errors := query_error_message
                return false
            }
            if (query_result) {
                let fi_name = "{arg.name}"
                var query_field = clone_type(arg._type)
                query_field.flags.constant = false
                let fi = system_state_class |> add_structure_field(fi_name, query_field, default<ExpressionPtr>)
                system_state_class.fields[fi].annotation |> add_annotation_argument("safe_when_uninitialized", true)
                var query_types : array<ExpressionPtr>
                for (i in 1..length(arg._type.dimExpr)) {
                    query_types |> push <| arg._type.dimExpr[i]
                }
                var register_query_call = make_call(arg.at, "query")
                var register_query_macro = register_query_call as ExprCallMacro
                register_query_macro.arguments |> push <| qmacro(ctx.world)
                for (qt in query_types) {
                    register_query_macro.arguments |> push <| clone_expression(qt)
                }
                register_fn_body_exprs |> push <| qmacro_expr(${system_state.$f(arg.name) = $e(register_query_call);})
                var event_query_call = clone_expression(register_query_call)
                event_register_fn_body_exprs |> push <| qmacro_expr(${system_state.$f(arg.name) = $e(event_query_call);})
                wrapper_fn_body |> push <| qmacro_expr(${let $i(fi_name) = system_state.$f(fi_name);})
                wrapper_fn_call_args |> push <| qmacro($i(fi_name))
                continue
            }

            if (true) {
                is_entity_system = true
                let is_const = arg._type.flags.constant
                let term_fn = !is_const ? "system_term_inout" : "system_term_in"
                var component_type <- make_component_type(arg._type)

                let field_name = "__field_{arg_index}"
                var pointer_type <- make_pointer_type(component_type)
                if (is_const) {
                    wrapper_fn_batch_exprs |> push <| qmacro_expr(${ let $i(field_name) = reinterpret<$t(pointer_type)>(iter_field(iter, typeinfo sizeof(type<$t(component_type)>), $v(term_index))); })
                } else {
                    wrapper_fn_batch_exprs |> push <| qmacro_expr(${ var $i(field_name) = reinterpret<$t(pointer_type)>(iter_field(iter, typeinfo sizeof(type<$t(component_type)>), $v(term_index))); })
                }
                wrapper_fn_call_args |> push <| qmacro($i(field_name)[i])

                register_fn_body_exprs |> push <| qmacro_expr(${ $i(desc_name).terms[$v(term_index)] = $c(term_fn)(ctx.world, type<$t(component_type)>); })
                event_register_fn_body_exprs |> push <| qmacro_expr(${ $i(desc_name).terms[$v(term_index)] = $c(term_fn)(ctx.world, type<$t(component_type)>); })

                term_index += 1
            }
        }

        if (is_event_system && !is_entity_system) {
            event_register_fn_body_exprs |> push <| qmacro_expr(${ $i(desc_name).terms[$v(term_index)] = system_term_in(ctx.world, type<pulse::EventTag>); })
        }

        if (!is_event_system && phase_field == "") {
            return false
        } elif (is_event_system && phase_field != "") {
            errors := "event system no need set phase_field"
            return false
        }

        if (length(system_state_class.fields) > 0) {
            compiling_module() |> add_structure(system_state_class)
            wrapper_fn_body |> push(qmacro_expr(${var system_state = ecs_entity_get_mut(world, ecs_component(world, type<$t(system_state_class)>), type<$t(system_state_class)>);}), 1)
            register_fn_body_exprs |> push(qmacro_expr(${var system_state = default<$t(system_state_class)>;}), 0)
            register_fn_body_exprs |> push <| qmacro_expr(${ecs_entity_set(ctx.world, ecs_component(ctx.world, type<$t(system_state_class)>), system_state);})
            event_register_fn_body_exprs |> push(qmacro_expr(${var system_state = default<$t(system_state_class)>;}), 0)
            event_register_fn_body_exprs |> push <| qmacro_expr(${ecs_entity_set(ctx.world, ecs_component(ctx.world, type<$t(system_state_class)>), system_state);})
        }

        if (is_entity_system && !is_event_system) {
            wrapper_fn_batch_exprs |> push <| qmacro_block() {
                for (i in 0..iter.count) {
                    $c("_::{func.name}")($a(wrapper_fn_call_args))
                }
            }
            wrapper_fn_body |> push <| qmacro_block() {
                unsafe {
                    while (iter_next(iter)) {
                        $b(wrapper_fn_batch_exprs)
                    }
                }
            }
        } elif (is_entity_system && is_event_system) {
            wrapper_fn_batch_exprs |> push <| qmacro_expr(${let i = 0;})
            wrapper_fn_batch_exprs |> push <| qmacro($c("_::{func.name}")($a(wrapper_fn_call_args)))
            wrapper_fn_body |> push <| qmacro_block() {
                unsafe {
                    $b(wrapper_fn_batch_exprs)
                }
            }
        } else {
            wrapper_fn_body |> push <| qmacro_block() {
                $c("_::{func.name}")($a(wrapper_fn_call_args))
            }
        }

        var wrapper_fn <- qmacro_function(wrapper_name) $(var iter : ecs_iter_t?) : void {
            $b(wrapper_fn_body)
        }
        wrapper_fn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction

        let add_ok = compiling_module() |> add_function(wrapper_fn)

        let has_state = length(state_values) > 0
        let system_entity_name = "__system_entity"
        register_fn_body_exprs |> push <| make_register_expr("register_system_from_desc", wrapper_name, desc_name, system_entity_name, func.at, has_state)
        if (has_state) {
            for (state_value in state_values) {
                register_fn_body_exprs |> push <| make_state_reg_expr(state_type, state_value, system_entity_name, func.at)
            }
        }
        var register_fn <- qmacro_function(register_fn_name) $(ctx : ModuleContext) : void {
            unsafe {
                $b(register_fn_body_exprs)
            }
        }
        register_fn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction

        event_register_fn_body_exprs |> push <| make_register_expr("observe_from_desc", wrapper_name, desc_name, system_entity_name, func.at, has_state)
        if (has_state) {
            for (state_value in state_values) {
                event_register_fn_body_exprs |> push <| make_state_reg_expr(state_type, state_value, system_entity_name, func.at)
            }
        }
        var event_register_fn <- qmacro_function(register_fn_name) $(ctx : ModuleContext) : void {
            $b(event_register_fn_body_exprs)
        }

        let add_ok2 = compiling_module() |> add_function(is_event_system ? event_register_fn : register_fn)

        return add_ok && add_ok2
    }

    def resolve_phase_field(args : AnnotationArgumentList; var errors : das_string) {
        let phase = find_arg(args, "phase")
        if (phase is nothing) {
            return ""
        }
        if (!(phase is tString)) {
            errors := "system phase must be one of: init, update, postUpdate, render, imgui"
            return ""
        }

        let phase_name = phase as tString
        if (phase_name == "init") return "initPipeline"
        if (phase_name == "update") return "updatePipeline"
        if (phase_name == "postUpdate") return "postUpdatePipeline"
        if (phase_name == "render") return "renderPipeline"
        if (phase_name == "imgui") return "imguiPipeline"

        errors := "unsupported system phase '{phase_name}'; expected one of: init, update, postUpdate, render, imgui"
        return ""
    }

    def is_run_mode(func : FunctionPtr) : bool {
        return length(func.arguments) == 1 && is_run_iter_signature(func.arguments[0]._type)
    }

    def resolve_state_field(args : AnnotationArgumentList; var state_type : TypeDeclPtr&; var state_values : array<int64>; var errors : das_string) : bool {
        let state_arg = find_arg(args, "state")
        if (state_arg is nothing) {
            return true
        }
        if (!(state_arg is tString)) {
            errors := "system 'state' must be a string of '|' separated state enum member names"
            return false
        }

        let state_names = split(state_arg as tString, "|")
        var state_enum = state_machine_find_enum(compiling_module())
        if (state_enum == null) {
            errors := "system uses 'state' but the module has no enum annotated with [state_machine(init=...)]"
            return false
        }

        var enum_type = new TypeDecl(baseType = Type.tEnumeration, enumType = state_enum)
        for (state_name in state_names) {
            let trimmed_name = state_name |> trim()
            if (trimmed_name == "") {
                continue
            }
            var found = false
            var found_value = 0l
            var next_value = 0l
            for (entry in state_enum.list) {
                var current_value = next_value
                if (entry.value != null && entry.value is ExprConstInt) {
                    current_value = int64((entry.value as ExprConstInt).value)
                    next_value = current_value + 1l
                } else {
                    next_value = current_value + 1l
                }
                if (entry.name == trimmed_name) {
                    found = true
                    found_value = current_value
                }
            }
            if (!found) {
                errors := "state '{trimmed_name}' not found in state machine enum {state_enum.name}"
                return false
            }
            state_values |> push(found_value)
        }

        if (length(state_values) == 0) {
            errors := "system 'state' must list at least one state enum member, e.g. state=\"A|B\""
            return false
        }

        state_type = clone_type(enum_type)
        return true
    }

    def make_register_expr(call_name: string; wrapper_name: string; desc_name: string; entity_name: string; at: LineInfo; keep_entity: bool) : ExpressionPtr {
        var call_expr = make_call(at, call_name)
        var call = call_expr as ExprCall
        call.arguments |> push <| qmacro(ctx.world)
        call.arguments |> push <| qmacro($i(desc_name))
        call.arguments |> push <| qmacro(@@$c(wrapper_name))

        if (!keep_entity) {
            return call_expr
        }

        var vlet = new ExprLet(at = at, atInit = at)
        var vv <- qmacro_variable(entity_name, type<Entity>)
        vv.init = call_expr
        vlet.variables |> emplace_new() <| vv
        return vlet
    }

    def make_state_reg_expr(state_type : TypeDeclPtr; state_value : int64; entity_name : string; at : LineInfo) : ExpressionPtr {
        var call_expr = make_call(at, "state_machine_reg_state")
        var call = call_expr as ExprCall
        call.arguments |> push <| qmacro(ctx.world)
        call.arguments |> push <| qmacro(type<$t(state_type)>)
        call.arguments |> push <| qmacro($i(entity_name))
        call.arguments |> push <| new ExprConstInt64(at = at, value = state_value)
        return call_expr
    }

    def is_run_iter_signature(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return type_name == "flecs::ecs_iter_t? -const" || type_name == "ecs_iter_t? -const"
    }

    def is_entity_type(arg_type : TypeDeclPtr) : bool {
        var base_type <- make_component_type(arg_type)
        let type_name = string(describe(base_type))
        return type_name == "uint64 aka Entity"
    }

    def is_command_buffer_type(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return type_name == "flecs_boost::CommandBuffer const" || type_name == "flecs_boost::CommandBuffer" || type_name == "flecs_boost::CommandBuffer -const"
    }

    def is_app_id_type(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return (type_name == "PulseAppId") || (type_name == "PulseAppId const") || (type_name == "PulseAppId -const") || (type_name == "pulse::PulseAppId") || (type_name == "pulse::PulseAppId const") || (type_name == "pulse::PulseAppId -const")
    }

    def is_state_machine_type(arg: VariablePtr) {
        let arg_type = arg._type
        if (arg_type.baseType != Type.typeMacro || !(arg_type.dimExpr[0] is ExprConstString)) {
            return (false, "")
        }

        let macroExpr = arg_type.dimExpr[0] as ExprConstString
        let macroName = string(macroExpr.value)

        if (macroName != "SystemStateMachine") {
            return (false, "")
        }

        if (!arg_type.flags.constant || arg_type.flags.removeConstant) {
            var cloned = clone_type(arg_type)
            cloned.flags.constant = true
            cloned.flags.removeConstant = false
            return (false, "expecting '{arg.name}: {describe(cloned)}', got '{describe(arg)}'
            system parameter of type SystemStateMachine<T> cannot be declared with 'var'; remove 'var' keyword")
        }

        if (length(arg_type.dimExpr) < 2) {
            return (false, "expecting 'SystemStateMachine<T>', got 'SystemStateMachine<>'
            SystemStateMachine<T> requires a type parameter, e.g. SystemStateMachine<SnakeGameState>")
        }

        if (!(arg_type.dimExpr[1] is ExprTypeDecl)) {
            return (false, "SystemStateMachine<T> requires a type param, but {describe(arg_type.dimExpr[1])} is not a Type")
        }

        return (true, "")
    }

    def is_res_type(arg: VariablePtr) {
        let arg_type = arg._type
        if (arg_type.baseType != Type.typeMacro || !(arg_type.dimExpr[0] is ExprConstString)) {
            return (false, default<TypeDeclPtr>, "")
        }

        let macroExpr = arg_type.dimExpr[0] as ExprConstString
        let macroName = string(macroExpr.value)

        if (macroName != ("Res")) {
            return (false, default<TypeDeclPtr>, "")
        }

        if (!arg_type.flags.constant || arg_type.flags.removeConstant) {
            var cloned = clone_type(arg_type)
            cloned.flags.constant = true
            cloned.flags.removeConstant = false
            return (false, default<TypeDeclPtr>, "expecting '{arg.name}: {describe(cloned)}', got '{describe(arg)}'
            system parameter of type Res<T> cannot be declared with 'var'; remove 'var' keyword")
        }

        if (length(arg_type.dimExpr) < 2) {
            return (false, default<TypeDeclPtr>, "expecting 'Res<T>', got 'Res<>'
            Res<T> requires a type parameter, e.g. Res<ResourceManager>")
        }

        if (!(arg_type.dimExpr[1] is ExprTypeDecl)) {
            return (false, default<TypeDeclPtr>, "Res<T> requires a type param, but {describe(arg_type.dimExpr[1])} is not a Type")
        }

        return (true, (arg_type.dimExpr[1] as ExprTypeDecl).typeexpr, "")
    }

    def is_single_query_type(arg: VariablePtr) {
        let arg_type = arg._type
        if (arg_type.baseType != Type.typeMacro || !(arg_type.dimExpr[0] is ExprConstString)) {
            return (false, default<TypeDeclPtr>, "")
        }

        let macroExpr = arg_type.dimExpr[0] as ExprConstString
        let macroName = string(macroExpr.value)

        if (macroName != "SingletonQuery") {
            return (false, default<TypeDeclPtr>, "")
        }

        if (!arg_type.flags.constant || arg_type.flags.removeConstant) {
            var cloned = clone_type(arg_type)
            cloned.flags.constant = true
            cloned.flags.removeConstant = false
            return (false, default<TypeDeclPtr>, "expecting '{arg.name}: {describe(cloned)}', got '{describe(arg)}'
            system parameter of type SingletonQuery<T> cannot be declared with 'var'; remove 'var' keyword")
        }

        if (length(arg_type.dimExpr) < 2) {
            return (false, default<TypeDeclPtr>, "expecting 'SingletonQuery<T>', got 'SingletonQuery<>'
            SingletonQuery<T> requires a type parameter, e.g. SingletonQuery<Score>")
        }

        if (!(arg_type.dimExpr[1] is ExprTypeDecl)) {
            return (false, default<TypeDeclPtr>, "SingletonQuery<T> requires a type param, but {describe(arg_type.dimExpr[1])} is not a Type")
        }

        return (true, (arg_type.dimExpr[1] as ExprTypeDecl).typeexpr, "")
    }

    def is_event_writer_type(arg: VariablePtr) {
        let arg_type = arg._type
        if (arg_type.baseType != Type.typeMacro || !(arg_type.dimExpr[0] is ExprConstString)) {
            return (false, default<TypeDeclPtr>, "")
        }

        let macroExpr = arg_type.dimExpr[0] as ExprConstString
        let macroName = string(macroExpr.value)

        if (macroName != ("EventWriter")) {
            return (false, default<TypeDeclPtr>, "")
        }

        if (!arg_type.flags.constant || arg_type.flags.removeConstant) {
            var cloned = clone_type(arg_type)
            cloned.flags.constant = true
            cloned.flags.removeConstant = false
            return (false, default<TypeDeclPtr>, "expecting '{arg.name}: {describe(cloned)}', got '{describe(arg)}'
            system parameter of type EventWriter<T> cannot be declared with 'var'; remove 'var' keyword")
        }

        if (length(arg_type.dimExpr) < 2) {
            return (false, default<TypeDeclPtr>, "expecting 'EventWriter<T>', got 'EventWriter<>'
            EventWriter<T> requires a type parameter, e.g. EventWriter<AppleEatenEvent>")
        }

        if (!(arg_type.dimExpr[1] is ExprTypeDecl)) {
            return (false, default<TypeDeclPtr>, "EventWriter<T> requires a type param, but {describe(arg_type.dimExpr[1])} is not a Type")
        }

        return (true, (arg_type.dimExpr[1] as ExprTypeDecl).typeexpr, "")
    }

    def is_event_reader_type(arg: VariablePtr) {
        let arg_type = arg._type
        if (arg_type.baseType != Type.typeMacro || !(arg_type.dimExpr[0] is ExprConstString)) {
            return (false, default<TypeDeclPtr>, "")
        }

        let macroExpr = arg_type.dimExpr[0] as ExprConstString
        let macroName = string(macroExpr.value)

        if (macroName != ("EventReader")) {
            return (false, default<TypeDeclPtr>, "")
        }

        if (!arg_type.flags.constant || arg_type.flags.removeConstant) {
            var cloned = clone_type(arg_type)
            cloned.flags.constant = true
            cloned.flags.removeConstant = false
            return (false, default<TypeDeclPtr>, "expecting '{arg.name}: {describe(cloned)}', got '{describe(arg)}'
            system parameter of type EventReader<T> cannot be declared with 'var'; remove 'var' keyword")
        }

        if (length(arg_type.dimExpr) < 2) {
            return (false, default<TypeDeclPtr>, "expecting 'EventReader<T>', got 'EventReader<>'
            EventReader<T> requires a type parameter, e.g. EventReader<AppleEatenEvent>")
        }

        if (!(arg_type.dimExpr[1] is ExprTypeDecl)) {
            return (false, default<TypeDeclPtr>, "EventReader<T> requires a type param, but {describe(arg_type.dimExpr[1])} is not a Type")
        }

        return (true, (arg_type.dimExpr[1] as ExprTypeDecl).typeexpr, "")
    }

    def is_query_type(arg: VariablePtr) {
        let arg_type = arg._type
        if (arg_type.baseType != Type.typeMacro || length(arg_type.dimExpr) < 2) {
            return (false, "")
        }

        if (!(arg_type.dimExpr[0] is ExprConstString)) {
            return (false, "")
        }

        let macroExpr = arg_type.dimExpr[0] as ExprConstString
        let macroName = string(macroExpr.value)

        if (macroName != "Query") {
            return (false, "")
        }

        if (!arg_type.flags.constant || arg_type.flags.removeConstant) {
            var cloned = clone_type(arg_type)
            cloned.flags.constant = true
            cloned.flags.removeConstant = false
            return (false, "expecting '{arg.name}: {describe(cloned)}', got '{describe(arg)}'
            system parameter of type Query<T> cannot be declared with 'var'; remove 'var' keyword")
        }

        for (i in 1..length(arg_type.dimExpr)) {
            if (!(arg_type.dimExpr[i] is ExprTypeDecl)) {
                return (false, "")
            }
        }

        return (true, "")
    }

    def make_component_type(arg_type : TypeDeclPtr) : TypeDeclPtr {
        var component_type <- clone_type(arg_type)
        component_type.flags &= ~TypeDeclFlags.constant
        component_type.flags &= ~TypeDeclFlags.ref
        return component_type
    }

    def make_pointer_type(component_type : TypeDeclPtr) : TypeDeclPtr {
        return new TypeDecl(at = component_type.at,
            baseType = Type.tPointer,
            firstType <- clone_type(component_type)
        )
    }

    def add_register_helper(func : FunctionPtr; callback_name : string; phase_field : string; state_type : TypeDeclPtr; state_values : array<int64>) : bool {
        let helper_name = "{func.name}`register"
        let desc_name = "__system_desc"
        var body_exprs : array<ExpressionPtr>
        body_exprs |> push <| qmacro_expr(${ var $i(desc_name) = system_desc($v(string(func.name)), ctx.$f(phase_field)); })

        let has_state = length(state_values) > 0
        let system_entity_name = "__system_entity"
        body_exprs |> push <| make_register_expr("register_system_from_desc", callback_name, desc_name, system_entity_name, func.at, has_state)
        if (has_state) {
            for (state_value in state_values) {
                body_exprs |> push <| make_state_reg_expr(state_type, state_value, system_entity_name, func.at)
            }
        }

        var helper_fn <- qmacro_function(helper_name) $(ctx : ModuleContext) : void {
            unsafe {
                $b(body_exprs)
            }
        }
        helper_fn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        return compiling_module() |> add_function(helper_fn)
    }
}

[call_macro(name="register_system")]
class FlecsRegisterSystemMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var expr : ExprCallMacro?) : ExpressionPtr {
        macro_verify(length(expr.arguments) == 2, prog, expr.at,
            "register_system requires 2 argument")
        macro_verify(expr.arguments[1] is ExprAddr, prog, expr.at,
            "third argument must an addr of function, not {expr.arguments[1].__rtti}")

        assume ctx = expr.arguments[0]        
        assume addrOfFunc = expr.arguments[1] as ExprAddr
        assume target = addrOfFunc.target
        let registerfunc = "{target}`register"
        return qmacro($c(registerfunc)($e(ctx)))
    }
}
