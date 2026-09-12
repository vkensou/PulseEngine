/**
 * @file addons/json/scan_asset_refs.c
 * @brief Scan JSON for asset references (opaque values mapped to strings).
 */

#include "../../private_api.h"
#include "json.h"
#include "../query_dsl/query_dsl.h"

#ifdef FLECS_JSON

typedef struct ecs_json_scan_ctx_t {
    const ecs_world_t *world;
    ecs_asset_ref_action_t action;
    void *user_ctx;
    ecs_from_json_desc_t desc;
} ecs_json_scan_ctx_t;

static
const char* flecs_json_scan_skip_value(
    const char *json)
{
    char token[ECS_MAX_TOKEN_SIZE];
    ecs_json_token_t token_kind = 0;
    int32_t depth = 0;

    while ((json = flecs_json_parse(json, &token_kind, token))) {
        if (token_kind == JsonLargeString) {
            ecs_strbuf_t buf = ECS_STRBUF_INIT;
            json = flecs_json_parse_large_string(json, &buf);
            ecs_strbuf_reset(&buf);
            if (!json) {
                return NULL;
            }
            if (!depth) {
                return json;
            }
            continue;
        }

        if (token_kind == JsonObjectOpen || token_kind == JsonArrayOpen) {
            depth ++;
        } else if (token_kind == JsonObjectClose ||
                   token_kind == JsonArrayClose)
        {
            depth --;
            if (depth < 0) {
                return NULL;
            }
            if (!depth) {
                return json;
            }
        } else if (!depth) {
            return json;
        }
    }

    return NULL;
}

static
const char* flecs_json_scan_asset_string(
    ecs_json_scan_ctx_t *scan,
    const char *json)
{
    char token[ECS_MAX_TOKEN_SIZE];
    ecs_json_token_t token_kind = 0;

    json = flecs_json_parse(json, &token_kind, token);
    if (!json) {
        return NULL;
    }

    if (token_kind == JsonNull) {
        return json;
    }

    if (token_kind == JsonString) {
        scan->action(scan->user_ctx, token);
        return json;
    }

    if (token_kind == JsonLargeString) {
        ecs_strbuf_t buf = ECS_STRBUF_INIT;
        json = flecs_json_parse_large_string(json, &buf);
        if (!json) {
            ecs_strbuf_reset(&buf);
            return NULL;
        }
        char *str = ecs_strbuf_get(&buf);
        scan->action(scan->user_ctx, str);
        ecs_os_free(str);
        return json;
    }

    ecs_parser_error(scan->desc.name, scan->desc.expr,
        json - scan->desc.expr, "expected asset path string");
    return NULL;
}

static
const char* flecs_json_scan_type(
    ecs_json_scan_ctx_t *scan,
    ecs_entity_t type,
    const char *json);

static
const char* flecs_json_scan_ops(
    ecs_json_scan_ctx_t *scan,
    const ecs_meta_op_t *ops,
    int16_t op_count,
    const char *json);

static
const char* flecs_json_scan_struct(
    ecs_json_scan_ctx_t *scan,
    const ecs_meta_op_t *ops,
    int16_t op_count,
    const char *json)
{
    char token[ECS_MAX_TOKEN_SIZE];
    ecs_json_token_t token_kind = 0;

    json = flecs_json_expect(json, JsonObjectOpen, token, &scan->desc);
    if (!json) {
        return NULL;
    }

    const char *lah = flecs_json_parse(json, &token_kind, token);
    if (!lah) {
        return NULL;
    }
    if (token_kind == JsonObjectClose) {
        return lah;
    }

    while (true) {
        json = flecs_json_expect_member(json, token, &scan->desc);
        if (!json) {
            return NULL;
        }

        const ecs_meta_op_t *member_ops = NULL;
        int16_t i = 1;
        while (i < op_count - 1) {
            if (ops[i].name && !ecs_os_strcmp(ops[i].name, token)) {
                member_ops = &ops[i];
                break;
            }
            i += ops[i].op_count;
        }

        if (member_ops) {
            json = flecs_json_scan_ops(
                scan, member_ops, member_ops->op_count, json);
        } else {
            json = flecs_json_scan_skip_value(json);
        }
        if (!json) {
            return NULL;
        }

        lah = flecs_json_parse(json, &token_kind, token);
        if (!lah) {
            return NULL;
        }
        if (token_kind == JsonObjectClose) {
            return lah;
        }
        if (token_kind != JsonComma) {
            ecs_parser_error(scan->desc.name, scan->desc.expr,
                json - scan->desc.expr, "expected ',' or '}'");
            return NULL;
        }
        json = lah;
    }
}

