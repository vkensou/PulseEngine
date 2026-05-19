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

static pulse_result_t nested_build(pulse_app_t app, void* ctx) {
    (void)ctx;
    nested_build_called = true;
    assert(pulse_app_world(app) != nullptr);
    return PULSE_OK;
}

static pulse_result_t my_build(pulse_app_t app, void* ctx) {
    (void)ctx;
    build_called = true;

    ecs_world_t* world = pulse_app_world(app);
    assert(world != nullptr);
    assert(ecs_new(world) != 0);

    pulse_plugin_desc nested = {
        .struct_size = sizeof(pulse_plugin_desc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "NestedPlugin",
        .build = nested_build,
    };
    return pulse_app_add_plugin(app, &nested);
}

static pulse_result_t my_post_build(pulse_app_t app, void* ctx) {
    (void)app;
    (void)ctx;
    post_build_called = true;
    return PULSE_OK;
}

static void my_shutdown(pulse_app_t app, void* ctx) {
    (void)app;
    (void)ctx;
    shutdown_called = true;
}

static pulse_result_t sub_build(pulse_app_t app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_build_called = true;
    return PULSE_OK;
}

static pulse_result_t sub_post_build(pulse_app_t app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_post_build_called = true;
    return PULSE_OK;
}

static void sub_shutdown(pulse_app_t app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_shutdown_called = true;
}

static pulse_result_t extract_subapp(pulse_app_t app, pulse_app_t subapp, void* ctx) {
    (void)ctx;
    extract_called = true;
    assert(pulse_app_world(app) != nullptr);
    assert(pulse_app_world(subapp) != nullptr);
    assert(pulse_app_world(app) != pulse_app_world(subapp));
    return PULSE_OK;
}

static pulse_result_t test_runner(pulse_app_t app, void* ctx) {
    int* frames = (int*)ctx;
    for (int i = 0; i < *frames; ++i) {
        pulse_result_t result = pulse_app_update(app);
        if (result != PULSE_OK) {
            return result;
        }
    }
    return PULSE_OK;
}

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    pulse_plugin_desc desc = {
        .struct_size = sizeof(pulse_plugin_desc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "TestPlugin",
        .build = my_build,
        .post_build = my_post_build,
        .shutdown = my_shutdown,
    };
    assert(pulse_app_add_plugin(app, &desc) == PULSE_OK);

    assert(build_called);
    assert(nested_build_called);
    assert(pulse_app_has_plugin(app, "TestPlugin"));
    assert(pulse_app_has_plugin(app, "NestedPlugin"));
    assert(pulse_app_add_plugin(app, &desc) == PULSE_ERROR_DUPLICATE_PLUGIN);

    pulse_app_t sub = pulse_subapp_create("Sub");
    assert(sub != nullptr);

    pulse_plugin_desc sub_desc = {
        .struct_size = sizeof(pulse_plugin_desc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "SubPlugin",
        .build = sub_build,
        .post_build = sub_post_build,
        .shutdown = sub_shutdown,
    };
    assert(pulse_app_add_plugin(sub, &sub_desc) == PULSE_OK);
    assert(sub_build_called);

    assert(pulse_app_insert_subapp(app, "Sub", sub) == PULSE_OK);
    assert(pulse_app_get_subapp(app, "Sub") == sub);
    assert(pulse_app_set_subapp_extract(app, "Sub", extract_subapp, nullptr) == PULSE_OK);

    int frames = 1;
    assert(pulse_app_set_runner(app, test_runner, &frames) == PULSE_OK);
    assert(pulse_app_run(app) == PULSE_OK);

    assert(post_build_called);
    assert(shutdown_called);
    assert(sub_post_build_called);
    assert(sub_shutdown_called);
    assert(extract_called);

    pulse_app_t removed = pulse_app_remove_subapp(app, "Sub");
    assert(removed == sub);
    assert(pulse_app_get_subapp(app, "Sub") == nullptr);
    pulse_app_destroy(removed);

    pulse_app_destroy(app);

    printf("All tests passed!\n");
    return 0;
}
