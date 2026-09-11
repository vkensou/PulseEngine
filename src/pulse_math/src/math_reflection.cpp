#include "math_internal.h"

namespace pulse_math_internal {

void register_reflection(ecs_world_t* world) {
    flecs::component<HMM_Vec2> vec2(world, "HMM_Vec2");
    vec2.member("X", &HMM_Vec2::X);
    vec2.member("Y", &HMM_Vec2::Y);

    flecs::component<HMM_Vec3> vec3(world, "HMM_Vec3");
    vec3.member("X", &HMM_Vec3::X);
    vec3.member("Y", &HMM_Vec3::Y);
    vec3.member("Z", &HMM_Vec3::Z);

    flecs::component<HMM_Vec4> vec4(world, "HMM_Vec4");
    vec4.member("X", &HMM_Vec4::X);
    vec4.member("Y", &HMM_Vec4::Y);
    vec4.member("Z", &HMM_Vec4::Z);
    vec4.member("W", &HMM_Vec4::W);

    flecs::component<HMM_Mat2> mat2(world, "HMM_Mat2");
    mat2.member("Columns", &HMM_Mat2::Columns);

    flecs::component<HMM_Mat3> mat3(world, "HMM_Mat3");
    mat3.member("Columns", &HMM_Mat3::Columns);

    flecs::component<HMM_Mat4> mat4(world, "HMM_Mat4");
    mat4.member("Columns", &HMM_Mat4::Columns);

    flecs::component<HMM_Quat> quat(world, "HMM_Quat");
    quat.member("X", &HMM_Quat::X);
    quat.member("Y", &HMM_Quat::Y);
    quat.member("Z", &HMM_Quat::Z);
    quat.member("W", &HMM_Quat::W);
}

} // namespace pulse_math_internal
