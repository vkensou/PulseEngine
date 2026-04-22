options gen2
options no_aot

module res_type_macro_mod

require daslib/typemacro_boost

// ----------------------------------------------------------------------
// Template definition
// ----------------------------------------------------------------------
// 这里先按“保存一个 T 类型字段”写骨架。
// 如果你的环境支持字段引用语义，可以把 value : T 改成更合适的引用形式。
// ----------------------------------------------------------------------

struct template Res {
    value : T
}

// ----------------------------------------------------------------------
// Type macro: res(type<T>)
// ----------------------------------------------------------------------

[typemacro_function]
def res(macroArgument, passArgument : TypeDeclPtr; T : TypeDeclPtr) : TypeDeclPtr {
    var inscope template_type <- typeinfo ast_typedecl(type<Res>)

    var inscope template_arguments <- [
        TypeMacroTemplateArgument(name = "T", argument_type <- clone_type(T))
    ]

    var inscope extra_template_arguments : array<tuple<string; string>>

    // generic path
    if (passArgument != null) {
        if (!is_typemacro_template_instance(passArgument, template_type, extra_template_arguments)) {
            return <- TypeDeclPtr()
        }
        if (!infer_struct_aliases(passArgument.structType, template_arguments)) {
            return <- TypeDeclPtr()
        }
        return <- infer_template_types(passArgument, template_arguments)
    }

    // concrete path
    if (!verify_arguments(template_arguments)) {
        return <- TypeDeclPtr()
    }

    var struct_name = template_structure_name(
        template_type.structType,
        template_arguments,
        extra_template_arguments
    )

    var existing_struct = compiling_program().find_unique_structure(struct_name)
    if (existing_struct != null) {
        return <- new TypeDecl(
            baseType = Type.tStructure,
            structType = existing_struct,
            at = template_type.at
        )
    }

    var inscope resType <- qmacro_template_class(struct_name, type<Res>)

    make_typemacro_template_instance(
        resType.structType,
        template_type.structType,
        extra_template_arguments
    )

    add_structure_aliases(resType.structType, template_arguments)

    return <- resType
}