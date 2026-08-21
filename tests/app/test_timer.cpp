#include "test_common.h"

int main(void) {
    // --- Timer: singleton exists and is refreshed every frame ---
    PulseAppDesc app_desc = {
        .name = "test-app-timer",
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

    printf("Timer test passed!\n");
    return 0;
}