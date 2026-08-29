#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_input.h"

int main() {
    PulseAppDesc app_desc = {
        .name = "test-input-components",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    ecs_world_t* world = pulse_app_world(app);
    assert(world != nullptr);

    // Event components should be registered
    assert(ecs_id(PulseKeyEvent) != 0);
    assert(ecs_id(PulseMouseButtonEvent) != 0);

    // Component sizes should match our structs
    assert(ecs_get_type_info(world, ecs_id(PulseKeyEvent)) != nullptr);
    assert(ecs_get_type_info(world, ecs_id(PulseKeyEvent))->size == sizeof(PulseKeyEvent));
    assert(ecs_get_type_info(world, ecs_id(PulseMouseButtonEvent)) != nullptr);
    assert(ecs_get_type_info(world, ecs_id(PulseMouseButtonEvent))->size == sizeof(PulseMouseButtonEvent));

    pulse_destroy_app(app);
    return 0;
}
