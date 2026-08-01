#include <assert.h>
#include <stdio.h>

#include <flecs.h>

#include "pulse_app.h"

static bool build_called = false;
static bool post_build_called = false;
static bool shutdown_called = false;
static bool nested_build_called = false;
static bool sub_build_called = false;
static bool sub_post_build_called = false;
static bool sub_shutdown_called = false;
static bool extract_called = false;

static EPulseResult nested_build(PulseAppId app, void* ctx) {
    (void)ctx;
    nested_build_called = true;
    assert(pulse_app_world(app) != nullptr);
    return PULSE_RESULT_OK;
}

static EPulseResult my_build(PulseAppId app, void* ctx) {
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
    return pulse_app_add_plugin(app, &nested);
}

static EPulseResult my_post_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    post_build_called = true;
    return PULSE_RESULT_OK;
}

static void my_shutdown(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    shutdown_called = true;
}

static EPulseResult sub_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_build_called = true;
    return PULSE_RESULT_OK;
}

static EPulseResult sub_post_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_post_build_called = true;
    return PULSE_RESULT_OK;
}

static void sub_shutdown(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_shutdown_called = true;
}

static EPulseResult extract_subapp(PulseAppId app, PulseAppId subapp, void* ctx) {
    (void)ctx;
    extract_called = true;
    assert(pulse_app_world(app) != nullptr);
    assert(pulse_app_world(subapp) != nullptr);
    assert(pulse_app_world(app) != pulse_app_world(subapp));
    return PULSE_RESULT_OK;
}

static EPulseResult test_runner(PulseAppId app, void* ctx) {
    int* frames = (int*)ctx;
    for (int i = 0; i < *frames; ++i) {
        EPulseResult result = pulse_app_update(app);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    return PULSE_RESULT_OK;
}

int main(void) {
    PulseAppId app = pulse_create_app("test-app");
    assert(app != nullptr);

    PulsePluginDesc desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "TestPlugin",
        .build = my_build,
        .post_build = my_post_build,
        .shutdown = my_shutdown,
    };
    assert(pulse_app_add_plugin(app, &desc) == PULSE_RESULT_OK);

    assert(build_called);
    assert(nested_build_called);
    assert(pulse_app_has_plugin(app, "TestPlugin"));
    assert(pulse_app_has_plugin(app, "NestedPlugin"));
    assert(pulse_app_add_plugin(app, &desc) == PULSE_RESULT_ERROR_DUPLICATE_PLUGIN);

    PulseAppId sub = pulse_create_app("Sub");
    assert(sub != nullptr);

    PulsePluginDesc sub_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "SubPlugin",
        .build = sub_build,
        .post_build = sub_post_build,
        .shutdown = sub_shutdown,
    };
    assert(pulse_app_add_plugin(sub, &sub_desc) == PULSE_RESULT_OK);
    assert(sub_build_called);

    assert(pulse_app_insert_subapp(app, "Sub", sub) == PULSE_RESULT_OK);
    assert(pulse_app_get_subapp(app, "Sub") == sub);
    assert(pulse_app_set_subapp_extract(app, "Sub", extract_subapp, nullptr) == PULSE_RESULT_OK);

    int frames = 1;
    assert(pulse_app_set_runner(app, test_runner, &frames) == PULSE_RESULT_OK);
    assert(pulse_app_run(app) == PULSE_RESULT_OK);

    assert(post_build_called);
    assert(sub_post_build_called);
    assert(extract_called);

    PulseAppId removed = pulse_app_remove_subapp(app, "Sub");
    assert(removed == sub);
    assert(pulse_app_get_subapp(app, "Sub") == nullptr);
    pulse_destroy_app(removed);
    assert(sub_shutdown_called);

    pulse_destroy_app(app);
    assert(shutdown_called);

    printf("All tests passed!\n");
    return 0;
}
