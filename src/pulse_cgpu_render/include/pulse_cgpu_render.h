#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "cgpu/api.h"
#include "flecs.h"
#include "pulse_app.h"
#include "rendergraph.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*pulse_cgpu_render_record_callback)(
    pulse_app_t app,
    pulse_rendergraph_t* graph,
    void* user_data
);

typedef struct pulse_cgpu_renderer_record_callback_desc {
    pulse_cgpu_render_record_callback callback;
    void* user_data;
    int32_t priority;
} pulse_cgpu_renderer_record_callback_desc;

pulse_texture_handle_t pulse_cgpu_render_import_window_backbuffer(
    pulse_app_t app,
    pulse_rendergraph_t* graph,
    ecs_entity_t window_entity
);

#define PULSE_CGPU_RENDER_PLUGIN_DESC_VERSION 1u

typedef struct pulse_cgpu_render_plugin_desc {
    uint32_t struct_size;
    uint32_t version;
    ECGPUBackend backend;
    ECGPUTextureFormat swapchain_format;
    uint32_t image_count;
    bool enable_debug_layer;
    bool enable_gpu_based_validation;
    bool enable_vsync;
    pulse_cgpu_render_record_callback record_callback;
    void* record_user_data;
} pulse_cgpu_render_plugin_desc;

typedef struct pulse_cgpu_renderer {
    CGPUInstanceId instance;
    CGPUAdapterId adapter;
    CGPUDeviceId device;
    CGPUQueueId graphics_queue;
    CGPUQueueId present_queue;
    CGPURenderPassId render_pass;
    ECGPUBackend backend;
    ECGPUTextureFormat swapchain_format;
    uint32_t image_count;
    uint64_t frame_index;
} pulse_cgpu_renderer;

typedef struct pulse_cgpu_surface {
    CGPUInstanceId instance;
    CGPUSurfaceId surface;
} pulse_cgpu_surface;

typedef struct pulse_cgpu_swapchain {
    CGPUDeviceId device;
    CGPUSwapChainId swapchain;
    CGPUTextureViewId* backbuffer_views;
    CGPUFramebufferId* framebuffers;
    CGPUSemaphoreId* image_available_semaphores;
    CGPUSemaphoreId* render_finished_semaphores;
    uint32_t backbuffer_count;
    uint32_t width;
    uint32_t height;
    uint32_t current_backbuffer_index;
    pulse_backbuffer_data_t* backbuffers;
    pulse_backbuffer_data_t* current_backbuffer;
} pulse_cgpu_swapchain;

extern ECS_COMPONENT_DECLARE(pulse_cgpu_renderer);
extern ECS_COMPONENT_DECLARE(pulse_cgpu_surface);
extern ECS_COMPONENT_DECLARE(pulse_cgpu_swapchain);

pulse_cgpu_render_plugin_desc pulse_cgpu_render_plugin_desc_default(void);

pulse_result_t pulse_cgpu_render_add_plugin(
    pulse_app_t app,
    const pulse_cgpu_render_plugin_desc* desc
);

pulse_result_t pulse_cgpu_render_add_record_callback(
    pulse_app_t app,
    const pulse_cgpu_renderer_record_callback_desc* desc);

pulse_result_t pulse_cgpu_render_remove_record_callback(
    pulse_app_t app,
    pulse_cgpu_render_record_callback callback);

const pulse_cgpu_renderer* pulse_cgpu_renderer_get(pulse_app_t app);
const pulse_cgpu_surface* pulse_cgpu_surface_get(pulse_app_t app, ecs_entity_t entity);
const pulse_cgpu_swapchain* pulse_cgpu_swapchain_get(pulse_app_t app, ecs_entity_t entity);

#ifdef __cplusplus
}
#endif
