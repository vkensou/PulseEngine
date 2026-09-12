#include "prefab_internal.h"

namespace pulse_prefab_internal {

namespace {

PulseAssetRequest request_file_load(PulseAssetSystemId asset_system, uint64_t type_id, const char* path) {
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    return pulse_asset_system_load(asset_system, &desc);
}

void collect_asset_reference(void* ctx, uint64_t asset_type, const char* path) {
    auto* references = static_cast<prefab_reference_set*>(ctx);
    if (!asset_type || !path || !path[0]) {
        return;
    }
    for (const prefab_reference& seen : references->references) {
        if (seen.type_id == asset_type && seen.path == path) {
            return;
        }
    }
    references->references.push_back(prefab_reference{ asset_type, path });
}

bool asset_state_is_pending(EPulseAssetState state) {
    return state == PULSE_ASSET_STATE_WAITING_LOAD || state == PULSE_ASSET_STATE_LOADING || state == PULSE_ASSET_STATE_WAITING_DEPENDENCIES || state == PULSE_ASSET_STATE_PROCESSING;
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

    if (!s->references) {
        s->references = new (std::nothrow) prefab_reference_set();
        if (!s->references) {
            *out_error = "prefab loader: out of memory";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        s->references->json.assign(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
        if (!ecs_asset_refs_from_json(world, s->references->json.c_str(), collect_asset_reference, s->references)) {
            *out_error = "prefab loader: json scan failed";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
    }

    for (const prefab_reference& reference : s->references->references) {
        PulseAssetRequest request = request_file_load(ctx->asset_system, reference.type_id, reference.path.c_str());
        if (asset_state_is_pending(pulse_asset_system_get_state(ctx->asset_system, request))) {
            return PULSE_ASSET_LOADER_STATUS_PENDING;
        }
    }

    bool suspended = ecs_is_deferred(world);
    if (suspended) {
        ecs_defer_suspend(world);
    }

    ecs_entity_t root = ecs_new(world);
    bool deserialized = ecs_entity_from_json(world, root, s->references->json.c_str(), nullptr) != nullptr;
    if (deserialized) {
        ecs_add_id(world, root, EcsPrefab);
    } else {
        ecs_delete(world, root);
    }

    if (suspended) {
        ecs_defer_resume(world);
    }

    if (!deserialized) {
        *out_error = "prefab loader: json deserialize failed";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    static_cast<PulsePrefabData*>(ctx->out_asset)->root = root;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void dtor_prefab_load(void* state, const PulseAssetLoadTask* ctx) {
    (void)ctx;
    auto* s = static_cast<prefab_load_state*>(state);
    delete s->references;
    s->references = nullptr;
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
