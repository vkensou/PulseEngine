#pragma once

#ifndef PULSE_TRANSFORM_API_HEADER_GUARD
#define PULSE_TRANSFORM_API_HEADER_GUARD
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-attributes"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wattributes"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:5030)
#endif

#include <stdint.h>
#include "pulse_platform.h"
#include "pulse_app.h"

#include "pulse_math.h"

#if defined(PULSE_TRANSFORM_MODULE_BUILD)
#  define PULSE_TRANSFORM_API PULSE_EXPORT
#else
#  define PULSE_TRANSFORM_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_TRANSFORM_PLUGIN_DESC_VERSION 1u












typedef struct PulseLocalTransform
{
    HMM_Vec3             translation;
    HMM_Quat             rotation;
    HMM_Vec3             scale;

} PulseLocalTransform;
PULSE_TRANSFORM_API extern ECS_COMPONENT_DECLARE(PulseLocalTransform);

typedef struct PulseWorldTransform
{
    HMM_Mat4             value;

} PulseWorldTransform;
PULSE_TRANSFORM_API extern ECS_COMPONENT_DECLARE(PulseWorldTransform);

typedef struct PulseShowMatrix
{
    HMM_Mat4             model;

} PulseShowMatrix;
PULSE_TRANSFORM_API extern ECS_COMPONENT_DECLARE(PulseShowMatrix);


PULSE_TRANSFORM_API EPulseAppAddPluginResult pulse_add_transform_plugin(PulseAppId app);
PULSE_TRANSFORM_API void pulse_set_parent(PulseAppId app, ecs_entity_t child, ecs_entity_t parent);
PULSE_TRANSFORM_API void pulse_remove_parent(PulseAppId app, ecs_entity_t child);
PULSE_TRANSFORM_API ecs_entity_t pulse_get_parent(PulseAppId app, ecs_entity_t child);

#ifdef __cplusplus
}
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
#endif // PULSE_TRANSFORM_API_HEADER_GUARD
