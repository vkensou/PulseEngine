#pragma once

#include "pulse_input.h"

#include <SDL3/SDL.h>

namespace pulse_input_internal {

struct pulse_input_plugin_state {
    PulseAppId app = nullptr;
    bool prev_keyboard[PULSE_SCANCODE_COUNT]{};
    uint8_t prev_mouse;
};

typedef struct pulse_input_state_resource {
    pulse_input_plugin_state* state;
} pulse_input_state_resource;

extern ECS_COMPONENT_DECLARE(pulse_input_state_resource);

void register_components(ecs_world_t* world);
pulse_input_plugin_state* state_from_world(ecs_world_t* world);
pulse_input_plugin_state* state_from_app(PulseAppId app);

ecs_entity_t install_input_system(
    ecs_world_t* world,
    pulse_input_plugin_state* state
);

} // namespace pulse_input_internal
