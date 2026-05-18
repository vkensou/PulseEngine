#include <stdio.h>
#include <assert.h>

#define flecs_STATIC
#include <flecs.h>

#include "pulse_app.h"

static bool build_called = false;
static bool ready_called = false;
static bool finish_called = false;
static bool cleanup_called = false;
static bool system_called = false;
static int resource_value = 0;

static void my_system_callback(ecs_iter_t* it) {
    system_called = true;
    int* val = ecs_field(it, int, 1);
    if (val) *val += 1;
}

static void my_build(pulse_app_t app, void* ctx) {
    build_called = true;
    ecs_world_t* world = pulse_app_world(app);

    ecs_entity_t int_type = ecs_component(world, &(ecs_component_desc_t){
        .entity = ecs_new(world),
        .type = { .size = sizeof(int), .alignment = ECS_ALIGNOF(int) }
    });

    int initial = 0;
    pulse_app_insert_resource(app, int_type, sizeof(int), &initial);

    ecs_entity_t sys = ecs_system(world, &(ecs_system_desc_t){
        .query = { .terms = {
            { .id = int_type, .inout = EcsInOut },
        }},
        .callback = my_system_callback,
    });
    (void)sys;
}

static bool my_ready(pulse_app_t app, void* ctx) {
    ready_called = true;
    return true;
}

static void my_finish(pulse_app_t app, void* ctx) {
    finish_called = true;
}

static void my_cleanup(pulse_app_t app, void* ctx) {
    cleanup_called = true;
}

static void test_runner(pulse_app_t app, void* ctx) {
    int* frames = (int*)ctx;
    for (int i = 0; i < *frames; ++i) {
        pulse_app_update(app);
    }
}

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != 0);

    pulse_plugin_desc desc = {
        .build = my_build,
        .ready = my_ready,
        .finish = my_finish,
        .cleanup = my_cleanup,
        .name = "TestPlugin",
    };
    pulse_app_add_plugin(app, &desc);

    assert(build_called);

    int frames = 1;
    pulse_app_set_runner(app, test_runner, &frames);

    pulse_app_run(app);

    assert(ready_called);
    assert(finish_called);
    assert(cleanup_called);
    assert(system_called);

    assert(pulse_app_is_plugin_added(app, "TestPlugin"));
    assert(pulse_app_plugins_state(app) == 3);

    pulse_app_update(app);
    assert(system_called);

    pulse_app_t sub = pulse_subapp_create("Sub");
    assert(sub != 0);
    pulse_app_insert_subapp(app, "Sub", sub);
    pulse_app_t got = pulse_app_get_subapp(app, "Sub");
    assert(got == sub);
    pulse_app_remove_subapp(app, "Sub");
    assert(pulse_app_get_subapp(app, "Sub") == 0);
    pulse_subapp_destroy(sub);

    pulse_app_destroy(app);

    printf("All tests passed!\n");
    return 0;
}
