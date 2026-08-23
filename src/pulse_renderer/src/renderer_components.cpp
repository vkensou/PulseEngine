#include <flecs.h>
#include "renderer_internal.h"

#include <string.h>

ECS_COMPONENT_DECLARE(PulseCamera);
ECS_COMPONENT_DECLARE(PulseLight);
ECS_COMPONENT_DECLARE(PulseRenderable);
ECS_COMPONENT_DECLARE(pulse_renderer_state_resource);

namespace pulse_renderer_internal {

void register_renderer_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, PulseCamera);
    ECS_COMPONENT_DEFINE(world, PulseLight);
    ECS_COMPONENT_DEFINE(world, PulseRenderable);
    ECS_COMPONENT_DEFINE(world, pulse_renderer_state_resource);

    // Bind C++ types to the C component ids so gameplay modules can use flecs C++ API.
    flecs::world world_view(world);
    world_view.component<PulseCamera>("PulseCamera", true, ecs_id(PulseCamera));
    world_view.component<PulseLight>("PulseLight", true, ecs_id(PulseLight));
    world_view.component<PulseRenderable>("PulseRenderable", true, ecs_id(PulseRenderable));
}

pulse_renderer_state* state_from_app(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return nullptr;
    const pulse_renderer_state_resource* res = ecs_singleton_get(world, pulse_renderer_state_resource);
    return res ? res->state : nullptr;
}

} // namespace pulse_renderer_internal