static
const char* flecs_json_scan_collection(
    ecs_json_scan_ctx_t *scan,
    const ecs_meta_op_t *ops,
    int16_t op_count,
    const char *json)
{
    char token[ECS_MAX_TOKEN_SIZE];
    ecs_json_token_t token_kind = 0;

    json = flecs_json_expect(json, JsonArrayOpen, token, &scan->desc);
    if (!json) {
        return NULL;
    }

    const char *lah = flecs_json_parse(json, &token_kind, token);
    if (!lah) {
        return NULL;
    }
    if (token_kind == JsonArrayClose) {
        return lah;
    }

    while (true) {
        json = flecs_json_scan_ops(scan, &ops[1], op_count - 2, json);
        if (!json) {
            return NULL;
        }

        lah = flecs_json_parse(json, &token_kind, token);
        if (!lah) {
            return NULL;
        }
        if (token_kind == JsonArrayClose) {
            return lah;
        }
        if (token_kind != JsonComma) {
            ecs_parser_error(scan->desc.name, scan->desc.expr,
                json - scan->desc.expr, "expected ',' or ']'");
            return NULL;
        }
        json = lah;
    }
}

static
const char* flecs_json_scan_ops(
    ecs_json_scan_ctx_t *scan,
    const ecs_meta_op_t *ops,
    int16_t op_count,
    const char *json)
{
    const ecs_meta_op_t *op = &ops[0];

    switch (op->kind) {
    case EcsOpForward:
        return flecs_json_scan_type(scan, op->type, json);
    case EcsOpOpaqueValue:
    case EcsOpOpaqueStruct:
    case EcsOpOpaqueArray:
    case EcsOpOpaqueVector: {
        const EcsOpaque *opaque = ecs_get(scan->world, op->type, EcsOpaque);
        if (!opaque || !opaque->as_type) {
            return flecs_json_scan_skip_value(json);
        }
        if (opaque->as_type == ecs_id(ecs_string_t)) {
            return flecs_json_scan_asset_string(scan, json);
        }
        return flecs_json_scan_type(scan, opaque->as_type, json);
    }
    case EcsOpPushStruct:
        return flecs_json_scan_struct(scan, ops, op_count, json);
    case EcsOpPushArray:
    case EcsOpPushVector:
        return flecs_json_scan_collection(scan, ops, op_count, json);
    default:
        return flecs_json_scan_skip_value(json);
    }
}

static
const char* flecs_json_scan_type(
    ecs_json_scan_ctx_t *scan,
    ecs_entity_t type,
    const char *json)
{
    const EcsTypeSerializer *ser = ecs_get(
        scan->world, type, EcsTypeSerializer);
    if (!ser) {
        return flecs_json_scan_skip_value(json);
    }

    const ecs_meta_op_t *ops = ecs_vec_first(&ser->ops);
    int32_t count = ecs_vec_count(&ser->ops);
    if (!ops || count <= 0) {
        return flecs_json_scan_skip_value(json);
    }

    return flecs_json_scan_ops(scan, ops, flecs_ito(int16_t, count), json);
}

static
const char* flecs_json_scan_entity(
    ecs_json_scan_ctx_t *scan,
    const char *json);

static
const char* flecs_json_scan_results(
    ecs_json_scan_ctx_t *scan,
    const char *json)
{
    char token[ECS_MAX_TOKEN_SIZE];
    ecs_json_token_t token_kind = 0;

    json = flecs_json_expect(json, JsonArrayOpen, token, &scan->desc);
    if (!json) {
        return NULL;
    }

    const char *lah = flecs_json_parse(json, &token_kind, token);
    if (!lah) {
        return NULL;
    }
    if (token_kind != JsonArrayClose) {
        while (true) {
            json = flecs_json_scan_entity(scan, json);
            if (!json) {
                return NULL;
            }

            lah = flecs_json_parse(json, &token_kind, token);
            if (!lah) {
                return NULL;
            }
            if (token_kind == JsonArrayClose) {
                json = lah;
                break;
            }
            if (token_kind != JsonComma) {
                ecs_parser_error(scan->desc.name, scan->desc.expr,
                    json - scan->desc.expr, "expected ',' or ']'");
                return NULL;
            }
            json = lah;
        }
    } else {
        json = lah;
    }

    return json;
}

