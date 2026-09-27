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
#include "pulse_text.h"

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
 * 单个实体内联文本缓冲上限，含结尾 '\0'
 *
 */
#define PULSE_TEXT_MAX_CHARS 256u












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
    float                orthographic_size;
    bool                 orthographic;
    uint32_t             clear_color;

} PulseCamera;
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseCamera);

typedef struct PulseLight
{
    HMM_Vec4             color;

} PulseLight;
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseLight);

typedef struct PulseRenderable
{
    PulseMeshHandle      mesh;
    PulseMaterialHandle  material;

} PulseRenderable;
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseRenderable);

/**
 * 文字渲染组件；block 为 pulse_text 排版块 id，text 为内联 utf8；revision 由 TextSet 自增，驱动懒重排
 *
 */
typedef struct PulseText
{
    PulseTextBlockDesc   block;
    char                 text[PULSE_TEXT_MAX_CHARS];
    float                box_width;
    float                box_height;
    uint32_t             revision;

} PulseText;
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseText);



/**
 * Functions
 *
 * @param[in] app
 *
 */
PULSE_RENDERER_API EPulseAppAddPluginResult pulse_add_renderer_plugin(PulseAppId app);

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
