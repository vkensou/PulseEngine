#include "flecs.h"

#include "script.h"

#include <string.h>

typedef struct script_scan_ctx {
    ecs_world_t* world;
    prefab_script_reference_fn on_reference;
    void* ctx;
    ecs_entity_t* using_scopes;
    int32_t using_count;
    int32_t using_capacity;
} script_scan_ctx;

typedef struct script_scope_defaults {
    ecs_id_t own;
    ecs_id_t enclosing;
} script_scope_defaults;

static void push_using_entity(script_scan_ctx* ctx, ecs_entity_t entity) {
    if (ctx->using_count == ctx->using_capacity) {
        int32_t capacity = ctx->using_capacity ? ctx->using_capacity * 2 : 8;
        ctx->using_scopes = ecs_os_realloc(ctx->using_scopes, capacity * (int32_t)sizeof(ecs_entity_t));
        ctx->using_capacity = capacity;
    }
    ctx->using_scopes[ctx->using_count++] = entity;
}

static void push_using_scope(script_scan_ctx* ctx, const char* name) {
    if (!name || !name[0]) {
        return;
    }

    size_t len = strlen(name);
    if (len > 2 && name[len - 2] == '.' && name[len - 1] == '*') {
        char* path = ecs_os_malloc(len - 1);
        memcpy(path, name, len - 2);
        path[len - 2] = '\0';
        ecs_entity_t from = ecs_lookup(ctx->world, path);
        ecs_os_free(path);
        if (!from) {
            return;
        }
        ecs_iter_t it = ecs_children(ctx->world, from);
        while (ecs_children_next(&it)) {
            for (int32_t i = 0; i < it.count; ++i) {
                push_using_entity(ctx, it.entities[i]);
            }
        }
        return;
    }

    ecs_entity_t from = ecs_lookup_path_w_sep(ctx->world, 0, name, NULL, NULL, false);
    if (from) {
        push_using_entity(ctx, from);
    }
}

static ecs_entity_t resolve_script_name(script_scan_ctx* ctx, const char* name) {
    if (!name || !name[0] || name[0] == '$') {
        return 0;
    }

    ecs_entity_t result = 0;
    for (int32_t i = ctx->using_count - 1; i >= 0; --i) {
        ecs_entity_t found = ecs_lookup_path_w_sep(ctx->world, ctx->using_scopes[i], name, NULL, NULL, false);
        if (found) {
            result = found;
        }
    }
    if (!result) {
        result = ecs_lookup_path_w_sep(ctx->world, 0, name, NULL, NULL, true);
    }
    return result;
}

static ecs_entity_t resolve_tag_id(script_scan_ctx* ctx, const ecs_script_id_t* id) {
    if (id->first_expr || id->second_expr || !id->first) {
        return 0;
    }
    ecs_entity_t first = resolve_script_name(ctx, id->first);
    if (!first) {
        return 0;
    }
    if (!id->second) {
        return first;
    }
    ecs_entity_t second = resolve_script_name(ctx, id->second);
    return second ? ecs_pair(first, second) : 0;
}

static ecs_entity_t resolve_component_type(script_scan_ctx* ctx, const ecs_script_id_t* id) {
    if (id->first_expr || id->second_expr || !id->first) {
        return 0;
    }
    if (!id->second) {
        return resolve_script_name(ctx, id->first);
    }
    return resolve_script_name(ctx, id->second);
}

static uint64_t script_asset_type_of(script_scan_ctx* ctx, ecs_entity_t type) {
    if (!type) {
        return 0;
    }
    const EcsOpaque* opaque = ecs_get(ctx->world, type, EcsOpaque);
    if (!opaque || opaque->as_type != ecs_id(ecs_string_t)) {
        return 0;
    }
    return opaque->user_data;
}

