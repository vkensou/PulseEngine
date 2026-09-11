#include "math_internal.h"

namespace pulse_math_internal {

constexpr const char* kPluginName = "pulse_math";

EPulsePluginBuildResult math_plugin_build(PulseAppId app, void* ctx) {
    (void)ctx;
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    register_reflection(world);

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult math_plugin_post_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void math_plugin_shutdown(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
}

} // namespace pulse_math_internal

using namespace pulse_math_internal;

extern "C" {

EPulseAppAddPluginResult pulse_add_math_plugin(PulseAppId app) {
    if (!app) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_MATH_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = nullptr,
        .build = math_plugin_build,
        .post_build = math_plugin_post_build,
        .shutdown = math_plugin_shutdown,
        .dependency_count = 0,
        .dependencies = nullptr,
    };

    return pulse_app_add_plugin(app, &plugin_desc);
}

} // extern "C"
