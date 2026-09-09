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

    flecs::untyped_component camera(world, ecs_id(PulseCamera));
    camera.member("window_entity", &PulseCamera::window_entity);
    camera.member("fov", &PulseCamera::fov).range(0.0, 180.0);
    camera.member("near_plane", &PulseCamera::near_plane).range(0.0, 1000.0);
    camera.member("far_plane", &PulseCamera::far_plane).range(0.0, 100000.0);

    flecs::untyped_component light(world, ecs_id(PulseLight));
    light.member("color", &PulseLight::color);

    ecs_entity_t ecs_id(PulseMeshHandle) = flecs::_::type<PulseMeshHandle>::id(world);
    flecs::untyped_component mesh_handle(world, ecs_id(PulseMeshHandle));
    mesh_handle.member("index", &PulseMeshHandle::index);
    mesh_handle.member("generation", &PulseMeshHandle::generation);

    ecs_entity_t ecs_id(PulseMaterialHandle) = flecs::_::type<PulseMaterialHandle>::id(world);
    flecs::untyped_component material_handle(world, ecs_id(PulseMaterialHandle));
    material_handle.member("index", &PulseMeshHandle::index);
    material_handle.member("generation", &PulseMeshHandle::generation);

    flecs::untyped_component renderable(world, ecs_id(PulseRenderable));
    renderable.member("mesh", &PulseRenderable::mesh);
    renderable.member("material", &PulseRenderable::material);
}

pulse_renderer_state* state_from_app(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return nullptr;
    const pulse_renderer_state_resource* res = ecs_singleton_get(world, pulse_renderer_state_resource);
    return res ? res->state : nullptr;
}

} // namespace pulse_renderer_internal
