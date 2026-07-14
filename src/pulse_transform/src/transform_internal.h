#pragma once

#include "pulse_transform.h"

namespace pulse_transform_internal {

struct pulse_transform_plugin_state {
    PulseAppId app = nullptr;
};

void register_components(ecs_world_t* world);
void install_transform_systems(ecs_world_t* world);

} // namespace pulse_transform_internal
