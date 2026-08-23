#include "snake_module.h"

#include "pulse_config.h"
#include "pulse_cpp_gameplay.h"
#include "pulse_imgui.h"

namespace {

constexpr const char* kPluginName = "PulseSnakePlugin";

using SnakePluginState = pulse::GameplayModuleState;

EPulsePluginBuildResult snake_plugin_build(PulseAppId app, void* ctx)
{
    if (!app || !ctx) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* state = static_cast<SnakePluginState*>(ctx);
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    flecs::world world_view(world);

    pulse::init_gameplay_base(world_view);

    pulse::ModuleContext moduleContext = pulse::make_module_context(
        world_view,
        pulse_imgui_get_phase(app),
        &state->eventCenter);
    importModule(&moduleContext);

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void snake_plugin_shutdown(PulseAppId app, void* ctx)
{
    auto* state = static_cast<SnakePluginState*>(ctx);
    if (!state) {
        return;
    }
    state->eventCenter.clear();
    delete state;
}

EPulseAppAddPluginResult pulse_add_snake_plugin(PulseAppId app)
{
    if (!app) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    auto* state = new (std::nothrow) SnakePluginState();
    if (!state) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INTERNAL;
    }

    static const char* snake_dependencies[] = {
        "PulseWindowPlugin",
        "PulseInputPlugin",
        "PulseAssetPlugin",
        "PulseTransformPlugin",
        "PulseGraphicPlugin",
        "PulseRendererPlugin",
        "PulseImguiPlugin",
    };

    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = 1,
        .name = kPluginName,
        .ctx = state,
        .build = snake_plugin_build,
        .post_build = nullptr,
        .shutdown = snake_plugin_shutdown,
        .dependency_count = 7,
        .dependencies = snake_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

} // namespace

extern "C" PULSE_EXPORT EPulseResult pulse_package_register(PulseAppId app, PulseConfig* config)
{
    if (config) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return pulse::to_package_result(pulse_add_snake_plugin(app));
}
