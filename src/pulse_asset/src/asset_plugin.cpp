#include "asset_internal.h"

struct PulseAssetSystem {
    pulse::asset::AssetSystem impl;

    explicit PulseAssetSystem(const PulseAssetPluginDesc& desc)
        : impl(desc) {

    }
};

namespace pulse::asset {

struct pulse_asset_state_resource {
    PulseAssetSystem* system;
};

ECS_COMPONENT_DECLARE(pulse_asset_state_resource);

PulseAssetPluginDesc default_plugin_desc() {
    PulseAssetPluginDesc desc{};
    desc.struct_size = sizeof(PulseAssetPluginDesc);
    desc.version = PULSE_ASSET_PLUGIN_DESC_VERSION;
    desc.root_path = "assets";
    desc.max_requests_per_update = 8;
    return desc;
}

PulseAssetPluginDesc normalize_plugin_desc(const PulseAssetPluginDesc* desc) {
    PulseAssetPluginDesc normalized = default_plugin_desc();
    if (desc) {
        normalized = *desc;
    }
    normalized.struct_size = sizeof(PulseAssetPluginDesc);
    normalized.version = PULSE_ASSET_PLUGIN_DESC_VERSION;
    if (!normalized.root_path || !normalized.root_path[0]) {
        normalized.root_path = ".";
    }
    if (normalized.max_requests_per_update == 0) {
        normalized.max_requests_per_update = 8;
    }
    return normalized;
}

bool validate_plugin_desc(const PulseAssetPluginDesc* desc) {
    return !desc ||
        (desc->struct_size == sizeof(PulseAssetPluginDesc) &&
         desc->version == PULSE_ASSET_PLUGIN_DESC_VERSION);
}

static void process_load_requests_callback(ecs_iter_t* it) {
    AssetSystem* system = static_cast<AssetSystem*>(it->ctx);
    if (system) {
        system->process_load_requests();
    }
}

EPulseResult AssetSystem::build(PulseAppId app, ecs_world_t* world) {
    app_ = app;
    install_process_system(world);
    return process_system_ ? PULSE_RESULT_OK : PULSE_RESULT_ERROR_INTERNAL;
}

void AssetSystem::shutdown(PulseAppId app) {
    load_queue_.cancel_all(*this);
    storage_.destroy_all_assets();

    ecs_world_t* world = pulse_app_world(app);
    uninstall_process_system(world);

    if (world && ecs_id(pulse_asset_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_asset_state_resource);
        if (ecs_is_alive(world, ecs_id(pulse_asset_state_resource))) {
            ecs_delete(world, ecs_id(pulse_asset_state_resource));
        }
        ecs_id(pulse_asset_state_resource) = 0;
    }
}

void AssetSystem::install_process_system(ecs_world_t* world) {
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "PulseAssetProcessLoadRequests";

    ecs_system_desc_t system_desc{};
    system_desc.entity = ecs_entity_init(world, &entity_desc);
    system_desc.phase = EcsOnLoad;
    system_desc.callback = process_load_requests_callback;
    system_desc.ctx = this;
    process_system_ = ecs_system_init(world, &system_desc);
}

void AssetSystem::uninstall_process_system(ecs_world_t* world) {
    if (world && process_system_ && ecs_is_alive(world, process_system_)) {
        ecs_delete(world, process_system_);
    }
    process_system_ = 0;
}

PulseAssetSystemId system_from_app(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(pulse_asset_state_resource) == 0) {
        return nullptr;
    }

    const pulse_asset_state_resource* resource =
        ecs_singleton_get(world, pulse_asset_state_resource);
    return resource ? resource->system : nullptr;
}

EPulseResult asset_plugin_build_callback(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    PulseAssetSystem* system = static_cast<PulseAssetSystem*>(ctx);
    if (system) {
        auto build_result = system->impl.build(app, world);
        if (build_result != PULSE_RESULT_OK)
            return build_result;

        ECS_COMPONENT_DEFINE(world, pulse_asset_state_resource);
        pulse_asset_state_resource resource{};
        resource.system = system;
        ecs_singleton_set_ptr(world, pulse_asset_state_resource, &resource);
        return PULSE_RESULT_OK;
    }
    return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
}

