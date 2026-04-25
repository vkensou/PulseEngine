options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module flecs_systems private

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

[function_macro(name="system")]
class FlecsSystemMacro : AstFunctionAnnotation {
    def override apply(var func : FunctionPtr; var group : ModuleGroup; args : AnnotationArgumentList; var errors : das_string) : bool {
        let phase_field = resolve_phase_field(args, errors)
        if (phase_field == "") {
            return false
        }

        if (is_run_mode(func)) {
            return add_register_helper(func, string(func.name), phase_field)
        }

        return generate_functions(func, phase_field)
    }

    def generate_functions(var func : FunctionPtr; phase_field : string)  : bool {
        let wrapper_name = "{func.name}`system_wrapper"

        var inscope wrapper_fn_batch_exprs : array<ExpressionPtr>
        var inscope wrapper_fn_call_args : array<ExpressionPtr>
        var inscope wrapper_fn_body : array<ExpressionPtr>
        let register_fn_name = "{func.name}`register"
        var inscope register_fn_body_exprs : array<ExpressionPtr>
        let desc_name = "__system_desc"
        register_fn_body_exprs |> emplace_new <| qmacro_expr(${ var $i(desc_name) = system_desc($v(string(func.name)), ctx.$f(phase_field)); })

        var term_index = 0
        for (arg, arg_index in func.arguments, count()) {
            print("{describe(arg._type)}\n")
            if (is_entity_type(arg._type)) {
                wrapper_fn_call_args |> emplace_new <| qmacro(iter_entity(iter, i))
                continue
            }

            if (is_command_buffer_type(arg._type)) {
                wrapper_fn_body |> emplace_new <| qmacro_expr(${ var cmd = CommandBuffer(world = get_world(iter)); })
                wrapper_fn_call_args |> emplace_new <| qmacro(cmd)
                continue
            }

            if (is_res_type(arg._type)) {
                // var res_ptr = ecs_get_mut_res(ctx.world, type<Position>)
                // typedef PositionRes = $Res<Position>
                // var res = PositionRes(value_ptr = res_ptr)
                // wrapper_fn_body |> emplace_new <| qmacro_expr(${ var res_ptr = ecs_get_mut_res(ctx.world, type<Position>); })
                // wrapper_fn_body |> emplace_new <| qmacro_expr(${ var res = $e(arg._type)(value_ptr = res_ptr); })
                // wrapper_fn_call_args |> emplace_new <| qmacro(res)
                continue
            }

            if (true) {
                let term_fn = is_inout_argument(arg._type) ? "system_term_inout" : "system_term_in"
                var inscope component_type <- make_component_type(arg._type)

                let field_name = "__field_{arg_index}"
                var inscope pointer_type <- make_pointer_type(component_type)
                wrapper_fn_batch_exprs |> emplace_new <| qmacro_expr(${ var $i(field_name) = reinterpret<$t(pointer_type)>(iter_field(iter, typeinfo sizeof(type<$t(component_type)>), $v(term_index))); })
                wrapper_fn_call_args |> emplace_new <| qmacro($i(field_name)[i])

                register_fn_body_exprs |> emplace_new <| qmacro_expr(${ $i(desc_name).terms[$v(term_index)] = $c(term_fn)(ctx.world, type<$t(component_type)>); })

                term_index += 1
            }
        }

        wrapper_fn_batch_exprs |> emplace_new <| qmacro_block() {
            for (i in 0..iter.count) {
                $c("_::{func.name}")($a(wrapper_fn_call_args))
            }
        }

        wrapper_fn_body |> emplace_new <| qmacro_block() {
            unsafe {
                while (iter_next(iter)) {
                    $b(wrapper_fn_batch_exprs)
                }
            }
        }

        var inscope wrapper_fn <- qmacro_function(wrapper_name) $(var iter : Iter?) : void {
            $b(wrapper_fn_body)
        }
        wrapper_fn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction

        let add_ok = compiling_module() |> add_function(wrapper_fn)

        register_fn_body_exprs |> emplace_new <| qmacro(register_system_from_desc(ctx.world, $i(desc_name), @@$c(wrapper_name)))
        var inscope register_fn <- qmacro_function(register_fn_name) $(ctx : ModuleContext) : void {
            unsafe {
                $b(register_fn_body_exprs)
            }
        }
        register_fn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        let add_ok2 = compiling_module() |> add_function(register_fn)

        return add_ok && add_ok2
    }

