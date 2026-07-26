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
 * Renderer property type
 *
 */
typedef enum EPulseRendererPropertyType
{
    PULSE_RENDERER_PROPERTY_TYPE_VP_MATRIX,   /** ( 0)                                */
    PULSE_RENDERER_PROPERTY_TYPE_MODEL_MATRIX, /** ( 1)                                */

    PULSE_RENDERER_PROPERTY_TYPE_COUNT

} EPulseRendererPropertyType;








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
PULSE_API void pulse_set_shader_property_name_mapper(PulseAppId app, EPulseRendererPropertyType type, const char* name);

#ifdef __cplusplus
}
#endif

#endif // PULSE_RENDERER_API_HEADER_GUARD
