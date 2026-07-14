#pragma once

#ifndef PULSE_TRANSFORM_API_HEADER_GUARD
#define PULSE_TRANSFORM_API_HEADER_GUARD

#include <stdint.h>
#include "pulse_app.h"

#include "pulse_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PULSE_API
#define PULSE_API
#endif

#define PULSE_TRANSFORM_PLUGIN_DESC_VERSION 1u










typedef struct PulsePosition
{
    HMM_Vec3             value;

} PulsePosition;

typedef struct PulseRotation
{
    HMM_Quat             value;

} PulseRotation;

typedef struct PulseLocalTransform
{
    HMM_Mat4             model;

} PulseLocalTransform;

typedef struct PulseWorldTransform
{
    HMM_Mat4             value;

} PulseWorldTransform;

typedef struct PulseShowMatrix
{
    HMM_Mat4             model;

} PulseShowMatrix;

typedef struct PulseTree
{
    ecs_entity_t         parent;
    ecs_entity_t         first_child;
    ecs_entity_t         last_child;
    ecs_entity_t         previous_sibling;
    ecs_entity_t         next_sibling;

} PulseTree;


// ECS declarations
extern ECS_COMPONENT_DECLARE(PulsePosition);
extern ECS_COMPONENT_DECLARE(PulseRotation);
extern ECS_COMPONENT_DECLARE(PulseLocalTransform);
extern ECS_COMPONENT_DECLARE(PulseWorldTransform);
extern ECS_COMPONENT_DECLARE(PulseShowMatrix);
extern ECS_COMPONENT_DECLARE(PulseTree);

PULSE_API EPulseResult pulse_add_transform_plugin(PulseAppId app);

#ifdef __cplusplus
}
#endif

#endif // PULSE_TRANSFORM_API_HEADER_GUARD
