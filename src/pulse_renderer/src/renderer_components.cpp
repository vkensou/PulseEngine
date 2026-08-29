#include <flecs.h>
#include "renderer_internal.h"

#include <string.h>

ECS_COMPONENT_DECLARE(PulseCamera);
ECS_COMPONENT_DECLARE(PulseLight);
ECS_COMPONENT_DECLARE(PulseRenderable);
ECS_COMPONENT_DECLARE(pulse_renderer_state_resource);

namespace pulse_renderer_internal {

void register_renderer_components(ecs_world_t* world) {
    ecs_id(PulseCamera) = flecs::_::type<PulseCamera>::id(world);
    ecs_id(PulseLight) = flecs::_::type<PulseLight>::id(world);
    ecs_id(PulseRenderable) = flecs::_::type<PulseRenderable>::id(world);
    ecs_id(pulse_renderer_state_resource) = flecs::_::type<pulse_renderer_state_resource>::id(world);
}

pulse_renderer_state* state_from_app(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return nullptr;
    const pulse_renderer_state_resource* res = ecs_singleton_get(world, pulse_renderer_state_resource);
    return res ? res->state : nullptr;
}

} // namespace pulse_renderer_internal
