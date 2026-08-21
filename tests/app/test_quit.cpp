#include "test_common.h"

int main(void) {
    // ---- finish / should_quit ----
    PulseAppDesc quit_desc = {
        .name = "test-app-quit",
    };
    PulseAppId quit_app = pulse_create_app(&quit_desc);
    assert(quit_app != nullptr);
    assert(!pulse_app_should_quit(quit_app));
    pulse_app_finish(quit_app);
    assert(pulse_app_should_quit(quit_app));
    pulse_destroy_app(quit_app);

    printf("Quit test passed!\n");
    return 0;
}