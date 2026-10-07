#include "prefab_internal.h"

#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#define PREFAB_TLS __declspec(thread)
#else
#define PREFAB_TLS _Thread_local
#endif

namespace pulse_prefab_internal {

namespace {

PREFAB_TLS char g_last_error[1024];

const char* set_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(g_last_error, sizeof(g_last_error), format, args);
    va_end(args);
    return g_last_error;
}

struct script_reference {
    uint64_t type_id = 0;
    std::string path;
};

struct prefab_script_load_state {
    ecs_script_t* parsed = nullptr;
    std::vector<script_reference> references;
    bool syntax_checked = false;
    bool scanned = false;
    bool references_ready = false;
};

void on_script_reference(void* ctx, uint64_t type_id, const char* path) {
    auto* references = static_cast<std::vector<script_reference>*>(ctx);
    for (const script_reference& reference : *references) {
        if (reference.type_id == type_id && reference.path == path) {
            return;
        }
    }
    references->push_back(script_reference{ type_id, path });
}

const char* set_eval_error(const char* path, const ecs_script_eval_result_t& result) {
    const char* message = result.error ? result.error : "script error";
    if (result.line > 0) {
        return set_error("%s:%d:%d: %s", path, result.line, result.column, message);
    }
    return set_error("%s: %s", path, message);
}

EPulseResult ctor_prefab_script_load(void* state, const PulseAssetLoadTask* ctx) {
    (void)ctx;
    new (state) prefab_script_load_state();
    return PULSE_RESULT_OK;
}

void dtor_prefab_script_load(void* state, const PulseAssetLoadTask* ctx) {
    (void)ctx;
    auto* s = static_cast<prefab_script_load_state*>(state);
    if (s->parsed) {
        ecs_script_free(s->parsed);
    }
    s->~prefab_script_load_state();
}

void delete_script_entities(ecs_world_t* world, ecs_entity_t owner) {
    std::vector<ecs_entity_t> created;
    ecs_iter_t it = ecs_each_id(world, ecs_pair_t(EcsScript, owner));
    while (ecs_each_next(&it)) {
        for (int32_t i = 0; i < it.count; ++i) {
            created.push_back(it.entities[i]);
        }
    }
    for (ecs_entity_t entity : created) {
        if (ecs_is_alive(world, entity)) {
            ecs_delete(world, entity);
        }
    }
    if (ecs_is_alive(world, owner)) {
        ecs_delete(world, owner);
    }
}

EPulseAssetLoaderStatus step_prefab_script_load(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    auto* s = static_cast<prefab_script_load_state*>(state);

    ecs_world_t* world = pulse_app_world(ctx->app);
    if (!world) {
        *out_error = "prefab script loader: app world unavailable";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    if (!s->syntax_checked) {
        std::string source(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
        ecs_script_eval_result_t eval_result{};
        s->parsed = ecs_script_parse(world, ctx->path, source.c_str(), nullptr, &eval_result);
        if (!s->parsed) {
            *out_error = set_eval_error(ctx->path, eval_result);
            ecs_os_free(eval_result.error);
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        s->syntax_checked = true;
    }

    if (!s->scanned) {
        prefab_script_collect_references(world, s->parsed, on_script_reference, &s->references);
        s->scanned = true;
    }

    if (!s->references_ready) {
        for (const script_reference& reference : s->references) {
            PulseAssetRequest request = request_file_load(ctx->asset_system, reference.type_id, reference.path.c_str());
            if (asset_state_is_pending(pulse_asset_system_get_state(ctx->asset_system, request))) {
                return PULSE_ASSET_LOADER_STATUS_PENDING;
            }
        }
        s->references_ready = true;
    }

    ecs_entity_t script_entity = ecs_new(world);
    ecs_script_t* parsed = s->parsed;
    s->parsed = nullptr;

    bool is_deferred = ecs_is_deferred(world);
    ecs_suspend_readonly_state_t suspend_state;
    ecs_world_t* real_world = nullptr;
    if (is_deferred) {
        real_world = flecs_suspend_readonly(world, &suspend_state);
    }

    EcsScript* script_component = ecs_ensure(world, script_entity, EcsScript);
    script_component->code = ecs_os_strdup(parsed->code);
    script_component->script = parsed;

    ecs_id_t prev_with = ecs_set_with(world, ecs_pair_t(EcsScript, script_entity));
    ecs_script_eval_result_t eval_result{};
    bool failed = ecs_script_eval(parsed, nullptr, &eval_result) != 0;
    ecs_set_with(world, prev_with);

    if (failed) {
        script_component = ecs_ensure(world, script_entity, EcsScript);
        ecs_script_free(script_component->script);
        script_component->script = nullptr;
        ecs_delete_with(world, ecs_pair_t(EcsScript, script_entity));
    }

    if (is_deferred) {
        flecs_resume_readonly(real_world, &suspend_state);
    }

    if (failed) {
        *out_error = set_error("%s: %s", ctx->path, eval_result.error ? eval_result.error : "script evaluation failed");
        ecs_os_free(eval_result.error);
        delete_script_entities(world, script_entity);
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    ecs_os_free(eval_result.error);

    ecs_iter_t it = ecs_each_id(world, ecs_pair_t(EcsScript, script_entity));
    if (!ecs_each_next(&it)) {
        delete_script_entities(world, script_entity);
        *out_error = set_error("%s: script declares no entity", ctx->path);
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    auto* data = static_cast<PulsePrefabScriptData*>(ctx->out_asset);
    data->script = script_entity;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void destroy_prefab_script(void* ptr, void* user_data) {
    auto* data = static_cast<PulsePrefabScriptData*>(ptr);
    PulseAppId app = static_cast<PulseAppId>(user_data);
    ecs_world_t* world = app ? pulse_app_world(app) : nullptr;
    if (world && data->script && ecs_is_alive(world, data->script)) {
        delete_script_entities(world, data->script);
    }
    data->script = 0;
}

} // namespace

void register_prefab_script_type(PulseAssetSystemId asset_system, PulseAppId app) {
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_PREFAB_SCRIPT;
    type_desc.size = sizeof(PulsePrefabScriptData);
    type_desc.align = alignof(PulsePrefabScriptData);
    type_desc.destroy = destroy_prefab_script;
    type_desc.user_data = app;
    pulse_asset_system_register_type(asset_system, &type_desc);
}

void register_prefab_script_loader(PulseAssetSystemId asset_system) {
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_PREFAB_SCRIPT;
    ld.extensions = "flecs";
    ld.ctor = ctor_prefab_script_load;
    ld.dtor = dtor_prefab_script_load;
    ld.step = step_prefab_script_load;
    ld.loader_size = sizeof(prefab_script_load_state);
    ld.loader_align = alignof(prefab_script_load_state);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = nullptr;
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_prefab_internal

extern "C" {

PulsePrefabScriptRequest pulse_load_prefab_script(PulseAppId app, const char* filepath) {
    PulsePrefabScriptRequest result{};
    if (!app || !filepath || !filepath[0]) {
        return result;
    }

    PulseAssetRequest request = pulse_prefab_internal::request_file_load(pulse_get_asset_system(app), PULSE_TYPE_PREFAB_SCRIPT, filepath);
    if (!pulse_asset_request_is_valid(request)) {
        return result;
    }

    result.index = request.index;
    result.generation = request.generation;
    return result;
}

ecs_entity_t pulse_prefab_script_get_entity(PulseAppId app, PulsePrefabScriptHandle script, const char* name) {
    if (!app || !name || !name[0]) {
        return 0;
    }

    void* ptr = nullptr;
    if (!pulse_asset_system_borrow(pulse_get_asset_system(app), pulse_prefab_script_to_handle(script), &ptr, nullptr) || !ptr) {
        return 0;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return 0;
    }

    ecs_entity_t owner = static_cast<PulsePrefabScriptData*>(ptr)->script;
    ecs_iter_t it = ecs_each_id(world, ecs_pair_t(EcsScript, owner));
    while (ecs_each_next(&it)) {
        for (int32_t i = 0; i < it.count; ++i) {
            const char* entity_name = ecs_get_name(world, it.entities[i]);
            if (entity_name && strcmp(entity_name, name) == 0) {
                return it.entities[i];
            }
        }
    }
    return 0;
}

ecs_entity_t pulse_prefab_script_instantiate(PulseAppId app, ecs_entity_t prefab, ecs_entity_t entity, const char** p_names, size_t names_count, const char** p_values, size_t values_count) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !prefab) {
        pulse_prefab_internal::set_error("prefab script: invalid argument");
        return 0;
    }
    if (names_count != values_count || (names_count && (!p_names || !p_values))) {
        pulse_prefab_internal::set_error("prefab script: names and values must be non-null arrays of equal length");
        return 0;
    }
    if (!ecs_is_alive(world, prefab)) {
        pulse_prefab_internal::set_error("prefab script: prefab entity is not alive");
        return 0;
    }
    if (entity && !ecs_is_alive(world, entity)) {
        pulse_prefab_internal::set_error("prefab script: target entity is not alive");
        return 0;
    }

    const EcsScript* script = ecs_get(world, prefab, EcsScript);
    if (!script || !script->template_) {
        if (!ecs_has_id(world, prefab, EcsPrefab)) {
            pulse_prefab_internal::set_error("prefab script: entity '%s' is not a template or prefab", ecs_get_name(world, prefab) ? ecs_get_name(world, prefab) : "?");
            return 0;
        }
        if (names_count) {
            pulse_prefab_internal::set_error("prefab script: prefab '%s' takes no props", ecs_get_name(world, prefab) ? ecs_get_name(world, prefab) : "?");
            return 0;
        }
        if (entity) {
            ecs_add_pair(world, entity, EcsIsA, prefab);
            return entity;
        }
        return ecs_new_w_pair(world, EcsIsA, prefab);
    }

    const ecs_type_info_t* ti = ecs_get_type_info(world, prefab);
    if (!ti || !ti->size) {
        pulse_prefab_internal::set_error("prefab script: template has no reflection data");
        return 0;
    }

    bool created = false;
    if (!entity) {
        entity = ecs_new(world);
        created = true;
    }

    void* ptr = ecs_ensure_id(world, entity, prefab, ti->size);
    ecs_meta_cursor_t cursor = ecs_meta_cursor(world, prefab, ptr);
    if (!ptr || !cursor.valid || ecs_meta_push(&cursor) != 0) {
        if (created) {
            ecs_delete(world, entity);
        }
        pulse_prefab_internal::set_error("prefab script: template has no reflection data");
        return 0;
    }

    for (size_t i = 0; i < names_count; ++i) {
        const char* name = p_names[i];
        const char* value = p_values[i] ? p_values[i] : "";
        if (!name || !name[0]) {
            if (created) {
                ecs_delete(world, entity);
            }
            pulse_prefab_internal::set_error("prefab script: prop name must be a non-empty string");
            return 0;
        }

        const ecs_member_t* member = ecs_struct_get_member(world, prefab, name);
        if (!member) {
            if (created) {
                ecs_delete(world, entity);
            }
            pulse_prefab_internal::set_error("prefab script: unknown prop '%s'", name);
            return 0;
        }

        const EcsOpaque* opaque = member->type ? ecs_get(world, member->type, EcsOpaque) : nullptr;
        if (opaque && opaque->as_type == ecs_id(ecs_string_t) && opaque->user_data != 0 && value[0]) {
            uint64_t type_id = static_cast<uint64_t>(opaque->user_data);
            pulse_prefab_internal::request_file_load(pulse_get_asset_system(app), type_id, value);
            if (!pulse_asset_handle_is_valid(pulse_asset_system_find_loaded(pulse_get_asset_system(app), type_id, value))) {
                if (created) {
                    ecs_delete(world, entity);
                }
                pulse_prefab_internal::set_error("prefab script: prop '%s': asset '%s' is not loaded", name, value);
                return 0;
            }
        }

        if (ecs_meta_member(&cursor, name) != 0) {
            if (created) {
                ecs_delete(world, entity);
            }
            pulse_prefab_internal::set_error("prefab script: prop '%s' cannot be selected", name);
            return 0;
        }
        if (ecs_meta_set_string(&cursor, value) != 0) {
            if (created) {
                ecs_delete(world, entity);
            }
            pulse_prefab_internal::set_error("prefab script: prop '%s' cannot be set from string '%s'", name, value);
            return 0;
        }
    }

    ecs_meta_pop(&cursor);
    ecs_modified_id(world, entity, prefab);
    return entity;
}

const char* pulse_prefab_last_error(void) {
    return pulse_prefab_internal::g_last_error;
}

} // extern "C"
