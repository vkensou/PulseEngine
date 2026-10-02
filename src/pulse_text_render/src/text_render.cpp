#include "text_render_internal.h"

#include "pulse_text_render_reflection.h"

ECS_COMPONENT_DECLARE(PulseText);

namespace pulse_text_render_internal {

constexpr const char* kPluginName = "pulse_text_render";

EPulsePluginBuildResult text_render_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    auto* state = static_cast<pulse_text_render_state*>(ctx);
    if (!state) return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    state->app = app;

    pulse_text_render_register_reflection(world);
    install_text_feature(app, world);

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void text_render_plugin_shutdown(PulseAppId app, void* ctx) {
    if (app) pulse_remove_render_feature(app, kTextFeatureName);
    delete static_cast<pulse_text_render_state*>(ctx);
}

} // namespace pulse_text_render_internal

using namespace pulse_text_render_internal;

extern "C" {

EPulseAppAddPluginResult pulse_add_text_render_plugin(PulseAppId app) {
    if (!app) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    if (pulse_app_has_plugin(app, kPluginName)) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;

    auto* state = new pulse_text_render_state();

    const char* text_render_dependencies[] = { "pulse_graphics", "pulse_transform", "pulse_renderer", "pulse_text", "pulse_font" };

    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_TEXT_RENDER_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = state,
        .build = text_render_plugin_build,
        .post_build = nullptr,
        .shutdown = text_render_plugin_shutdown,
        .dependency_count = 5,
        .dependencies = text_render_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

} // extern "C"
