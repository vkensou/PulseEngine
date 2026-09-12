#include "prefab_internal.h"

namespace pulse_prefab_internal {

const char* kPluginName = "pulse_prefab";

EPulsePluginBuildResult prefab_plugin_build(PulseAppId app, void* ctx) {
    (void)ctx;
    PulseAssetSystemId asset_system = pulse_get_asset_system(app);
    if (!asset_system) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }

    register_prefab_type(asset_system, app);
    register_prefab_load_loader(asset_system);
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult prefab_plugin_post_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void prefab_plugin_shutdown(PulseAppId app, void* ctx) {
    (void)ctx;
    pulse_asset_system_force_unload_assets(pulse_get_asset_system(app), PULSE_TYPE_PREFAB);
}

} // namespace pulse_prefab_internal

using namespace pulse_prefab_internal;

extern "C" {

EPulseAppAddPluginResult pulse_add_prefab_plugin(PulseAppId app) {
    if (!app) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    const char* prefab_dependencies[] = { "pulse_asset" };

    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_PREFAB_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = nullptr,
        .build = prefab_plugin_build,
        .post_build = prefab_plugin_post_build,
        .shutdown = prefab_plugin_shutdown,
        .dependency_count = 1,
        .dependencies = prefab_dependencies,
    };

    return pulse_app_add_plugin(app, &plugin_desc);
}

} // extern "C"
