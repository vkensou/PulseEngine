#include "asset_internal.h"

namespace pulse::asset {

ECS_COMPONENT_DECLARE(pulse_asset_state_resource);

static void process_load_requests_callback(ecs_iter_t* it) {
    AssetSystem* system = static_cast<AssetSystem*>(it->ctx);
    if (system) {
        system->process_load_requests();
    }
}

EPulseResult AssetSystem::build(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    app_ = app;
    ECS_COMPONENT_DEFINE(world, pulse_asset_state_resource);

    pulse_asset_state_resource resource{};
    resource.system = this;
    ecs_singleton_set_ptr(world, pulse_asset_state_resource, &resource);

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

AssetSystem* system_from_app(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(pulse_asset_state_resource) == 0) {
        return nullptr;
    }

    const pulse_asset_state_resource* resource =
        ecs_singleton_get(world, pulse_asset_state_resource);
    return resource ? resource->system : nullptr;
}

EPulseResult asset_plugin_build_callback(PulseAppId app, void* ctx) {
    AssetSystem* system = static_cast<AssetSystem*>(ctx);
    return system ? system->build(app) : PULSE_RESULT_ERROR_INVALID_ARGUMENT;
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