static ecs_entity_t script_element_type(script_scan_ctx* ctx, ecs_entity_t type) {
    if (!type) {
        return 0;
    }
    const EcsArray* array = ecs_get(ctx->world, type, EcsArray);
    if (array) {
        return array->type;
    }
    const EcsVector* vector = ecs_get(ctx->world, type, EcsVector);
    if (vector) {
        return vector->type;
    }
    return 0;
}

static void scan_script_expr(script_scan_ctx* ctx, ecs_entity_t type, const ecs_expr_node_t* expr);

static void scan_script_initializer(script_scan_ctx* ctx, ecs_entity_t type, const ecs_expr_initializer_t* init) {
    const ecs_expr_initializer_element_t* elements = ecs_vec_first(&init->elements);
    int32_t count = ecs_vec_count(&init->elements);
    for (int32_t i = 0; i < count; ++i) {
        const ecs_expr_initializer_element_t* element = &elements[i];
        if (element->member) {
            const ecs_member_t* member = ecs_struct_get_member(ctx->world, type, element->member);
            scan_script_expr(ctx, member ? member->type : 0, element->value);
        } else {
            scan_script_expr(ctx, script_element_type(ctx, type), element->value);
        }
    }
}

static void scan_script_expr(script_scan_ctx* ctx, ecs_entity_t type, const ecs_expr_node_t* expr) {
    if (!type || !expr) {
        return;
    }

    if (expr->kind == EcsExprValue) {
        if (expr->type != ecs_id(ecs_string_t)) {
            return;
        }
        uint64_t asset_type = script_asset_type_of(ctx, type);
        if (!asset_type) {
            return;
        }
        const ecs_expr_value_node_t* value = (const ecs_expr_value_node_t*)expr;
        const char* path = *(const char**)value->ptr;
        if (path && path[0]) {
            ctx->on_reference(ctx->ctx, asset_type, path);
        }
        return;
    }

    if (expr->kind == EcsExprInitializer) {
        scan_script_initializer(ctx, type, (const ecs_expr_initializer_t*)expr);
    }
}

static void scan_script_scope(script_scan_ctx* ctx, const ecs_script_scope_t* scope, script_scope_defaults defaults);

