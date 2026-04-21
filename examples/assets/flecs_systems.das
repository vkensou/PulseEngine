options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module flecs_systems private

require daslib/ast_boost
require daslib/templates_boost

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

        if (!validate_each_signature(func, errors)) {
            return false
        }

        let wrapper_name = "{func.name}`generated"
        return add_each_wrapper(func, wrapper_name) && add_register_helper(func, wrapper_name, phase_field)
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

    def validate_each_signature(func : FunctionPtr; var errors : das_string) : bool {
        for (arg, arg_index in func.arguments, count()) {
            if (is_entity_type(arg._type)) {
                if (arg_index != 0 || !is_value_entity_argument(arg._type)) {
                    errors := "unsupported each-mode signature: first-pass each mode allows an optional Entity only as the first argument by value, followed by component arguments as T const or var T"
                    return false
                }
                continue
            }

            if (is_iter_type(arg._type)) {
                errors := "Iter is only supported in run mode"
                return false
            }

            if (!is_supported_component_argument(arg._type)) {
                errors := "unsupported each-mode argument {describe(arg._type)}; first-pass each mode allows an optional Entity first by value, followed by component arguments as T const or var T"
                return false
            }
        }

        return true
    }

    def is_entity_type(arg_type : TypeDeclPtr) : bool {
        var inscope base_type <- make_component_type(arg_type)
        let type_name = string(describe(base_type))
        return type_name == "flecs::Entity" || type_name == "Entity"
    }

    def is_iter_type(arg_type : TypeDeclPtr) : bool {
        if (is_run_iter_signature(arg_type)) {
            return true
        }
        var inscope base_type <- make_component_type(arg_type)
        let type_name = string(describe(base_type))
        return type_name == "flecs::Iter?" || type_name == "Iter?"
    }

    def is_value_entity_argument(arg_type : TypeDeclPtr) : bool {
        return !arg_type.isRefType && !arg_type.flags.ref
    }

    def is_supported_component_argument(arg_type : TypeDeclPtr) : bool {
        var inscope base_type <- make_component_type(arg_type)
        if (length(base_type.dim) != 0) {
            return false
        }
        return arg_type.flags.constant || is_inout_argument(arg_type)
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

    def add_each_wrapper(func : FunctionPtr; wrapper_name : string) : bool {
        var inscope batch_exprs : array<ExpressionPtr>
        var inscope call_args : array<ExpressionPtr>
        // Entity comes from iter.entities, so only component arguments advance the query term index.
        var term_index = 0

        for (arg, arg_index in func.arguments, count()) {
            if (is_entity_type(arg._type)) {
                call_args |> emplace_new <| qmacro(iter_entity(iter, i))
                continue
            }

            let field_name = "__field_{arg_index}"
            if (true) {
                var inscope component_type <- make_component_type(arg._type)
                var inscope pointer_type <- make_pointer_type(component_type)
                batch_exprs |> emplace_new <| qmacro_expr(${ var $i(field_name) = reinterpret<$t(pointer_type)>(iter_field(iter, typeinfo sizeof(type<$t(component_type)>), $v(term_index))); })
                call_args |> emplace_new <| qmacro($i(field_name)[i])
                term_index += 1
            }
        }

        batch_exprs |> emplace_new <| qmacro_block() {
            for (i in 0..iter.count) {
                $c("_::{func.name}")($a(call_args))
            }
        }

        var inscope wrapper_fn <- qmacro_function(wrapper_name) $(var iter : Iter?) : void {
            unsafe {
                while (iter_next(iter)) {
                    $b(batch_exprs)
                }
            }
        }
        wrapper_fn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        return compiling_module() |> add_function(wrapper_fn)
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

        body_exprs |> emplace_new <| qmacro(register_system(ctx.world, $i(desc_name), @@$c(callback_name)))

        var inscope helper_fn <- qmacro_function(helper_name) $(ctx : ModuleContext) : void {
            unsafe {
                $b(body_exprs)
            }
        }
        helper_fn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        return compiling_module() |> add_function(helper_fn)
    }
}
