#include "transform_internal.h"

#include <assert.h>
#include <string.h>

ECS_COMPONENT_DECLARE(PulsePosition);
ECS_COMPONENT_DECLARE(PulseRotation);
ECS_COMPONENT_DECLARE(PulseScale);
ECS_COMPONENT_DECLARE(PulseLocalTransform);
ECS_COMPONENT_DECLARE(PulseWorldTransform);
ECS_COMPONENT_DECLARE(PulseShowMatrix);

namespace pulse_transform_internal {
namespace {

// UpdateLocalTransform: Position (+ optional Rotation, optional Scale) -> LocalTransform
void update_local_transform(ecs_iter_t* it) {
    PulseLocalTransform* lt = ecs_field(it, PulseLocalTransform, 0);
    PulsePosition* p = ecs_field(it, PulsePosition, 1);
    if (!lt || !p) return;

    PulseRotation* r = ecs_field(it, PulseRotation, 2);
    PulseScale* s = ecs_field(it, PulseScale, 3);

    for (int i = 0; i < it->count; i++) {
        // Build local matrix: T * R * S
        HMM_Mat4 t = HMM_Translate(p[i].value);
        HMM_Mat4 rot = r ? HMM_QToM4(r[i].value) : HMM_M4D(1.0f);
        HMM_Mat4 sc = s ? HMM_Scale(s[i].value) : HMM_M4D(1.0f);
        lt[i].model = HMM_Mul(HMM_Mul(t, rot), sc);
    }
}

// PropagateWorldTransform: LocalTransform -> WorldTransform
// Uses EcsCascade on the parent WorldTransform term to automatically
// traverse (ChildOf, *) and guarantee parent-before-child order.
void propagate_world_transform(ecs_iter_t* it) {
    PulseWorldTransform* wt_out = ecs_field(it, PulseWorldTransform, 0);
    PulseWorldTransform* wt_parent = ecs_field(it, PulseWorldTransform, 1);
    PulseLocalTransform* local = ecs_field(it, PulseLocalTransform, 2);
    if (!wt_out || !local) return;

    for (int i = 0; i < it->count; i++) {
        if (wt_parent) {
            // wt_parent is shared (EcsCascade source), always index [0]
            wt_out[i].value = HMM_Mul(wt_parent[0].value, local[i].model);
        } else {
            // Root entity (no ChildOf) — world = local
            wt_out[i].value = local[i].model;
        }
    }
}

} // anonymous namespace

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, PulsePosition);
    ECS_COMPONENT_DEFINE(world, PulseRotation);
    ECS_COMPONENT_DEFINE(world, PulseScale);
    ECS_COMPONENT_DEFINE(world, PulseLocalTransform);
    ECS_COMPONENT_DEFINE(world, PulseWorldTransform);
    ECS_COMPONENT_DEFINE(world, PulseShowMatrix);

    // Auto-insertion (EcsWith):
    //   Adding any of Position/Rotation/Scale automatically ensures both
    //   LocalTransform and WorldTransform are present.
    ecs_add_pair(world, ecs_id(PulsePosition), EcsWith, ecs_id(PulseLocalTransform));
    ecs_add_pair(world, ecs_id(PulseRotation), EcsWith, ecs_id(PulseLocalTransform));
    ecs_add_pair(world, ecs_id(PulseScale),    EcsWith, ecs_id(PulseLocalTransform));
    ecs_add_pair(world, ecs_id(PulseLocalTransform), EcsWith, ecs_id(PulseWorldTransform));
}

void install_transform_systems(ecs_world_t* world) {
    // UpdateLocalTransform: Position (+ optional Rotation, Scale) -> LocalTransform
    {
        ecs_entity_desc_t entity_desc;
        memset(&entity_desc, 0, sizeof(entity_desc));
        entity_desc.name = "UpdateLocalTransform";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc;
        memset(&desc, 0, sizeof(desc));
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.query.cache_kind = EcsQueryCacheNone;
        desc.query.terms[0].id = ecs_id(PulseLocalTransform);
        desc.query.terms[1].id = ecs_id(PulsePosition);
        desc.query.terms[2].id = ecs_id(PulseRotation);
        desc.query.terms[2].oper = EcsOptional;
        desc.query.terms[3].id = ecs_id(PulseScale);
        desc.query.terms[3].oper = EcsOptional;
        desc.callback = update_local_transform;
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
