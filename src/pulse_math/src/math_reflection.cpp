#include "math_internal.h"

namespace pulse_math_internal {

void register_reflection(ecs_world_t* world) {
    ecs_entity_t ecs_id(HMM_Vec2) = flecs::_::type<HMM_Vec2>::id(world);
    flecs::untyped_component vec2(world, ecs_id(HMM_Vec2));
    vec2.member("X", &HMM_Vec2::X);
    vec2.member("Y", &HMM_Vec2::Y);

    ecs_entity_t ecs_id(HMM_Vec3) = flecs::_::type<HMM_Vec3>::id(world);
    flecs::untyped_component vec3(world, ecs_id(HMM_Vec3));
    vec3.member("X", &HMM_Vec3::X);
    vec3.member("Y", &HMM_Vec3::Y);
    vec3.member("Z", &HMM_Vec3::Z);

    ecs_entity_t ecs_id(HMM_Vec4) = flecs::_::type<HMM_Vec4>::id(world);
    flecs::untyped_component vec4(world, ecs_id(HMM_Vec4));
    vec4.member("X", &HMM_Vec4::X);
    vec4.member("Y", &HMM_Vec4::Y);
    vec4.member("Z", &HMM_Vec4::Z);
    vec4.member("W", &HMM_Vec4::W);

    ecs_entity_t ecs_id(HMM_Mat4) = flecs::_::type<HMM_Mat4>::id(world);
    flecs::untyped_component mat4(world, ecs_id(HMM_Mat4));
    mat4.member("Columns", &HMM_Mat4::Columns);

    ecs_entity_t ecs_id(HMM_Quat) = flecs::_::type<HMM_Quat>::id(world);
    flecs::untyped_component quat(world, ecs_id(HMM_Quat));
    quat.member("X", &HMM_Quat::X);
    quat.member("Y", &HMM_Quat::Y);
    quat.member("Z", &HMM_Quat::Z);
    quat.member("W", &HMM_Quat::W);
}

} // namespace pulse_math_internal
