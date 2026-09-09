#include "transform_internal.h"

#include <assert.h>
#include <string.h>

ECS_COMPONENT_DECLARE(PulseLocalTransform);
ECS_COMPONENT_DECLARE(PulseWorldTransform);
ECS_COMPONENT_DECLARE(PulseShowMatrix);

namespace pulse_transform_internal {
namespace {

// EnsureWorldTransform: repair entities that have LocalTransform but are
// missing WorldTransform (e.g. due to manual ecs_remove).
// Runs before PropagateWorldTransform in the same PostUpdate phase.
void ensure_world_transform(ecs_iter_t* it) {
    PulseLocalTransform* local = ecs_field(it, PulseLocalTransform, 0);
    if (!local) return;

    for (int i = 0; i < it->count; i++) {
        HMM_Mat4 local_mat = HMM_TRS(
            local[i].translation,
            local[i].rotation,
            local[i].scale);
        PulseWorldTransform wt;
        wt.value = local_mat;
        ecs_set_ptr(it->world, it->entities[i], PulseWorldTransform, &wt);
    }
}

// PropagateWorldTransform: LocalTransform -> WorldTransform
// Computes the local matrix from PulseLocalTransform's TRS fields on the fly,
// then propagates through the hierarchy via EcsCascade.
// Uses EcsCascade on the parent WorldTransform term to automatically
// traverse (ChildOf, *) and guarantee parent-before-child order.
void propagate_world_transform(ecs_iter_t* it) {
    PulseWorldTransform* wt_out = ecs_field(it, PulseWorldTransform, 0);
    PulseWorldTransform* wt_parent = ecs_field(it, PulseWorldTransform, 1);
    PulseLocalTransform* local = ecs_field(it, PulseLocalTransform, 2);
    if (!wt_out || !local) return;

    for (int i = 0; i < it->count; i++) {
        // Compute local matrix from TRS: T * R * S
        const PulseLocalTransform& local_transform = local[i];
        HMM_Mat4 local_mat = HMM_TRS(local_transform.translation, local_transform.rotation, local_transform.scale);

        if (wt_parent) {
            // wt_parent is shared (EcsCascade source), always index [0]
            wt_out[i].value = HMM_Mul(wt_parent[0].value, local_mat);
        } else {
            // Root entity (no ChildOf) — world = local
            wt_out[i].value = local_mat;
        }
    }
}

} // anonymous namespace

void register_components(ecs_world_t* world) {
    ecs_id(PulseLocalTransform) = flecs::_::type<PulseLocalTransform>::id(world);
    ecs_id(PulseWorldTransform) = flecs::_::type<PulseWorldTransform>::id(world);
    ecs_id(PulseShowMatrix) = flecs::_::type<PulseShowMatrix>::id(world);

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

    flecs::untyped_component local_transform(world, ecs_id(PulseLocalTransform));
    local_transform.member("translation", &PulseLocalTransform::translation);
    local_transform.member("rotation", &PulseLocalTransform::rotation);
    local_transform.member("scale", &PulseLocalTransform::scale);

    flecs::untyped_component world_transform(world, ecs_id(PulseWorldTransform));
    world_transform.member("value", &PulseWorldTransform::value);

    flecs::untyped_component show_matrix(world, ecs_id(PulseShowMatrix));
    show_matrix.member("model", &PulseShowMatrix::model);

    // Auto-insertion (EcsWith):
    //   Adding LocalTransform automatically ensures WorldTransform is present.
    ecs_add_pair(world, ecs_id(PulseLocalTransform), EcsWith, ecs_id(PulseWorldTransform));
}

void install_transform_systems(ecs_world_t* world) {
    // EnsureWorldTransform: repair entities that have LocalTransform but
    // are missing WorldTransform. Runs before PropagateWorldTransform.
    {
        ecs_entity_desc_t entity_desc;
        memset(&entity_desc, 0, sizeof(entity_desc));
        entity_desc.name = "EnsureWorldTransform";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc;
        memset(&desc, 0, sizeof(desc));
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        // Term 0: this entity's LocalTransform (read)
        desc.query.terms[0].id = ecs_id(PulseLocalTransform);
        desc.query.terms[0].inout = EcsIn;
        // Term 1: must NOT have WorldTransform — filters out intact entities
        desc.query.terms[1].id = ecs_id(PulseWorldTransform);
        desc.query.terms[1].oper = EcsNot;
        desc.callback = ensure_world_transform;
        ecs_system_init(world, &desc);
    }

    // PropagateWorldTransform: LocalTransform -> WorldTransform
    // EcsCascade guarantees parent-before-child iteration order via (ChildOf, *).
    {
        ecs_entity_desc_t entity_desc;
        memset(&entity_desc, 0, sizeof(entity_desc));
        entity_desc.name = "PropagateWorldTransform";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc;
        memset(&desc, 0, sizeof(desc));
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        // cache_kind defaults → Auto (EcsCascade requires caching)
        // Term 0: output — this entity's WorldTransform
        desc.query.terms[0].id = ecs_id(PulseWorldTransform);
        desc.query.terms[0].inout = EcsOut;
        // Term 1: input — parent's WorldTransform via ChildOf cascade (optional for roots)
        desc.query.terms[1].id = ecs_id(PulseWorldTransform);
        desc.query.terms[1].inout = EcsIn;
        desc.query.terms[1].oper = EcsOptional;
        desc.query.terms[1].src.id = EcsCascade;  // implies EcsUp, default trav=ChildOf
        // Term 2: input — this entity's LocalTransform
        desc.query.terms[2].id = ecs_id(PulseLocalTransform);
        desc.query.terms[2].inout = EcsIn;
        desc.callback = propagate_world_transform;
        ecs_system_init(world, &desc);
    }
}

} // namespace pulse_transform_internal
