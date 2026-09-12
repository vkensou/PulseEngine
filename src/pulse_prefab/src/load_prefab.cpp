#include "prefab_internal.h"

#include <cstring>
#include <string>
#include <vector>

namespace pulse_prefab_internal {

namespace {

struct prefab_reference {
    uint64_t type_id = 0;
    std::string path;
};

struct prefab_reference_set {
    std::vector<prefab_reference> references;
};

struct prefab_apply_ctx {
    ecs_world_t* world = nullptr;
    prefab_reference_set* references = nullptr;
};

PulseAssetRequest request_file_load(PulseAssetSystemId asset_system, uint64_t type_id, const char* path) {
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    return pulse_asset_system_load(asset_system, &desc);
}

bool asset_state_is_pending(EPulseAssetState state) {
    return state == PULSE_ASSET_STATE_WAITING_LOAD || state == PULSE_ASSET_STATE_LOADING || state == PULSE_ASSET_STATE_WAITING_DEPENDENCIES || state == PULSE_ASSET_STATE_PROCESSING;
}

bool node_is_container(const PulseDatalist* node) {
    EPulseDatalistType type = pulse_datalist_get_type(node, nullptr);
    return type == PULSE_DATALIST_TYPE_LIST || type == PULSE_DATALIST_TYPE_MAP || type == PULSE_DATALIST_TYPE_MIXED;
}

bool node_is_empty_container(const PulseDatalist* node) {
    return pulse_datalist_object_count(node) == 0 && pulse_datalist_count(node) == 0;
}

void collect_reference(prefab_apply_ctx* ctx, uint64_t type_id, const char* path) {
    if (!ctx->references || !type_id || !path || !path[0]) {
        return;
    }
    for (const prefab_reference& seen : ctx->references->references) {
        if (seen.type_id == type_id && seen.path == path) {
            return;
        }
    }
    ctx->references->references.push_back(prefab_reference{ type_id, path });
}

bool walk_node(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error);
bool walk_struct(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error);
bool walk_collection(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error);

bool apply_scalar(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    EPulseDatalistType type = pulse_datalist_get_type(node, nullptr);
    if (type == PULSE_DATALIST_TYPE_NIL) {
        *out_error = "prefab loader: nil member value";
        return false;
    }
    if (ecs_meta_get_type(cursor) == ecs_id(ecs_entity_t)) {
        const char* text = pulse_datalist_get_string(node, nullptr, nullptr);
        if (!text) {
            *out_error = "prefab loader: entity reference needs a path string";
            return false;
        }
        return ecs_meta_set_entity(cursor, ecs_lookup_path_w_sep(ctx->world, 0, text, ".", nullptr, false)) == 0;
    }

    switch (type) {
    case PULSE_DATALIST_TYPE_BOOL:
        return ecs_meta_set_bool(cursor, pulse_datalist_get_bool(node, nullptr, false)) == 0;
    case PULSE_DATALIST_TYPE_INT:
        return ecs_meta_set_int(cursor, pulse_datalist_get_int(node, nullptr, 0)) == 0;
    case PULSE_DATALIST_TYPE_DOUBLE:
        return ecs_meta_set_float(cursor, pulse_datalist_get_double(node, nullptr, 0.0)) == 0;
    case PULSE_DATALIST_TYPE_STRING:
        return ecs_meta_set_string(cursor, pulse_datalist_get_string(node, nullptr, "")) == 0;
    default:
        *out_error = "prefab loader: member value is not a scalar";
        return false;
    }
}

bool walk_member(ecs_meta_cursor_t* cursor, const char* name, const PulseDatalist* value, prefab_apply_ctx* ctx, const char** out_error) {
    if (strchr(name, '.') != nullptr || ecs_meta_try_dotmember(cursor, name) != 0) {
        return true;
    }

    ecs_entity_t member_type = node_is_container(value) ? 0 : ecs_meta_get_type(cursor);
    const EcsOpaque* opaque = member_type ? ecs_get(ctx->world, member_type, EcsOpaque) : nullptr;
    if (ctx->references && opaque && opaque->as_type == ecs_id(ecs_string_t) && opaque->user_data != 0) {
        collect_reference(ctx, opaque->user_data, pulse_datalist_get_string(value, nullptr, nullptr));
        return true;
    }

    return walk_node(cursor, value, ctx, out_error);
}

bool walk_struct(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    size_t count = pulse_datalist_object_count(node);
    for (size_t i = 0; i < count; ++i) {
        const char* key = pulse_datalist_object_key(node, i);
        PulseDatalist* value = pulse_datalist_object_value(node, i);
        if (!key || !value) {
            continue;
        }
        if (!walk_member(cursor, key, value, ctx, out_error)) {
            return false;
        }
    }
    return true;
}

bool walk_collection(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    size_t count = pulse_datalist_count(node);
    for (size_t i = 0; i < count; ++i) {
        PulseDatalist* item = pulse_datalist_get(node, i);
        if (!item) {
            continue;
        }
        if (ecs_meta_elem(cursor, (int32_t)i) != 0) {
            *out_error = "prefab loader: collection has more elements than the reflected member";
            return false;
        }
        if (!walk_node(cursor, item, ctx, out_error)) {
            return false;
        }
    }
    return true;
}

bool walk_node(ecs_meta_cursor_t* cursor, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    if (!node_is_container(node)) {
        return apply_scalar(cursor, node, ctx, out_error);
    }
    if (ecs_meta_push(cursor) != 0) {
        *out_error = "prefab loader: member value does not match the reflected type";
        return false;
    }

    bool is_collection = ecs_meta_is_collection(cursor);
    if (!(is_collection ? walk_collection(cursor, node, ctx, out_error) : walk_struct(cursor, node, ctx, out_error))) {
        return false;
    }
    if (ecs_meta_pop(cursor) != 0) {
        *out_error = "prefab loader: failed to close member scope";
        return false;
    }
    return true;
}

ecs_id_t component_id_from_datalist(ecs_world_t* world, const char* name, const ecs_type_info_t** out_type_info) {
    ecs_entity_t id = ecs_lookup_path_w_sep(world, 0, name, ".", nullptr, false);
    const ecs_type_info_t* type_info = id ? ecs_get_type_info(world, id) : nullptr;
    if (!type_info || !type_info->component) {
        return 0;
    }
    *out_type_info = type_info;
    return id;
}

bool apply_component(ecs_world_t* world, ecs_entity_t entity, const char* name, const PulseDatalist* node, prefab_apply_ctx* ctx, const char** out_error) {
    const ecs_type_info_t* type_info = nullptr;
    ecs_id_t id = component_id_from_datalist(world, name, &type_info);
    if (!id) {
        return true;
    }
    if (pulse_datalist_get_type(node, nullptr) == PULSE_DATALIST_TYPE_NIL || node_is_empty_container(node)) {
        ecs_add_id(world, entity, id);
        return true;
    }

    void* ptr = ecs_ensure_id(world, entity, id, static_cast<size_t>(type_info->size));
    if (!ptr) {
        *out_error = "prefab loader: component has no reflection data";
        return false;
    }

    ecs_meta_cursor_t cursor = ecs_meta_cursor(world, type_info->component, ptr);
    if (!cursor.valid) {
        *out_error = "prefab loader: component has no reflection data";
        return false;
    }
    if (!walk_node(&cursor, node, ctx, out_error)) {
        return false;
    }

    ecs_modified_id(world, entity, id);
    return true;
}

bool scan_asset_references(ecs_world_t* world, const PulseDatalist* root, prefab_reference_set* references, const char** out_error) {
    if (!node_is_container(root)) {
        *out_error = "prefab loader: prefab root is not an object";
        return false;
    }

    PulseDatalist* components = pulse_datalist_get_obj(root, "components");
    if (!components) {
        return true;
    }

    prefab_apply_ctx ctx{ world, references };
    size_t count = pulse_datalist_object_count(components);
    for (size_t i = 0; i < count; ++i) {
        const char* name = pulse_datalist_object_key(components, i);
        PulseDatalist* node = pulse_datalist_object_value(components, i);
        if (!name || !node) {
            continue;
        }

        const ecs_type_info_t* type_info = nullptr;
        if (!component_id_from_datalist(world, name, &type_info) || type_info->size == 0) {
            continue;
        }

        std::vector<unsigned char> scratch(static_cast<size_t>(type_info->size));
        ecs_meta_cursor_t cursor = ecs_meta_cursor(world, type_info->component, scratch.data());
        if (!cursor.valid) {
            continue;
        }
        if (!walk_node(&cursor, node, &ctx, out_error)) {
            return false;
        }
    }
    return true;
}

ecs_entity_t build_prefab_entity(ecs_world_t* world, const PulseDatalist* root, const char** out_error) {
    if (!node_is_container(root)) {
        *out_error = "prefab loader: prefab root is not an object";
        return 0;
    }

    ecs_entity_t entity = ecs_new(world);
    const char* name = pulse_datalist_get_string(root, "name", nullptr);
    if (name && name[0] != '#') {
        ecs_set_name(world, entity, name);
    }

    PulseDatalist* components = pulse_datalist_get_obj(root, "components");
    if (components) {
        prefab_apply_ctx ctx{ world, nullptr };
        size_t count = pulse_datalist_object_count(components);
        for (size_t i = 0; i < count; ++i) {
            const char* component_name = pulse_datalist_object_key(components, i);
            PulseDatalist* node = pulse_datalist_object_value(components, i);
            if (!component_name || !node) {
                continue;
            }
            if (!apply_component(world, entity, component_name, node, &ctx, out_error)) {
                ecs_delete(world, entity);
                return 0;
            }
        }
    }

    ecs_add_id(world, entity, EcsPrefab);
    return entity;
}

void destroy_prefab(void* ptr, void* user_data) {
    auto* data = static_cast<PulsePrefabData*>(ptr);
    PulseAppId app = static_cast<PulseAppId>(user_data);
    ecs_world_t* world = app ? pulse_app_world(app) : nullptr;
    if (world && data->root && ecs_is_alive(world, data->root)) {
        ecs_delete(world, data->root);
    }
    data->root = 0;
}

EPulseAssetLoaderStatus step_prefab_load(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    auto* s = static_cast<prefab_load_state*>(state);

    ecs_world_t* world = pulse_app_world(ctx->app);
    if (!world) {
        *out_error = "prefab loader: app world unavailable";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    if (!s->datalist) {
        s->datalist = pulse_datalist_create_from_text(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
        if (!s->datalist) {
            *out_error = pulse_datalist_last_error();
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
    }

    if (!s->references_ready) {
        prefab_reference_set references;
        if (!scan_asset_references(world, s->datalist, &references, out_error)) {
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        for (const prefab_reference& reference : references.references) {
            PulseAssetRequest request = request_file_load(ctx->asset_system, reference.type_id, reference.path.c_str());
            if (asset_state_is_pending(pulse_asset_system_get_state(ctx->asset_system, request))) {
                return PULSE_ASSET_LOADER_STATUS_PENDING;
            }
        }
        s->references_ready = true;
    }

    bool suspended = ecs_is_deferred(world);
    if (suspended) {
        ecs_defer_suspend(world);
    }

    ecs_entity_t root = build_prefab_entity(world, s->datalist, out_error);

    if (suspended) {
        ecs_defer_resume(world);
    }

    if (!root) {
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    static_cast<PulsePrefabData*>(ctx->out_asset)->root = root;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void dtor_prefab_load(void* state, const PulseAssetLoadTask* ctx) {
    (void)ctx;
    auto* s = static_cast<prefab_load_state*>(state);
    pulse_datalist_release(s->datalist);
    s->datalist = nullptr;
}

} // namespace

void register_prefab_type(PulseAssetSystemId asset_system, PulseAppId app) {
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_PREFAB;
    type_desc.size = sizeof(PulsePrefabData);
    type_desc.align = alignof(PulsePrefabData);
    type_desc.destroy = destroy_prefab;
    type_desc.user_data = app;
    pulse_asset_system_register_type(asset_system, &type_desc);
}

void register_prefab_load_loader(PulseAssetSystemId asset_system) {
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_PREFAB;
    ld.extensions = "prefab";
    ld.ctor = nullptr;
    ld.dtor = dtor_prefab_load;
    ld.step = step_prefab_load;
    ld.loader_size = sizeof(prefab_load_state);
    ld.loader_align = alignof(prefab_load_state);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = nullptr;
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_prefab_internal

extern "C" {

PulsePrefabRequest pulse_load_prefab(PulseAppId app, const char* filepath) {
    PulsePrefabRequest result{};
    if (!app || !filepath || !filepath[0]) {
        return result;
    }

    PulseAssetRequest request = pulse_prefab_internal::request_file_load(pulse_get_asset_system(app), PULSE_TYPE_PREFAB, filepath);
    if (!pulse_asset_request_is_valid(request)) {
        return result;
    }

    result.index = request.index;
    result.generation = request.generation;
    return result;
}

PulsePrefabHandle pulse_prefab_get_handle(PulseAppId app, PulsePrefabRequest request) {
    if (!pulse_prefab_is_ready(app, request)) {
        return PulsePrefabHandle{};
    }
    PulseAssetHandle handle = pulse_asset_system_get_handle(pulse_get_asset_system(app), pulse_prefab_request_to_asset_request(request));
    return !pulse_asset_handle_is_valid(handle) ? PulsePrefabHandle{} : PulsePrefabHandle{ handle.index, handle.generation };
}

bool pulse_prefab_is_ready(PulseAppId app, PulsePrefabRequest request) {
    return pulse_asset_system_is_ready(pulse_get_asset_system(app), pulse_prefab_request_to_asset_request(request));
}

bool pulse_prefab_is_alive(PulseAppId app, PulsePrefabRequest request) {
    return pulse_asset_system_is_alive(pulse_get_asset_system(app), pulse_prefab_request_to_asset_request(request));
}

ecs_entity_t pulse_prefab_get_root(PulseAppId app, PulsePrefabHandle prefab) {
    void* ptr = nullptr;
    if (!pulse_asset_system_borrow(pulse_get_asset_system(app), pulse_prefab_to_handle(prefab), &ptr, nullptr)) {
        return 0;
    }
    return static_cast<PulsePrefabData*>(ptr)->root;
}

ecs_entity_t pulse_prefab_instantiate(PulseAppId app, PulsePrefabHandle prefab) {
    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t root = pulse_prefab_get_root(app, prefab);
    if (!world || !root || !ecs_is_alive(world, root)) {
        return 0;
    }

    ecs_entity_t instance = ecs_new(world);
    ecs_add_pair(world, instance, EcsIsA, root);
    return instance;
}

} // extern "C"
