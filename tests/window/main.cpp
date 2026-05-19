#include <flecs.h>

#include "pulse_app.h"
#include "pulse_window.h"

int main() {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    auto window_plugin_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_plugin_desc) == PULSE_OK);

    pulse_app_run(app);

    pulse_app_destroy(app);

    printf("All tests passed!\n");
    return 0;
}