void asset_plugin_shutdown_callback(PulseAppId app, void* ctx) {
    AssetSystem* system = static_cast<AssetSystem*>(ctx);
    if (!system) {
        return;
    }

    system->shutdown(app);
    delete system;
}

} // namespace pulse::asset

extern "C" {
static inline pulse::asset::AssetSystem* to_impl(PulseAssetSystemId asset_system) {
    return asset_system ? &asset_system->impl : nullptr;
}

PulseAssetPluginDesc pulse_asset_plugin_desc_default(void) {
    return pulse::asset::default_plugin_desc();
}

EPulseResult pulse_add_asset_plugin(
    PulseAppId app,
    const PulseAssetPluginDesc* desc
) {
    if (!app || !pulse::asset::validate_plugin_desc(desc)) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, pulse::asset::kPluginName)) {
        return PULSE_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    PulseAssetPluginDesc normalized = pulse::asset::normalize_plugin_desc(desc);
    auto* system = new (std::nothrow) PulseAssetSystem(normalized);
    if (!system) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    PulsePluginDesc plugin_desc = {
        sizeof(PulsePluginDesc),
        PULSE_PLUGIN_DESC_VERSION,
        pulse::asset::kPluginName,
        system,
        pulse::asset::asset_plugin_build_callback,
        nullptr,
        pulse::asset::asset_plugin_shutdown_callback,
    };

    EPulseResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_RESULT_OK && !pulse_app_has_plugin(app, pulse::asset::kPluginName)) {
        delete system;
    }
    return result;
}

PULSE_API PulseAssetSystemId pulse_get_asset_system(
    PulseAppId app
) {
    return pulse::asset::system_from_app(app);
}

EPulseResult pulse_asset_system_register_type(
    PulseAssetSystemId asset_system,
    const PulseAssetTypeDesc* desc
) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    return system ? system->register_type(desc) : PULSE_RESULT_ERROR_INVALID_ARGUMENT;
}

EPulseResult pulse_asset_system_register_loader(
    PulseAssetSystemId asset_system,
    const PulseAssetLoaderDesc* desc
) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    return system ? system->register_loader(desc) : PULSE_RESULT_ERROR_INVALID_ARGUMENT;
}

PulseAssetRequest pulse_asset_system_load(
    PulseAssetSystemId asset_system,
    const PulseAssetLoadDesc* desc
) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    return system ? pulse::asset::handle_to_request(system->load(desc)) : pulse_asset_request_make_invalid();
}

PulseAssetRequest pulse_asset_system_load_from_memory(
    PulseAssetSystemId asset_system,
    const PulseAssetMemoryLoadDesc* desc
) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    return system ? pulse::asset::handle_to_request(system->load_from_memory(desc)) : pulse_asset_request_make_invalid();
}

PulseAssetRequest pulse_asset_system_build(
    PulseAssetSystemId asset_system,
    const PulseAssetBuildDesc* desc
) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    return system ? pulse::asset::handle_to_request(system->build_asset(desc)) : pulse_asset_request_make_invalid();
}

PulseAssetHandle pulse_asset_system_build_sync(
    PulseAssetSystemId asset_system,
    const PulseAssetBuildDesc* desc
) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    PulseAssetHandle handle = system ? system->build_asset(desc) : pulse_asset_handle_make_invalid();
    return system && system->get_state(handle) == PULSE_ASSET_STATE_LOADED ? handle : pulse_asset_handle_make_invalid();
}

EPulseAssetState pulse_asset_system_get_state(
    PulseAssetSystemId asset_system,
    PulseAssetRequest request
) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    return system ? system->get_state(pulse::asset::request_to_handle(request)) : PULSE_ASSET_STATE_EMPTY;
}

bool pulse_asset_system_is_alive(PulseAssetSystemId asset_system, PulseAssetRequest request)
{
    auto state = pulse_asset_system_get_state(asset_system, request);
    return !(state == PULSE_ASSET_STATE_EMPTY || state == PULSE_ASSET_STATE_FAILED || state == PULSE_ASSET_STATE_PENDING_DELETE);
}

bool pulse_asset_system_is_ready(PulseAssetSystemId asset_system, PulseAssetRequest request) {
    return pulse_asset_system_get_state(asset_system, request) == PULSE_ASSET_STATE_LOADED;
}

