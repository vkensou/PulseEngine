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










typedef struct PulseLocalTransform
{
    HMM_Vec3             translation;
    HMM_Quat             rotation;
    HMM_Vec3             scale;

} PulseLocalTransform;

typedef struct PulseWorldTransform
{
    HMM_Mat4             value;

} PulseWorldTransform;

typedef struct PulseShowMatrix
{
    HMM_Mat4             model;

} PulseShowMatrix;


// ECS declarations
extern ECS_COMPONENT_DECLARE(PulseLocalTransform);
extern ECS_COMPONENT_DECLARE(PulseWorldTransform);
extern ECS_COMPONENT_DECLARE(PulseShowMatrix);

PULSE_API EPulseAppAddPluginResult pulse_add_transform_plugin(PulseAppId app);
PULSE_API void pulse_set_parent(PulseAppId app, ecs_entity_t child, ecs_entity_t parent);
PULSE_API void pulse_remove_parent(PulseAppId app, ecs_entity_t child);
PULSE_API ecs_entity_t pulse_get_parent(PulseAppId app, ecs_entity_t child);

#ifdef __cplusplus
}
#endif

#endif // PULSE_TRANSFORM_API_HEADER_GUARD
