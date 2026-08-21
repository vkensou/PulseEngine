#include <flecs.h>

#include "pulse_app.h"
#include "pulse_input.h"
#include "pulse_window.h"
#include <cstring>

int main() {
    PulseAppDesc app_desc = {
        .name = "test-window",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    assert(pulse_add_input_plugin(app) == PULSE_RESULT_OK);

    auto window_plugin_desc = pulse_window_plugin_desc_default();
    assert(pulse_add_window_plugin(app, &window_plugin_desc) == PULSE_RESULT_OK);

    // Runtime title changes: the component owns the string, invalid
    // entities/titles are rejected.
    ecs_entity_t primary = pulse_window_get_primary(app);
    assert(primary != 0);
    assert(pulse_window_set_title(app, primary, "test-window retitled") == PULSE_RESULT_OK);
    assert(pulse_window_set_title(app, 0, "x") == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
    assert(pulse_window_set_title(app, primary, nullptr) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);

    const PulseWindow* window = ecs_get(pulse_app_world(app), primary, PulseWindow);
    assert(window && window->title && std::strcmp(window->title, "test-window retitled") == 0);

    pulse_app_run(app);

    pulse_destroy_app(app);

    printf("All tests passed!\n");
    return 0;
}