static
const char* flecs_json_scan_components(
    ecs_json_scan_ctx_t *scan,
    const char *json)
{
    char token[ECS_MAX_TOKEN_SIZE];
    char token_buffer[256];
    ecs_json_token_t token_kind = 0;

    json = flecs_json_expect(json, JsonObjectOpen, token, &scan->desc);
    if (!json) {
        return NULL;
    }

    const char *lah = flecs_json_parse(json, &token_kind, token);
    if (!lah) {
        return NULL;
    }
    if (token_kind == JsonObjectClose) {
        return lah;
    }

    while (true) {
        json = flecs_json_expect_member(json, token, &scan->desc);
        if (!json) {
            return NULL;
        }

        ecs_id_t id = 0;
        if (token[0] != '(') {
            id = ecs_lookup_path_w_sep(
                scan->world, 0, token, ".", NULL, false);
        } else {
            ecs_term_t term = {0};
            if (flecs_term_parse(
                scan->world, NULL, token, token_buffer, &term) &&
                term.first.name && term.second.name)
            {
                ecs_entity_t rel = ecs_lookup_path_w_sep(
                    scan->world, 0, term.first.name, ".", NULL, false);
                ecs_entity_t tgt = ecs_lookup_path_w_sep(
                    scan->world, 0, term.second.name, ".", NULL, false);
                if (rel && tgt) {
                    id = ecs_pair(rel, tgt);
                }
            }
        }

        const ecs_type_info_t *ti = id
            ? ecs_get_type_info(scan->world, id) : NULL;
        if (ti && ti->component) {
            json = flecs_json_scan_type(scan, ti->component, json);
        } else {
            json = flecs_json_scan_skip_value(json);
        }
        if (!json) {
            return NULL;
        }

        lah = flecs_json_parse(json, &token_kind, token);
        if (!lah) {
            return NULL;
        }
        if (token_kind == JsonObjectClose) {
            return lah;
        }
        if (token_kind != JsonComma) {
            ecs_parser_error(scan->desc.name, scan->desc.expr,
                json - scan->desc.expr, "expected ',' or '}'");
            return NULL;
        }
        json = lah;
    }
}

static
const char* flecs_json_scan_entity(
    ecs_json_scan_ctx_t *scan,
    const char *json)
{
    char token[ECS_MAX_TOKEN_SIZE];
    ecs_json_token_t token_kind = 0;

    json = flecs_json_expect(json, JsonObjectOpen, token, &scan->desc);
    if (!json) {
        return NULL;
    }

    const char *lah = flecs_json_parse(json, &token_kind, token);
    if (!lah) {
        return NULL;
    }
    if (token_kind == JsonObjectClose) {
        return lah;
    }

    while (true) {
        json = flecs_json_expect_member(json, token, &scan->desc);
        if (!json) {
            return NULL;
        }

        if (!ecs_os_strcmp(token, "components")) {
            json = flecs_json_scan_components(scan, json);
        } else if (!ecs_os_strcmp(token, "results")) {
            json = flecs_json_scan_results(scan, json);
        } else {
            json = flecs_json_scan_skip_value(json);
        }
        if (!json) {
            return NULL;
        }

        lah = flecs_json_parse(json, &token_kind, token);
        if (!lah) {
            return NULL;
        }
        if (token_kind == JsonObjectClose) {
            return lah;
        }
        if (token_kind != JsonComma) {
            ecs_parser_error(scan->desc.name, scan->desc.expr,
                json - scan->desc.expr, "expected ',' or '}'");
            return NULL;
        }
        json = lah;
    }
}

const char* ecs_asset_refs_from_json(
    const ecs_world_t *world,
    const char *json,
    ecs_asset_ref_action_t action,
    void *ctx)
{
    ecs_check(world != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(json != NULL, ECS_INVALID_PARAMETER, NULL);
    ecs_check(action != NULL, ECS_INVALID_PARAMETER, NULL);

    ecs_json_scan_ctx_t scan = {0};
    scan.world = world;
    scan.action = action;
    scan.user_ctx = ctx;
    scan.desc.name = "ecs_asset_refs_from_json";
    scan.desc.expr = json;

    return flecs_json_scan_entity(&scan, json);
error:
    return NULL;
}

#endif
