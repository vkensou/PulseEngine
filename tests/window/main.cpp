#include <flecs.h>

#include "pulse_app.h"
#include "pulse_window.h"

int main() {
    PulseAppId app = pulse_create_app("test-window");
    assert(app != nullptr);

    auto window_plugin_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_plugin_desc) == PULSE_RESULT_OK);

    pulse_app_run(app);

    pulse_destroy_app(app);

    printf("All tests passed!\n");
    return 0;
}