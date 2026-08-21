#include "test_common.h"

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-app-manual",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // ---- manual lifecycle: prepare + update*n + teardown ----

    // update before prepare is invalid
    assert(pulse_app_update(app) == PULSE_RESULT_ERROR_INVALID_STATE);

    // prepare once, then update any number of frames
    assert(pulse_app_prepare(app) == PULSE_RESULT_OK);
    for (int i = 0; i < 5; ++i) {
        assert(pulse_app_update(app) == PULSE_RESULT_OK);
    }

    // prepare is single-shot, and run() cannot be mixed with the
    // manual prepare/update/teardown flow
    assert(pulse_app_prepare(app) == PULSE_RESULT_ERROR_INVALID_STATE);
    assert(pulse_app_run(app) == PULSE_RESULT_ERROR_INVALID_STATE);

    // explicit teardown ends the manual lifecycle; it is idempotent
    pulse_app_teardown(app);
    assert(pulse_app_update(app) == PULSE_RESULT_ERROR_INVALID_STATE);
    assert(pulse_app_run(app) == PULSE_RESULT_ERROR_INVALID_STATE);
    pulse_app_teardown(app);

    pulse_destroy_app(app);

    printf("Manual lifecycle test passed!\n");
    return 0;
}