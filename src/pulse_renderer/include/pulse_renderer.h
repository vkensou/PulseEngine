#pragma once

#ifndef PULSE_RENDERER_API_HEADER_GUARD
#define PULSE_RENDERER_API_HEADER_GUARD
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
#include "pulse_graphics.h"

#if defined(PULSE_RENDERER_MODULE_BUILD)
#  define PULSE_RENDERER_API PULSE_EXPORT
#else
#  define PULSE_RENDERER_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
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
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseCamera);
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseLight);
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseRenderable);


/**
 * Functions
 *
 * @param[in] app
 *
 */
PULSE_RENDERER_API EPulseAppAddPluginResult pulse_add_renderer_plugin(PulseAppId app);
PULSE_RENDERER_API void pulse_set_shader_property_name_mapper(PulseAppId app, EPulseRendererPropertyType type, const char* name);

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
#endif // PULSE_RENDERER_API_HEADER_GUARD
