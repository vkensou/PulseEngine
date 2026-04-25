options gen2
options indenting = 4
options no_unused_block_arguments = false
options no_unused_function_arguments = false
options strict_smart_pointers = true

module flecs_systems_ex private

require daslib/ast_boost
require daslib/templates_boost
require daslib/macro_boost

[call_macro(name="register_system_ex")]
class FlecsRegisterSystemMacro : AstCallMacro {
    def override visit(prog : ProgramPtr; mod : Module?; var expr : smart_ptr<ExprCallMacro>) : ExpressionPtr {
        macro_verify(length(expr.arguments) == 3, prog, expr.at,
            "register_system_ex requires 3 argument")
        macro_verify(expr.arguments[2] is ExprAddr, prog, expr.at,
            "third argument must an addr of function, not {expr.arguments[2].__rtti}")

        assume addrOfFunc = expr.arguments[2] as ExprAddr
        assume func = addrOfFunc.func

        if (length(func.arguments) == 1 && is_run_iter_signature(func.arguments[0]._type)) {
            assume world = expr.arguments[0]
            assume desc = expr.arguments[1]
            assume adfunc = expr.arguments[2]
            return <- qmacro(register_system_from_desc($e(world), $e(desc), $e(adfunc)))
        }

        macro_verify(length(func.arguments) > 0, prog, expr.at,
            "system function({func.name}) must have at least one argument")

        assume world = expr.arguments[0]
        assume desc = expr.arguments[1]
        let wrapper_name = "{func.name}`system_wrapper"

        var inscope batch_exprs : array<ExpressionPtr>
        var inscope call_args : array<ExpressionPtr>
        var inscope steps : array<ExpressionPtr>

        let term_alias = "term`{int(expr.at.line)}"
        steps |> emplace_new <| qmacro_expr(${ var $i(term_alias) = $e(desc); })

        print("compiling system:{func.name}\n")
        var term_index = 0
        for (arg, arg_index in func.arguments, count()) {
            print("{describe(arg._type)}\n")
            if (is_entity_type(arg._type)) {
                call_args |> emplace_new <| qmacro(iter_entity(iter, i))
                continue
            }

            if (is_command_buffer_type(arg._type)) {
                call_args |> emplace_new <| qmacro(cmd)
                continue
            }

            if (true) {
                let term_fn = is_inout_argument(arg._type) ? "system_term_inout" : "system_term_in"
                var inscope component_type <- make_component_type(arg._type)
                steps |> emplace_new <| qmacro_expr(${ $i(term_alias).terms[$v(term_index)] = $c(term_fn)($e(world), type<$t(component_type)>); })

                let field_name = "__field_{arg_index}"
                var inscope pointer_type <- make_pointer_type(component_type)
                batch_exprs |> emplace_new <| qmacro_expr(${ var $i(field_name) = reinterpret<$t(pointer_type)>(iter_field(iter, typeinfo sizeof(type<$t(component_type)>), $v(term_index))); })
                call_args |> emplace_new <| qmacro($i(field_name)[i])

                term_index += 1
            }
        }

        steps |> emplace_new <| qmacro(register_system_from_desc($e(world), $i(term_alias), @@$c(wrapper_name)))

        batch_exprs |> emplace_new <| qmacro_block() {
            for (i in 0..iter.count) {
                $c("_::{func.name}")($a(call_args))
            }
        }

        var inscope wrapper_fn <- qmacro_function(wrapper_name) $(var iter : Iter?) : void {
            var cmd = CommandBuffer(world = get_world(iter));
            unsafe {
                while (iter_next(iter)) {
                    $b(batch_exprs)
                }
            }
        }
        wrapper_fn.flags |= FunctionFlags.generated | FunctionFlags.privateFunction
        compiling_module() |> add_function(wrapper_fn)

        return <- qmacro_block() {
            $b(steps)
        }
    }

    def is_run_iter_signature(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return type_name == "flecs::Iter? -const" || type_name == "Iter? -const"
    }

    def is_entity_type(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return type_name == "flecs::Entity const" || type_name == "flecs::Entity"
    }

    def is_command_buffer_type(arg_type : TypeDeclPtr) : bool {
        let type_name = string(describe(arg_type))
        return type_name == "flecs_boost::CommandBuffer const" || type_name == "flecs_boost::CommandBuffer" || type_name == "flecs_boost::CommandBuffer -const"
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

    def is_inout_argument(arg_type : TypeDeclPtr) : bool {
        return arg_type.isRefType && !arg_type.flags.constant
    }
}