#include "datatable_internal.h"

#include <cstdio>
#include <memory>

struct pulse_data_table_state {
    PulseDataTableSystem* system = nullptr;
};

ECS_COMPONENT_DECLARE(pulse_data_table_state);

PulseDataTableSystem::PulseDataTableSystem()
    : memory_pool(std::pmr::monotonic_buffer_resource(64u * 1024u * 1024u)),
      registry_(&memory_pool) {
}

PulseDataTableSystem::~PulseDataTableSystem() = default;

pulse::datatable::Registry& PulseDataTableSystem::registry() {
    return registry_;
}

pulse::datatable::Registry* PulseDataTableSystem::registry_ptr() {
    return &registry_;
}

namespace pulse::datatable {

EPulsePluginBuildResult data_table_plugin_build_callback(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    auto* system = static_cast<PulseDataTableSystem*>(ctx);
    PulseAssetSystemId asset_system = pulse_get_asset_system(app);
    if (!world || !system || !asset_system) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }

    system->registry().bind(app, asset_system);
    EPulseResult type_result = register_data_table_type(asset_system, &system->registry());
    EPulseResult loader_result = register_data_table_loader(asset_system, &system->registry());
    if (type_result != PULSE_RESULT_OK) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }
    if (loader_result != PULSE_RESULT_OK) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }

    ecs_id(pulse_data_table_state) = flecs::_::type<pulse_data_table_state>::id(world);
    pulse_data_table_state state{};
    state.system = system;
    ecs_singleton_set_ptr(world, pulse_data_table_state, &state);
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult data_table_plugin_post_build_callback(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void data_table_plugin_shutdown_callback(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (world && ecs_id(pulse_data_table_state) != 0) {
        ecs_singleton_remove(world, pulse_data_table_state);
        if (ecs_is_alive(world, ecs_id(pulse_data_table_state))) {
            ecs_delete(world, ecs_id(pulse_data_table_state));
        }
        ecs_id(pulse_data_table_state) = 0;
    }
    pulse_asset_system_force_unload_assets(pulse_get_asset_system(app), PULSE_TYPE_DATA_TABLE);
    delete static_cast<PulseDataTableSystem*>(ctx);
}

} // namespace pulse::datatable

using namespace pulse::datatable;

extern "C" {

PulseDataTablePluginDesc pulse_data_table_plugin_desc_default(void) {
    PulseDataTablePluginDesc desc{};
    desc.struct_size = sizeof(PulseDataTablePluginDesc);
    desc.version = PULSE_DATA_TABLE_PLUGIN_DESC_VERSION;
    return desc;
}

EPulseAppAddPluginResult pulse_add_data_table_plugin(PulseAppId app, const PulseDataTablePluginDesc* desc) {
    if (!app || !desc || desc->struct_size != sizeof(PulseDataTablePluginDesc) || desc->version != PULSE_DATA_TABLE_PLUGIN_DESC_VERSION) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    const char* dependencies[] = { "pulse_asset" };

    std::unique_ptr<PulseDataTableSystem> system(new PulseDataTableSystem());

    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_DATA_TABLE_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = system.get(),
        .build = data_table_plugin_build_callback,
        .post_build = data_table_plugin_post_build_callback,
        .shutdown = data_table_plugin_shutdown_callback,
        .dependency_count = 1,
        .dependencies = dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK) {
        return result;
    }
    system.release();
    return result;
}

PulseDataTableSystemId pulse_get_data_table_system(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(pulse_data_table_state) == 0) {
        return nullptr;
    }
    const pulse_data_table_state* state = ecs_singleton_get(world, pulse_data_table_state);
    return state ? state->system : nullptr;
}

} // extern "C"
