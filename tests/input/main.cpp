#include <assert.h>
#include <stdio.h>

#include <flecs.h>

#include "pulse_app.h"
#include "pulse_input.h"

int main() {
    PulseAppId app = pulse_create_app("test-input");
    assert(app != nullptr);

    // Add input plugin (no desc needed)
    assert(pulse_add_input_plugin(app) == PULSE_RESULT_OK);

    // Adding again should fail with DUPLICATE
    assert(pulse_add_input_plugin(app) == PULSE_RESULT_ERROR_DUPLICATE_PLUGIN);

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

    printf("All input tests passed!\n");
    return 0;
}
