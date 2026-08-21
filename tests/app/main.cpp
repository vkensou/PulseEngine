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
    PulseAppDesc app_desc = {
        .name = "test-app",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // ---- eager build: build runs immediately on add ----
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

    // ---- subapp ----
    app_desc = {
        .name = "Sub",
    };
    PulseAppId sub = pulse_create_app(&app_desc);
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

    // ---- subapp removal is only valid before run ----
    app_desc = {
        .name = "Sub2",
    };
    PulseAppId sub2 = pulse_create_app(&app_desc);
    assert(pulse_app_insert_subapp(app, "Sub2", sub2) == PULSE_RESULT_OK);
    PulseAppId removed = pulse_app_remove_subapp(app, "Sub2");
    assert(removed == sub2);
    assert(pulse_app_get_subapp(app, "Sub2") == nullptr);
    pulse_destroy_app(removed);

    // ---- run: single-shot, auto-teardown ----
    int frames = 1;
    assert(pulse_app_set_runner(app, test_runner, &frames) == PULSE_RESULT_OK);
    assert(pulse_app_run(app) == PULSE_RESULT_OK);

    assert(post_build_called);
    assert(sub_post_build_called);
    assert(extract_called);
    // auto-teardown: everything is released when run() returns
    assert(shutdown_called);
    assert(sub_shutdown_called);

    // ---- single-shot: cannot run or update again ----
    assert(pulse_app_run(app) == PULSE_RESULT_ERROR_INVALID_STATE);
    assert(pulse_app_update(app) == PULSE_RESULT_ERROR_INVALID_STATE);

    pulse_destroy_app(app);

    // ---- manual lifecycle: prepare + update*n + teardown ----
    {
        PulseAppDesc manual_desc = {
            .name = "manual-app",
        };
        PulseAppId manual = pulse_create_app(&manual_desc);
        assert(manual != nullptr);

        // update before prepare is invalid
        assert(pulse_app_update(manual) == PULSE_RESULT_ERROR_INVALID_STATE);

        // prepare once, then update any number of frames
        assert(pulse_app_prepare(manual) == PULSE_RESULT_OK);
        for (int i = 0; i < 5; ++i) {
            assert(pulse_app_update(manual) == PULSE_RESULT_OK);
        }

        // prepare is single-shot, and run() cannot be mixed with the
        // manual prepare/update/teardown flow
        assert(pulse_app_prepare(manual) == PULSE_RESULT_ERROR_INVALID_STATE);
        assert(pulse_app_run(manual) == PULSE_RESULT_ERROR_INVALID_STATE);

        // explicit teardown ends the manual lifecycle; it is idempotent
        pulse_app_teardown(manual);
        assert(pulse_app_update(manual) == PULSE_RESULT_ERROR_INVALID_STATE);
        assert(pulse_app_run(manual) == PULSE_RESULT_ERROR_INVALID_STATE);
        pulse_app_teardown(manual);

        pulse_destroy_app(manual);
    }

    // ---- finish / should_quit ----
    {
        PulseAppDesc quit_desc = {
            .name = "quit-app",
        };
        PulseAppId quit_app = pulse_create_app(&quit_desc);
        assert(!pulse_app_should_quit(quit_app));
        pulse_app_finish(quit_app);
        assert(pulse_app_should_quit(quit_app));
        pulse_destroy_app(quit_app);
    }

    // --- Timer: singleton exists and is refreshed every frame ---
    {
    PulseAppDesc app_desc = {
        .name = "time-app",
    };
        PulseAppId time_app = pulse_create_app(&app_desc);
        assert(time_app != nullptr);

        ecs_world_t* world = pulse_app_world(time_app);
        assert(world != nullptr);

        PulseAppId rev_app = pulse_get_app_from_world(world);
        assert(rev_app != nullptr);

        // Singleton must exist from the start, with zeroed time
        const PulseTimer* ctx = ecs_singleton_get(world, PulseTimer);
        assert(ctx != nullptr);
        assert(ctx->time_since_startup_double == 0.0);

        // Run a few frames; time must advance
        int frames = 10;
        assert(pulse_app_set_runner(time_app, test_runner, &frames) == PULSE_RESULT_OK);
        assert(pulse_app_run(time_app) == PULSE_RESULT_OK);

        ctx = ecs_singleton_get(world, PulseTimer);
        assert(ctx != nullptr);
        assert(ctx->time_since_startup_double > 0.0);
        assert(ctx->delta_time_double >= 0.0);
        assert(ctx->time_since_startup > 0.0f);
        assert(ctx->fps >= 0);

        pulse_destroy_app(time_app);
    }

    printf("All tests passed!\n");
    return 0;
}
