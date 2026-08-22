#include "test_common.h"

static bool build_called = false;
static bool post_build_called = false;
static bool shutdown_called = false;
static bool nested_build_called = false;
static bool sub_build_called = false;
static bool sub_post_build_called = false;
static bool sub_shutdown_called = false;
static bool extract_called = false;

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

static EPulsePluginBuildResult my_post_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    post_build_called = true;
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

static void my_shutdown(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    shutdown_called = true;
}

static EPulsePluginBuildResult sub_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_build_called = true;
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

static EPulsePluginBuildResult sub_post_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_post_build_called = true;
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

static void sub_shutdown(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_shutdown_called = true;
}

static EPulseSubappExtractResult extract_subapp(PulseAppId app, PulseAppId subapp, void* ctx) {
    (void)ctx;
    extract_called = true;
    assert(pulse_app_world(app) != nullptr);
    assert(pulse_app_world(subapp) != nullptr);
    assert(pulse_app_world(app) != pulse_app_world(subapp));
    return PULSE_SUBAPP_EXTRACT_RESULT_OK;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-app-run",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulsePluginDesc desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "TestPlugin",
        .build = my_build,
        .post_build = my_post_build,
        .shutdown = my_shutdown,
    };
    assert(pulse_app_add_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // ---- subapp with its own plugin and an extract callback ----
    PulseAppDesc sub_desc = {
        .name = "Sub",
    };
    PulseAppId sub = pulse_create_app(&sub_desc);
    assert(sub != nullptr);

    PulsePluginDesc sub_plugin = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "SubPlugin",
        .build = sub_build,
        .post_build = sub_post_build,
        .shutdown = sub_shutdown,
    };
    assert(pulse_app_add_plugin(sub, &sub_plugin) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_insert_subapp(app, "Sub", sub) == PULSE_APP_INSERT_SUBAPP_RESULT_OK);
    assert(pulse_app_set_subapp_extract(app, "Sub", extract_subapp, nullptr) == PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_OK);

    // ---- run: single-shot, auto-teardown ----
    int frames = 1;
    assert(pulse_app_set_runner(app, test_runner, &frames) == PULSE_APP_SET_RUNNER_RESULT_OK);
    assert(pulse_app_run(app) == PULSE_APP_RUN_RESULT_OK);

    assert(post_build_called);
    assert(sub_post_build_called);
    assert(extract_called);
    // auto-teardown: everything is released when run() returns
    assert(shutdown_called);
    assert(sub_shutdown_called);

    // ---- single-shot: cannot run or update again ----
    assert(pulse_app_run(app) == PULSE_APP_RUN_RESULT_ERROR_INVALID_STATE);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_ERROR_INVALID_STATE);

    pulse_destroy_app(app);

    printf("Run test passed!\n");
    return 0;
}