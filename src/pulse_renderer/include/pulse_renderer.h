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
 * 跨包渲染 feature 契约：feature 包在插件 build 期调 pulse_add_render_feature 注册；
 * 各钩子按帧流水线阶段被回调，逐 item 的数据经 *FeatureItem 与 feature 自持的 data blob 传递
 *
 */
#define PULSE_RENDER_FEATURE_DESC_VERSION 1u








struct PulseFeatureExtractContext;
struct PulseFeatureCullContext;
struct PulseFeaturePrepareContext;
struct PulseFeatureDrawContext;
struct PulseFeatureRecordContext;
struct PulseFeatureItem;

typedef void (*PulseProcFeatureExtractFn)(PulseAppId app, PulseFeatureExtractContext* ctx, [[pulse::optional]] void* userdata);
typedef int32_t (*PulseProcFeatureCullFn)(PulseAppId app, PulseFeatureCullContext* ctx, const PulseFeatureItem* item, [[pulse::optional]] const void* item_data, [[pulse::optional]] void* userdata);
typedef void (*PulseProcFeaturePrepareFn)(PulseAppId app, PulseFeaturePrepareContext* ctx, [[pulse::optional]] void* userdata);
typedef void (*PulseProcFeatureDrawFn)(PulseAppId app, PulseRenderPassEncoder* encoder, PulseFeatureDrawContext* ctx, [[pulse::optional]] void* userdata);
typedef void (*PulseProcFeatureRecordFn)(PulseAppId app, PulseRenderGraphId graph, PulseRenderPassBuilder* pass, PulseFeatureRecordContext* ctx, [[pulse::optional]] void* userdata);
typedef void (*PulseProcFeatureDestroyFn)([[pulse::optional]] void* userdata);

struct PulseFeatureExtractContext;
typedef struct PulseFeatureExtractContext PulseFeatureExtractContext;

struct PulseFeatureCullContext;
typedef struct PulseFeatureCullContext PulseFeatureCullContext;

struct PulseFeaturePrepareContext;
typedef struct PulseFeaturePrepareContext PulseFeaturePrepareContext;

struct PulseFeatureDrawContext;
typedef struct PulseFeatureDrawContext PulseFeatureDrawContext;

struct PulseFeatureRecordContext;
typedef struct PulseFeatureRecordContext PulseFeatureRecordContext;

typedef struct PulseFeatureItem
{
    ecs_entity_t         entity;
    PulseMeshHandle      mesh;
    PulseMaterialHandle  material;
    PulseShaderHandle    shader;
    HMM_Mat4             world_matrix;
    uint32_t             instance_count;
    int32_t              sorting_order;

} PulseFeatureItem;

typedef struct PulseFeatureCullCamera
{
    ecs_entity_t         camera_entity;
    uint32_t             view_index;
    HMM_Mat4             view_matrix;
    HMM_Mat4             proj_matrix;
    bool                 orthographic;
    float                fov;
    float                orthographic_size;
    float                near_plane;
    float                far_plane;
    int32_t              width;
    int32_t              height;

} PulseFeatureCullCamera;

/**
 * extract/draw 必填；cull/prepare/record/destroy 可为空，cull 为空时全部 item 进 0 号 list
 *
 */
typedef struct PulseRenderFeatureDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    const char*          name;
    uint32_t             data_size;
    PulseProcFeatureExtractFn extract;
    PulseProcFeatureCullFn cull;
    PulseProcFeaturePrepareFn prepare;
    PulseProcFeatureDrawFn draw;
    PulseProcFeatureRecordFn record;
    PulseProcFeatureDestroyFn destroy;
    [[pulse::optional]]
    void*                userdata;

} PulseRenderFeatureDesc;


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
    int32_t              sorting_order;

} PulseRenderable;
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseRenderable);


PULSE_RENDERER_API EPulseResult pulse_add_render_feature(PulseAppId app, const PulseRenderFeatureDesc* desc);
PULSE_RENDERER_API EPulseResult pulse_remove_render_feature(PulseAppId app, const char* name);
PULSE_RENDERER_API bool pulse_feature_cull_get_camera(PulseFeatureCullContext* ctx, [[pulse::out]] PulseFeatureCullCamera* camera);
PULSE_RENDERER_API void pulse_feature_extract_submit(PulseFeatureExtractContext* ctx, const PulseFeatureItem* item, [[pulse::optional]] const void* data);
PULSE_RENDERER_API uint32_t pulse_feature_prepare_item_count(PulseFeaturePrepareContext* ctx);
PULSE_RENDERER_API bool pulse_feature_prepare_get_item(PulseFeaturePrepareContext* ctx, uint32_t index, [[pulse::out]] PulseFeatureItem* item);
[[pulse::optional]] PULSE_RENDERER_API const void* pulse_feature_prepare_item_data(PulseFeaturePrepareContext* ctx, uint32_t index);
[[pulse::optional]] PULSE_RENDERER_API void* pulse_feature_prepare_alloc_ubo(PulseFeaturePrepareContext* ctx, uint32_t index, uint32_t set, uint32_t binding, uint32_t size);
[[pulse::optional]] PULSE_RENDERER_API const PulseFeatureItem* pulse_feature_draw_get_item(PulseFeatureDrawContext* ctx);
[[pulse::optional]] PULSE_RENDERER_API const void* pulse_feature_draw_item_data(PulseFeatureDrawContext* ctx);
PULSE_RENDERER_API void pulse_feature_draw_bind_ubo_columns(PulseRenderPassEncoder* encoder, PulseFeatureDrawContext* ctx);
PULSE_RENDERER_API uint32_t pulse_feature_record_item_count(PulseFeatureRecordContext* ctx);
[[pulse::optional]] PULSE_RENDERER_API const void* pulse_feature_record_item_data(PulseFeatureRecordContext* ctx, uint32_t index);

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