static void scan_script_statement(script_scan_ctx* ctx, const ecs_script_node_t* node, script_scope_defaults defaults) {
    switch (node->kind) {
    case EcsAstScope:
        scan_script_scope(ctx, (const ecs_script_scope_t*)node, defaults);
        break;
    case EcsAstComponent:
    case EcsAstWithComponent: {
        const ecs_script_component_t* component = (const ecs_script_component_t*)node;
        scan_script_expr(ctx, resolve_component_type(ctx, &component->id), component->expr);
        break;
    }
    case EcsAstDefaultComponent: {
        const ecs_script_default_component_t* component = (const ecs_script_default_component_t*)node;
        scan_script_expr(ctx, defaults.enclosing, component->expr);
        break;
    }
    case EcsAstEntity: {
        const ecs_script_entity_t* entity = (const ecs_script_entity_t*)node;
        ecs_entity_t eval_kind = 0;
        if (entity->kind) {
            if (!strcmp(entity->kind, "prefab")) {
                eval_kind = EcsPrefab;
            } else if (strcmp(entity->kind, "slot")) {
                eval_kind = resolve_script_name(ctx, entity->kind);
            }
        } else {
            eval_kind = defaults.own;
        }

        ecs_id_t body_default = 0;
        if (eval_kind) {
            const EcsDefaultChildComponent* child_component = ecs_get(ctx->world, eval_kind, EcsDefaultChildComponent);
            if (child_component) {
                body_default = child_component->component;
            }
        }

        if (entity->scope) {
            script_scope_defaults body = { body_default, defaults.own };
            scan_script_scope(ctx, entity->scope, body);
        }
        break;
    }
    case EcsAstTemplate: {
        const ecs_script_template_node_t* templated = (const ecs_script_template_node_t*)node;
        if (templated->scope) {
            script_scope_defaults body = { 0, 0 };
            scan_script_scope(ctx, templated->scope, body);
        }
        break;
    }
    case EcsAstPairScope: {
        const ecs_script_pair_scope_t* pair = (const ecs_script_pair_scope_t*)node;
        if (pair->scope) {
            script_scope_defaults body = { 0, defaults.own };
            scan_script_scope(ctx, pair->scope, body);
        }
        break;
    }
    case EcsAstIf: {
        const ecs_script_if_t* conditional = (const ecs_script_if_t*)node;
        script_scope_defaults branch = { defaults.own, defaults.own };
        if (conditional->if_true) {
            scan_script_scope(ctx, conditional->if_true, branch);
        }
        if (conditional->if_false) {
            scan_script_scope(ctx, conditional->if_false, branch);
        }
        break;
    }
    case EcsAstFor: {
        const ecs_script_for_range_t* range = (const ecs_script_for_range_t*)node;
        if (range->scope) {
            script_scope_defaults branch = { defaults.own, defaults.own };
            scan_script_scope(ctx, range->scope, branch);
        }
        break;
    }
    case EcsAstWith: {
        const ecs_script_with_t* with = (const ecs_script_with_t*)node;
        if (with->expressions) {
            scan_script_scope(ctx, with->expressions, defaults);
        }
        if (with->scope) {
            ecs_id_t with_default = 0;
            if (with->expressions) {
                const ecs_script_node_t** stmts = ecs_vec_first(&with->expressions->stmts);
                int32_t count = ecs_vec_count(&with->expressions->stmts);
                if (count > 0 && stmts[count - 1]->kind == EcsAstWithTag) {
                    with_default = resolve_tag_id(ctx, &((const ecs_script_tag_t*)stmts[count - 1])->id);
                }
            }
            script_scope_defaults body = { with_default, defaults.own };
            scan_script_scope(ctx, with->scope, body);
        }
        break;
    }
    case EcsAstFunction: {
        const ecs_script_function_node_t* function = (const ecs_script_function_node_t*)node;
        if (function->body) {
            script_scope_defaults body = { 0, 0 };
            scan_script_scope(ctx, function->body, body);
        }
        break;
    }
    case EcsAstProp:
    case EcsAstConst:
    case EcsAstExportConst: {
        const ecs_script_var_node_t* var = (const ecs_script_var_node_t*)node;
        if (var->type) {
            scan_script_expr(ctx, resolve_script_name(ctx, var->type), var->expr);
        }
        break;
    }
    case EcsAstUsing:
        push_using_scope(ctx, ((const ecs_script_using_t*)node)->name);
        break;
    default:
        break;
    }
}

static void scan_script_scope(script_scan_ctx* ctx, const ecs_script_scope_t* scope, script_scope_defaults defaults) {
    if (!scope) {
        return;
    }

    int32_t prev_using_count = ctx->using_count;
    const ecs_script_node_t** stmts = ecs_vec_first(&scope->stmts);
    int32_t count = ecs_vec_count(&scope->stmts);
    for (int32_t i = 0; i < count; ++i) {
        scan_script_statement(ctx, stmts[i], defaults);
    }
    ctx->using_count = prev_using_count;
}

void prefab_script_collect_references(ecs_world_t* world, const ecs_script_t* script, prefab_script_reference_fn on_reference, void* ctx) {
    if (!world || !script || !on_reference) {
        return;
    }

    script_scan_ctx scan = {0};
    scan.world = world;
    scan.on_reference = on_reference;
    scan.ctx = ctx;

    push_using_entity(&scan, ecs_lookup(world, "flecs.meta"));

    const ecs_script_impl_t* impl = flecs_script_impl((ecs_script_t*)script);
    script_scope_defaults root_defaults = { 0, 0 };
    scan_script_scope(&scan, impl->root, root_defaults);

    ecs_os_free(scan.using_scopes);
}
