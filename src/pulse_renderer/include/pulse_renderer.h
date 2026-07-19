#pragma once

#ifndef PULSE_RENDERER_API_HEADER_GUARD
#define PULSE_RENDERER_API_HEADER_GUARD

#include <stdint.h>
#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PULSE_API
#define PULSE_API
#endif

/**
 * Constants
 *
 */
#define PULSE_RENDERER_PLUGIN_DESC_VERSION 1u










/**
 * ECS Components
 *
 */
typedef struct PulseCamera
{
    ecs_entity_t         window_entity;
    float                fov;
    float                near_plane;
    float                far_plane;

} PulseCamera;

typedef struct PulseLight
{
    HMM_Vec4             color;

} PulseLight;

typedef struct PulseRenderable
{
    PulseMeshHandle      mesh;
    PulseMaterialHandle  material;

} PulseRenderable;


// ECS component declarations
extern ECS_COMPONENT_DECLARE(PulseCamera);
extern ECS_COMPONENT_DECLARE(PulseLight);
extern ECS_COMPONENT_DECLARE(PulseRenderable);


/**
 * Functions
 *
 * @param[in] app
 *
 */
PULSE_API EPulseResult pulse_add_renderer_plugin(PulseAppId app);

#ifdef __cplusplus
}
#endif

#endif // PULSE_RENDERER_API_HEADER_GUARD
