#include "transform_internal.h"

namespace pulse_transform_internal {

constexpr const char* kPluginName = "PulseTransformPlugin";

// ============================================================
// Plugin lifecycle
// ============================================================

EPulsePluginBuildResult transform_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* state = static_cast<pulse_transform_plugin_state*>(ctx);
    if (!state) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    state->app = app;

    // Register ECS components
    register_components(world);

    // Install transform systems (no cross-plugin dependencies)
    install_transform_systems(world);

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult transform_plugin_post_build(PulseAppId app, void* ctx) {
    // Systems are installed in build phase — no additional setup needed
    (void)app;
    (void)ctx;
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void transform_plugin_shutdown(PulseAppId app, void* ctx) {
    (void)app;
    auto* state = static_cast<pulse_transform_plugin_state*>(ctx);
    delete state;
}

} // namespace pulse_transform_internal

using namespace pulse_transform_internal;

// ============================================================
// Public C API
// ============================================================

extern "C" {

EPulseAppAddPluginResult pulse_add_transform_plugin(PulseAppId app) {
    if (!app) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    auto* state = new pulse_transform_plugin_state();

    PulsePluginDesc plugin_desc = {
        sizeof(PulsePluginDesc),
        PULSE_PLUGIN_DESC_VERSION,
        kPluginName,
        state,
        transform_plugin_build,
        transform_plugin_post_build,
        transform_plugin_shutdown,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

void pulse_set_parent(PulseAppId app, ecs_entity_t child, ecs_entity_t parent) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !child || !parent) return;
    ecs_add_pair(world, child, EcsChildOf, parent);
}

void pulse_remove_parent(PulseAppId app, ecs_entity_t child) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !child) return;
    ecs_remove_pair(world, child, EcsChildOf, EcsWildcard);
}

ecs_entity_t pulse_get_parent(PulseAppId app, ecs_entity_t child) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !child) return 0;
    return ecs_get_target(world, child, EcsChildOf, 0);
}

} // extern "C"
