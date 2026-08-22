#include <assert.h>
#include <stdio.h>

#include <flecs.h>

#include "pulse_app.h"
#include "pulse_input.h"

int main() {
    PulseAppDesc app_desc = {
        .name = "test-input-plugin",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // Add input plugin (no desc needed)
    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_has_plugin(app, "PulseInputPlugin"));

    // Adding again should fail with DUPLICATE
    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN);

    pulse_destroy_app(app);
    return 0;
}
