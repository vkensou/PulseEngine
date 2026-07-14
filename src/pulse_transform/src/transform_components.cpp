#include "transform_internal.h"

#include <string.h>

#include "pulse_math.h"

ECS_COMPONENT_DECLARE(PulsePosition);
ECS_COMPONENT_DECLARE(PulseRotation);
ECS_COMPONENT_DECLARE(PulseLocalTransform);
ECS_COMPONENT_DECLARE(PulseWorldTransform);
ECS_COMPONENT_DECLARE(PulseShowMatrix);
ECS_COMPONENT_DECLARE(PulseTree);

namespace pulse_transform_internal {
namespace {

// UpdateLocalTransform: Position (+ optional Rotation) -> LocalTransform
void update_local_transform(ecs_iter_t* it) {
    PulsePosition* p = ecs_field(it, PulsePosition, 0);
    if (!p) return;

    PulseRotation* r = ecs_field(it, PulseRotation, 1);

    for (int i = 0; i < it->count; i++) {
        PulseLocalTransform lt_data;
        lt_data.model = HMM_Translate(p[i].value);
        if (r) {
            lt_data.model = HMM_Mul(lt_data.model, HMM_QToM4(r[i].value));
        }
        ecs_set_id(it->world, it->entities[i],
                   ecs_id(PulseLocalTransform), sizeof(PulseLocalTransform), &lt_data);
    }
}

// PropagateWorldTransform: Tree + LocalTransform -> WorldTransform
void propagate_world_transform(ecs_iter_t* it) {
    PulseTree* tree = ecs_field(it, PulseTree, 0);
    PulseLocalTransform* local = ecs_field(it, PulseLocalTransform, 1);
    if (!tree || !local) return;

    for (int i = 0; i < it->count; i++) {
        PulseWorldTransform wt_data;

        if (tree[i].parent == 0) {
            wt_data.value = local[i].model;
        } else {
            const PulseWorldTransform* pw = ecs_get(it->world, tree[i].parent, PulseWorldTransform);
            if (pw) {
                wt_data.value = HMM_Mul(pw->value, local[i].model);
            } else {
                wt_data.value = local[i].model;
            }
        }
        ecs_set_id(it->world, it->entities[i],
                   ecs_id(PulseWorldTransform), sizeof(PulseWorldTransform), &wt_data);
    }
}

} // anonymous namespace

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, PulsePosition);
    ECS_COMPONENT_DEFINE(world, PulseRotation);
    ECS_COMPONENT_DEFINE(world, PulseLocalTransform);
    ECS_COMPONENT_DEFINE(world, PulseWorldTransform);
    ECS_COMPONENT_DEFINE(world, PulseShowMatrix);
    ECS_COMPONENT_DEFINE(world, PulseTree);
}

void install_transform_systems(ecs_world_t* world) {
    // UpdateLocalTransform: PulsePosition (+ optional PulseRotation) -> PulseLocalTransform
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
        desc.query.terms[0].id = ecs_id(PulsePosition);
        desc.query.terms[1].id = ecs_id(PulseRotation);
        desc.query.terms[1].oper = EcsOptional;
        desc.callback = update_local_transform;
        desc.immediate = true;
        ecs_system_init(world, &desc);
    }

    // PropagateWorldTransform: Tree + LocalTransform -> WorldTransform
    {
        ecs_entity_desc_t entity_desc;
        memset(&entity_desc, 0, sizeof(entity_desc));
        entity_desc.name = "PropagateWorldTransform";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc;
        memset(&desc, 0, sizeof(desc));
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.query.cache_kind = EcsQueryCacheNone;
        desc.query.terms[0].id = ecs_id(PulseTree);
        desc.query.terms[1].id = ecs_id(PulseLocalTransform);
        desc.callback = propagate_world_transform;
        desc.immediate = true;
        ecs_system_init(world, &desc);
    }
}

} // namespace pulse_transform_internal
