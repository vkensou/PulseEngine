#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_input.h"

int main() {
    PulseAppDesc app_desc = {
        .name = "test-input-api",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // API functions should be callable (values depend on actual input state)
    bool key_down = pulse_input_is_key_down(app, 0);       // SDL_SCANCODE_UNKNOWN
    assert(key_down == false);  // unknown key should be not pressed

    pulse_input_get_mouse_position(app, NULL, NULL);  // null pointers should be safe

    float mx = -1.0f, my = -1.0f;
    pulse_input_get_mouse_position(app, &mx, &my);

    float dx = 0.0f, dy = 0.0f;
    pulse_input_get_mouse_delta(app, &dx, &dy);

    float sx = 0.0f, sy = 0.0f;
    pulse_input_get_mouse_scroll(app, &sx, &sy);

    pulse_destroy_app(app);
    return 0;
}
