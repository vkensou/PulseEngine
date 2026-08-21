#pragma once

#include "pulse_window.h"

#include <SDL3/SDL.h>

namespace pulse_window_internal {

struct pulse_window_plugin_state {
    PulseAppId app = nullptr;
    PulseWindowPluginDesc desc{};
    uint32_t initialized_sdl_flags = 0;
    ecs_entity_t post_frame_system = 0;
};

typedef struct pulse_window_state_resource {
    pulse_window_plugin_state* state;
} pulse_window_state_resource;

extern ECS_COMPONENT_DECLARE(pulse_window_state_resource);

void register_components(ecs_world_t* world);
pulse_window_plugin_state* state_from_world(ecs_world_t* world);
pulse_window_plugin_state* state_from_app(PulseAppId app);

ecs_entity_t install_window_post_frame_system(
    ecs_world_t* world,
    pulse_window_plugin_state* state
);

} // namespace pulse_window_internal
