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

typedef struct PulseScale
{
    HMM_Vec3             value;

} PulseScale;

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


// ECS declarations
extern ECS_COMPONENT_DECLARE(PulsePosition);
extern ECS_COMPONENT_DECLARE(PulseRotation);
extern ECS_COMPONENT_DECLARE(PulseScale);
extern ECS_COMPONENT_DECLARE(PulseLocalTransform);
extern ECS_COMPONENT_DECLARE(PulseWorldTransform);
extern ECS_COMPONENT_DECLARE(PulseShowMatrix);

PULSE_API EPulseResult pulse_add_transform_plugin(PulseAppId app);
PULSE_API void pulse_set_parent(PulseAppId app, ecs_entity_t child, ecs_entity_t parent);

/**
 * 移除实体的父级关系
 *
 * @param[in] app
 * @param[in] child
 *
 */
PULSE_API void pulse_remove_parent(PulseAppId app, ecs_entity_t child);

/**
 * 获取实体的父实体（无父级返回 0）
 *
 * @param[in] app
 * @param[in] child
 *
 */
PULSE_API ecs_entity_t pulse_get_parent(PulseAppId app, ecs_entity_t child);

#ifdef __cplusplus
}
#endif

#endif // PULSE_TRANSFORM_API_HEADER_GUARD