const char* pulse_asset_system_get_error(PulseAssetSystemId asset_system, PulseAssetRequest request) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    return system ? system->get_error(pulse::asset::request_to_handle(request)) : nullptr;
}

PulseAssetHandle pulse_asset_system_get_handle(
    PulseAssetSystemId asset_system,
    PulseAssetRequest request
) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    PulseAssetHandle handle = pulse::asset::request_to_handle(request);
    return system && system->get_state(handle) == PULSE_ASSET_STATE_LOADED ? handle : pulse_asset_handle_make_invalid();
}

void pulse_asset_system_cancel(PulseAssetSystemId asset_system, PulseAssetRequest request) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    if (system) {
        system->release(pulse::asset::request_to_handle(request), nullptr);
    }
}

bool pulse_asset_system_retain(PulseAssetSystemId asset_system, PulseAssetHandle handle, EPulseRetainErrorCode* out_error) {
    if (!pulse_asset_handle_is_valid(handle)) {
        if (out_error) {
            *out_error = PULSE_RETAIN_ERROR_CODE_HANDLE_IS_INVALID;
        }
        return false;
    }

    pulse::asset::AssetSystem* system = to_impl(asset_system);
    if (!system) {
        if (out_error) {
            *out_error = PULSE_RETAIN_ERROR_CODE_ASSET_SYSTEM_IS_INVALID;
        }
        return false;
    }

    return system->retain(handle, out_error);
}

bool pulse_asset_system_release(PulseAssetSystemId asset_system, PulseAssetHandle handle, EPulseReleaseErrorCode* out_error) {
    if (!pulse_asset_handle_is_valid(handle)) {
        if (out_error) {
            *out_error = PULSE_RELEASE_ERROR_CODE_HANDLE_IS_INVALID;
        }
        return false;
    }

    pulse::asset::AssetSystem* system = to_impl(asset_system);
    if (!system) {
        if (out_error) {
            *out_error = PULSE_RELEASE_ERROR_CODE_ASSET_SYSTEM_IS_INVALID;
        }
        return false;
    }

    return system->release(handle, out_error);
}

bool pulse_asset_system_borrow(PulseAssetSystemId asset_system, PulseAssetHandle handle, void** out_ref, EPulseBorrowErrorCode* out_error) {
    if (out_ref) {
        *out_ref = nullptr;
    }

    if (!pulse_asset_handle_is_valid(handle)) {
        if (out_error) {
            *out_error = PULSE_BORROW_ERROR_CODE_HANDLE_IS_INVALID;
        }
        return false;
    }

    pulse::asset::AssetSystem* system = to_impl(asset_system);
    if (!system) {
        if (out_error) {
            *out_error = PULSE_BORROW_ERROR_CODE_ASSET_SYSTEM_IS_INVALID;
        }
        return false;
    }

    return system->borrow(handle, out_ref, out_error);
}

void pulse_asset_system_mark_modified(PulseAssetSystemId asset_system, PulseAssetHandle handle) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    if (system) {
        system->mark_modified(handle);
    }
}

void pulse_asset_system_force_unload_assets(PulseAssetSystemId asset_system, uint64_t type_id) {
    pulse::asset::AssetSystem* system = to_impl(asset_system);
    if (system) {
        system->force_unload_assets(type_id);
    }
}

EPulseResult pulse_asset_load_task_add_dependency(
    PulseAssetLoadDependencyHint* hint,
    PulseAssetDepRef dependency,
    EPulseLoadDependencyRequirement flags
) {
    if (!hint) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return hint->parent->add_dependency(pulse::asset::dep_ref_to_handle(dependency), flags);
}

PulseAssetDepRef pulse_asset_system_to_asset_dep_ref_from_handle(PulseAssetSystemId asset_system, PulseAssetHandle handle) {
    (void)asset_system;
    return pulse::asset::handle_to_dep_ref(handle);
}

PulseAssetDepRef pulse_asset_system_to_asset_dep_ref_from_request(PulseAssetSystemId asset_system, PulseAssetRequest request) {
    (void)asset_system;
    return pulse::asset::request_to_dep_ref(request);
}

PulseAssetRequest pulse_asset_system_to_asset_request_from_dep_ref(PulseAssetSystemId asset_system, PulseAssetDepRef dep_ref) {
    (void)asset_system;
    return pulse::asset::dep_ref_to_request(dep_ref);
}

}