    def resolve_phase_field(args : AnnotationArgumentList; var errors : das_string) {
        let phase = find_arg(args, "phase")
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

    def is_run_iter_signature(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return type_name == "flecs::Iter? -const" || type_name == "Iter? -const"
    }

    def is_entity_type(arg_type : TypeDeclPtr) : bool {
        var inscope base_type <- make_component_type(arg_type)
        let type_name = string(describe(base_type))
        return type_name == "flecs::Entity" || type_name == "Entity"
    }

    def is_command_buffer_type(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return type_name == "flecs_boost::CommandBuffer const" || type_name == "flecs_boost::CommandBuffer" || type_name == "flecs_boost::CommandBuffer -const"
    }

    def is_res_type(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return type_name == "flecs_boost::res" || type_name == "flecs_boost::Res" || type_name == "flecs_boost::res -const"
    }

    def is_iter_type(arg_type : TypeDeclPtr) : bool {
        if (is_run_iter_signature(arg_type)) {
            return true
        }
        var inscope base_type <- make_component_type(arg_type)
        let type_name = string(describe(base_type))
        return type_name == "flecs::Iter?" || type_name == "Iter?"
    }

    def is_inout_argument(arg_type : TypeDeclPtr) : bool {
        return arg_type.isRefType && !arg_type.flags.constant
    }

    def make_component_type(arg_type : TypeDeclPtr) : TypeDeclPtr {
        var inscope component_type <- clone_type(arg_type)
        component_type.flags &= ~TypeDeclFlags.constant
        component_type.flags &= ~TypeDeclFlags.ref
        return <- component_type
    }

    def make_pointer_type(component_type : TypeDeclPtr) : TypeDeclPtr {
        return <- new TypeDecl(at = component_type.at,
            baseType = Type.tPointer,
            firstType <- clone_type(component_type)
        )
    }

    def add_register_helper(func : FunctionPtr; callback_name : string; phase_field : string) : bool {
        let helper_name = "{func.name}`register"
        let desc_name = "__system_desc"
        var inscope body_exprs : array<ExpressionPtr>
        body_exprs |> emplace_new <| qmacro_expr(${ var $i(desc_name) = system_desc($v(string(func.name)), ctx.$f(phase_field)); })

        if (!is_run_mode(func)) {
            // register_system copies desc.terms until the first zeroed slot, so generated terms must stay dense.
            var term_index = 0
            for (arg in func.arguments) {
                if (is_entity_type(arg._type)) {
                    continue
                }

                if (true) {
                    var inscope component_type <- make_component_type(arg._type)
                    let term_fn = is_inout_argument(arg._type) ? "system_term_inout" : "system_term_in"
                    body_exprs |> emplace_new <| qmacro_expr(${ $i(desc_name).terms[$v(term_index)] = $c(term_fn)(ctx.world, type<$t(component_type)>); })
                    term_index += 1
                }
            }
        }

        body_exprs |> emplace_new <| qmacro(register_system_from_desc(ctx.world, $i(desc_name), @@$c(callback_name)))

        var inscope helper_fn <- qmacro_function(helper_name) $(ctx : ModuleContext) : void {
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
    def override visit(prog : ProgramPtr; mod : Module?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        macro_verify(length(expr.arguments) == 2, prog, expr.at,
            "register_system requires 2 argument")
        macro_verify(expr.arguments[1] is ExprAddr, prog, expr.at,
            "third argument must an addr of function, not {expr.arguments[1].__rtti}")

        assume ctx = expr.arguments[0]        
        assume addrOfFunc = expr.arguments[1] as ExprAddr
        assume target = addrOfFunc.target
        let registerfunc = "{target}`register"
        return <- qmacro($c(registerfunc)($e(ctx)))
    }
}