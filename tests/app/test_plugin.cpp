#include "test_common.h"

static bool build_called = false;
static bool nested_build_called = false;

static EPulsePluginBuildResult nested_build(PulseAppId app, void* ctx) {
    (void)ctx;
    nested_build_called = true;
    assert(pulse_app_world(app) != nullptr);
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

static EPulsePluginBuildResult my_build(PulseAppId app, void* ctx) {
    (void)ctx;
    build_called = true;

    ecs_world_t* world = pulse_app_world(app);
    assert(world != nullptr);
    assert(ecs_new(world) != 0);

    PulsePluginDesc nested = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "NestedPlugin",
        .build = nested_build,
    };
    return static_cast<EPulsePluginBuildResult>(pulse_app_add_plugin(app, &nested));
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-app-plugin",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // ---- eager build: build runs immediately on add ----
    PulsePluginDesc desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "TestPlugin",
        .build = my_build,
    };
    assert(pulse_app_add_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(build_called);
    assert(nested_build_called);
    assert(pulse_app_has_plugin(app, "TestPlugin"));
    assert(pulse_app_has_plugin(app, "NestedPlugin"));
    assert(pulse_app_add_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN);

    pulse_destroy_app(app);

    printf("Plugin test passed!\n");
    return 0;
}