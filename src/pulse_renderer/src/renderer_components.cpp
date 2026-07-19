#include "renderer_internal.h"

#include <string.h>

ECS_COMPONENT_DECLARE(PulseCamera);
ECS_COMPONENT_DECLARE(PulseLight);
ECS_COMPONENT_DECLARE(PulseRenderable);

namespace pulse_renderer_internal {

void register_renderer_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, PulseCamera);
    ECS_COMPONENT_DEFINE(world, PulseLight);
    ECS_COMPONENT_DEFINE(world, PulseRenderable);
}

} // namespace pulse_renderer_internal
