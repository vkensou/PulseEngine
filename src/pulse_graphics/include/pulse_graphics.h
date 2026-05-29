#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "cgpu/api.h"
#include "flecs.h"
#include "pulse_asset.h"
#include "pulse_renderer_asset.h"
#include "rendergraph.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */

#define PULSE_GRAPHICS_PLUGIN_DESC_VERSION 1u

#define PULSE_TYPE_TEXTURE         UINT64_C(0x1000)
#define PULSE_TYPE_BUFFER          UINT64_C(0x1001)
#define PULSE_TYPE_SAMPLER         UINT64_C(0x1002)
#define PULSE_TYPE_SHADER_LIBRARY  UINT64_C(0x1003)
#define PULSE_TYPE_SHADER          UINT64_C(0x1004)
#define PULSE_TYPE_COMPUTE_SHADER  UINT64_C(0x1005)
#define PULSE_TYPE_MESH            UINT64_C(0x1006)
#define PULSE_TYPE_MATERIAL        UINT64_C(0x1007)

/* Asset handles and refs */

typedef struct pulse_shader_t         { uint32_t index; uint32_t generation; } pulse_shader_t;
typedef struct pulse_shader_library_t { uint32_t index; uint32_t generation; } pulse_shader_library_t;
typedef struct pulse_compute_shader_t { uint32_t index; uint32_t generation; } pulse_compute_shader_t;
typedef struct pulse_mesh_t           { uint32_t index; uint32_t generation; } pulse_mesh_t;
typedef struct pulse_texture_t        { uint32_t index; uint32_t generation; } pulse_texture_t;
typedef struct pulse_buffer_t         { uint32_t index; uint32_t generation; } pulse_buffer_t;
typedef struct pulse_material_t       { uint32_t index; uint32_t generation; } pulse_material_t;
typedef struct pulse_sampler_t        { uint32_t index; uint32_t generation; } pulse_sampler_t;

typedef struct pulse_graphics_shader_ref {
    pulse_shader_t handle;
    pulse_shader_data_t* ptr;
} pulse_graphics_shader_ref;

typedef struct pulse_graphics_compute_shader_ref {
    pulse_compute_shader_t handle;
    pulse_compute_shader_data_t* ptr;
} pulse_graphics_compute_shader_ref;

typedef struct pulse_graphics_shader_library_ref {
    pulse_shader_library_t handle;
    pulse_shader_library_data_t* ptr;
} pulse_graphics_shader_library_ref;

typedef struct pulse_graphics_buffer_ref {
    pulse_buffer_t handle;
    pulse_buffer_data_t* ptr;
} pulse_graphics_buffer_ref;

typedef struct pulse_graphics_texture_ref {
    pulse_texture_t handle;
    pulse_texture_data_t* ptr;
} pulse_graphics_texture_ref;

typedef struct pulse_graphics_mesh_ref {
    pulse_mesh_t handle;
    pulse_mesh_data_t* ptr;
} pulse_graphics_mesh_ref;

typedef struct pulse_graphics_material_ref {
    pulse_material_t handle;
    pulse_material_data_t* ptr;
} pulse_graphics_material_ref;

typedef struct pulse_graphics_sampler_ref {
    pulse_sampler_t handle;
    pulse_sampler_data_t* ptr;
} pulse_graphics_sampler_ref;

/* Plugin and runtime */

typedef void (*pulse_graphics_render_record_callback)(
    pulse_app_t app,
    pulse_rendergraph_t* graph,
    void* user_data);

typedef struct pulse_graphics_plugin_desc {
    uint32_t struct_size;
    uint32_t version;
    ECGPUBackend backend;
    ECGPUTextureFormat swapchain_format;
    uint32_t image_count;
    bool enable_debug_layer;
    bool enable_gpu_based_validation;
    bool enable_vsync;
    pulse_graphics_render_record_callback record_callback;
    void* record_user_data;
} pulse_graphics_plugin_desc;

pulse_graphics_plugin_desc pulse_graphics_plugin_desc_default(void);
pulse_result_t pulse_graphics_add_plugin(pulse_app_t app, const pulse_graphics_plugin_desc* desc);

typedef struct pulse_graphics_renderer {
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
} pulse_graphics_renderer;

typedef struct pulse_graphics_surface {
    CGPUInstanceId instance;
    CGPUSurfaceId surface;
} pulse_graphics_surface;

typedef struct pulse_graphics_swapchain {
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
} pulse_graphics_swapchain;

extern ECS_COMPONENT_DECLARE(pulse_graphics_renderer);
extern ECS_COMPONENT_DECLARE(pulse_graphics_surface);
extern ECS_COMPONENT_DECLARE(pulse_graphics_swapchain);

typedef struct pulse_graphics_renderer_record_callback_desc {
    pulse_graphics_render_record_callback callback;
    void* user_data;
    int32_t priority;
} pulse_graphics_renderer_record_callback_desc;

pulse_result_t pulse_graphics_render_add_record_callback(
    pulse_app_t app,
    const pulse_graphics_renderer_record_callback_desc* desc);

pulse_result_t pulse_graphics_render_remove_record_callback(
    pulse_app_t app,
    pulse_graphics_render_record_callback callback);

pulse_texture_handle_t pulse_graphics_render_import_window_backbuffer(
    pulse_app_t app,
    pulse_rendergraph_t* graph,
    ecs_entity_t window_entity);

const pulse_graphics_renderer* pulse_graphics_renderer_get(pulse_app_t app);
const pulse_graphics_surface* pulse_graphics_surface_get(pulse_app_t app, ecs_entity_t entity);
const pulse_graphics_swapchain* pulse_graphics_swapchain_get(pulse_app_t app, ecs_entity_t entity);

/* Shader library */

typedef struct pulse_graphics_shader_library_create_desc {
    const void* code;
    uint32_t code_size;
} pulse_graphics_shader_library_create_desc;

typedef struct pulse_graphics_shader_library_load_desc {
    const char* path;
} pulse_graphics_shader_library_load_desc;

pulse_shader_library_t pulse_graphics_shader_library_create(
    pulse_app_t app,
    const pulse_graphics_shader_library_create_desc* desc);

pulse_shader_library_t pulse_graphics_shader_library_load(
    pulse_app_t app,
    const pulse_graphics_shader_library_load_desc* desc);

static pulse_asset_handle pulse_graphics_shader_library_to_handle(pulse_shader_library_t lib) { return { PULSE_TYPE_SHADER_LIBRARY, lib.index, lib.generation }; }
static bool pulse_graphics_shader_library_is_alive(pulse_app_t app, pulse_shader_library_t lib) { return pulse_asset_is_alive(app, pulse_graphics_shader_library_to_handle(lib)); }
static bool pulse_graphics_shader_library_is_ready(pulse_app_t app, pulse_shader_library_t lib) { return pulse_asset_is_ready(app, pulse_graphics_shader_library_to_handle(lib)); }
bool pulse_graphics_shader_library_acquire(pulse_app_t app, pulse_shader_library_t handle, pulse_graphics_shader_library_ref* ref);
void pulse_graphics_shader_library_release(pulse_app_t app, pulse_graphics_shader_library_ref* ref);
static void pulse_graphics_shader_library_unload(pulse_app_t app, pulse_shader_library_t lib) { pulse_asset_unload(app, pulse_graphics_shader_library_to_handle(lib)); }

/* Shader */

typedef struct pulse_graphics_shader_create_from_binary_desc {
    const void* vs_data;
    uint32_t vs_size;
    const void* fs_data;
    uint32_t fs_size;
    CGPUBlendStateDescriptor blend_desc;
    CGPUDepthStateDescriptor depth_desc;
    CGPURasterizerStateDescriptor rasterizer_state;
} pulse_graphics_shader_create_from_binary_desc;

typedef struct pulse_graphics_shader_create_from_file_desc {
    const char* vert_path;
    const char* frag_path;
    CGPUBlendStateDescriptor blend_desc;
    CGPUDepthStateDescriptor depth_desc;
    CGPURasterizerStateDescriptor rasterizer_state;
} pulse_graphics_shader_create_from_file_desc;

pulse_shader_t pulse_graphics_shader_create_from_binary(
    pulse_app_t app,
    const pulse_graphics_shader_create_from_binary_desc* desc);

pulse_shader_t pulse_graphics_shader_create_from_file(
    pulse_app_t app,
    const pulse_graphics_shader_create_from_file_desc* desc);

static pulse_asset_handle pulse_graphics_shader_to_handle(pulse_shader_t shader) { return { PULSE_TYPE_SHADER, shader.index, shader.generation }; }
static bool pulse_graphics_shader_is_alive(pulse_app_t app, pulse_shader_t shader) { return pulse_asset_is_alive(app, pulse_graphics_shader_to_handle(shader)); }
static bool pulse_graphics_shader_is_ready(pulse_app_t app, pulse_shader_t shader) { return pulse_asset_is_ready(app, pulse_graphics_shader_to_handle(shader)); }
bool pulse_graphics_shader_acquire(pulse_app_t app, pulse_shader_t handle, pulse_graphics_shader_ref* ref);
void pulse_graphics_shader_release(pulse_app_t app, pulse_graphics_shader_ref* ref);
static void pulse_graphics_shader_unload(pulse_app_t app, pulse_shader_t shader) { pulse_asset_unload(app, pulse_graphics_shader_to_handle(shader)); }

/* Compute shader */

typedef struct pulse_graphics_compute_shader_create_from_binary_desc {
    const void* cs_data;
    uint32_t cs_size;
} pulse_graphics_compute_shader_create_from_binary_desc;

typedef struct pulse_graphics_compute_shader_create_from_file_desc {
    const char* cs_path;
} pulse_graphics_compute_shader_create_from_file_desc;

pulse_compute_shader_t pulse_graphics_compute_shader_create_from_binary(
    pulse_app_t app,
    const pulse_graphics_compute_shader_create_from_binary_desc* desc);

pulse_compute_shader_t pulse_graphics_compute_shader_create_from_file(
    pulse_app_t app,
    const pulse_graphics_compute_shader_create_from_file_desc* desc);

static pulse_asset_handle pulse_graphics_compute_shader_to_handle(pulse_compute_shader_t cs) { return { PULSE_TYPE_COMPUTE_SHADER, cs.index, cs.generation }; }
static bool pulse_graphics_compute_shader_is_alive(pulse_app_t app, pulse_compute_shader_t cs) { return pulse_asset_is_alive(app, pulse_graphics_compute_shader_to_handle(cs)); }
static bool pulse_graphics_compute_shader_is_ready(pulse_app_t app, pulse_compute_shader_t cs) { return pulse_asset_is_ready(app, pulse_graphics_compute_shader_to_handle(cs)); }
bool pulse_graphics_compute_shader_acquire(pulse_app_t app, pulse_compute_shader_t handle, pulse_graphics_compute_shader_ref* ref);
void pulse_graphics_compute_shader_release(pulse_app_t app, pulse_graphics_compute_shader_ref* ref);
static void pulse_graphics_compute_shader_unload(pulse_app_t app, pulse_compute_shader_t cs) { pulse_asset_unload(app, pulse_graphics_compute_shader_to_handle(cs)); }

/* Buffer */

typedef struct pulse_graphics_buffer_create_desc {
    CGPUBufferDescriptor desc;
    uint64_t data_size;
    const void* data;
} pulse_graphics_buffer_create_desc;

pulse_buffer_t pulse_graphics_buffer_create(
    pulse_app_t app,
    const pulse_graphics_buffer_create_desc* desc);

static pulse_asset_handle pulse_graphics_buffer_to_handle(pulse_buffer_t buffer) { return { PULSE_TYPE_BUFFER, buffer.index, buffer.generation }; }
static bool pulse_graphics_buffer_is_alive(pulse_app_t app, pulse_buffer_t buffer) { return pulse_asset_is_alive(app, pulse_graphics_buffer_to_handle(buffer)); }
static bool pulse_graphics_buffer_is_ready(pulse_app_t app, pulse_buffer_t buffer) { return pulse_asset_is_ready(app, pulse_graphics_buffer_to_handle(buffer)); }
bool pulse_graphics_buffer_acquire(pulse_app_t app, pulse_buffer_t handle, pulse_graphics_buffer_ref* ref);
void pulse_graphics_buffer_release(pulse_app_t app, pulse_graphics_buffer_ref* ref);
static void pulse_graphics_buffer_unload(pulse_app_t app, pulse_buffer_t buffer) { pulse_asset_unload(app, pulse_graphics_buffer_to_handle(buffer)); }

/* Sampler */

typedef struct pulse_graphics_sampler_create_desc {
    CGPUSamplerDescriptor desc;
} pulse_graphics_sampler_create_desc;

pulse_sampler_t pulse_graphics_sampler_create(
    pulse_app_t app,
    const pulse_graphics_sampler_create_desc* desc);

static pulse_asset_handle pulse_graphics_sampler_to_handle(pulse_sampler_t sampler) { return { PULSE_TYPE_SAMPLER, sampler.index, sampler.generation }; }
static bool pulse_graphics_sampler_is_alive(pulse_app_t app, pulse_sampler_t sampler) { return pulse_asset_is_alive(app, pulse_graphics_sampler_to_handle(sampler)); }
static bool pulse_graphics_sampler_is_ready(pulse_app_t app, pulse_sampler_t sampler) { return pulse_asset_is_ready(app, pulse_graphics_sampler_to_handle(sampler)); }
bool pulse_graphics_sampler_acquire(pulse_app_t app, pulse_sampler_t handle, pulse_graphics_sampler_ref* ref);
void pulse_graphics_sampler_release(pulse_app_t app, pulse_graphics_sampler_ref* ref);
static void pulse_graphics_sampler_unload(pulse_app_t app, pulse_sampler_t sampler) { pulse_asset_unload(app, pulse_graphics_sampler_to_handle(sampler)); }

/* Texture */

typedef struct pulse_graphics_texture_create_desc {
    CGPUTextureDescriptor desc;
    uint64_t pixel_data_size;
    const void* pixel_data;
    bool generate_mipmaps;
} pulse_graphics_texture_create_desc;

typedef struct pulse_graphics_texture_load_desc {
    const char* filepath;
    bool generate_mipmaps;
} pulse_graphics_texture_load_desc;

pulse_texture_t pulse_graphics_texture_create(
    pulse_app_t app,
    const pulse_graphics_texture_create_desc* desc);

pulse_texture_t pulse_graphics_texture_load(
    pulse_app_t app,
    const pulse_graphics_texture_load_desc* desc);

static pulse_asset_handle pulse_graphics_texture_to_handle(pulse_texture_t texture) { return { PULSE_TYPE_TEXTURE, texture.index, texture.generation }; }
static bool pulse_graphics_texture_is_alive(pulse_app_t app, pulse_texture_t texture) { return pulse_asset_is_alive(app, pulse_graphics_texture_to_handle(texture)); }
static bool pulse_graphics_texture_is_ready(pulse_app_t app, pulse_texture_t texture) { return pulse_asset_is_ready(app, pulse_graphics_texture_to_handle(texture)); }
bool pulse_graphics_texture_acquire(pulse_app_t app, pulse_texture_t handle, pulse_graphics_texture_ref* ref);
void pulse_graphics_texture_release(pulse_app_t app, pulse_graphics_texture_ref* ref);
static void pulse_graphics_texture_unload(pulse_app_t app, pulse_texture_t texture) { pulse_asset_unload(app, pulse_graphics_texture_to_handle(texture)); }

/* Mesh */

typedef struct pulse_graphics_mesh_create_from_data_desc {
    uint32_t vertex_count;
    uint32_t vertex_stride;
    CGPUVertexLayout layout;
    const uint8_t* vertex_data;
    bool update_vertex_data_from_compute_shader;
    ECGPUPrimitiveTopology topology;
    uint32_t index_count;
    uint32_t index_stride;
    const uint8_t* index_data;
    bool update_index_data_from_compute_shader;
} pulse_graphics_mesh_create_from_data_desc;

typedef struct pulse_graphics_mesh_create_dynamic_desc {
    ECGPUPrimitiveTopology topology;
    CGPUVertexLayout layout;
    uint32_t index_stride;
} pulse_graphics_mesh_create_dynamic_desc;

pulse_mesh_t pulse_graphics_mesh_create_from_data(
    pulse_app_t app,
    const pulse_graphics_mesh_create_from_data_desc* desc);

pulse_mesh_t pulse_graphics_mesh_create_dynamic(
    pulse_app_t app,
    const pulse_graphics_mesh_create_dynamic_desc* desc);

pulse_mesh_t pulse_graphics_mesh_load(
    pulse_app_t app,
    const char* filepath);

void pulse_graphics_mesh_update_vertices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count);
void pulse_graphics_mesh_update_indices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count);

static pulse_asset_handle pulse_graphics_mesh_to_handle(pulse_mesh_t mesh) { return { PULSE_TYPE_MESH, mesh.index, mesh.generation }; }
static bool pulse_graphics_mesh_is_alive(pulse_app_t app, pulse_mesh_t mesh) { return pulse_asset_is_alive(app, pulse_graphics_mesh_to_handle(mesh)); }
static bool pulse_graphics_mesh_is_ready(pulse_app_t app, pulse_mesh_t mesh) { return pulse_asset_is_ready(app, pulse_graphics_mesh_to_handle(mesh)); }
bool pulse_graphics_mesh_acquire(pulse_app_t app, pulse_mesh_t handle, pulse_graphics_mesh_ref* ref);
void pulse_graphics_mesh_release(pulse_app_t app, pulse_graphics_mesh_ref* ref);
static void pulse_graphics_mesh_unload(pulse_app_t app, pulse_mesh_t mesh) { pulse_asset_unload(app, pulse_graphics_mesh_to_handle(mesh)); }

/* Material */

typedef struct pulse_graphics_material_create_desc {
    pulse_shader_t shader;
} pulse_graphics_material_create_desc;

pulse_material_t pulse_graphics_material_create(
    pulse_app_t app,
    const pulse_graphics_material_create_desc* desc);

static pulse_asset_handle pulse_graphics_material_to_handle(pulse_material_t material) { return { PULSE_TYPE_MATERIAL, material.index, material.generation }; }
static bool pulse_graphics_material_is_alive(pulse_app_t app, pulse_material_t material) { return pulse_asset_is_alive(app, pulse_graphics_material_to_handle(material)); }
static bool pulse_graphics_material_is_ready(pulse_app_t app, pulse_material_t material) { return pulse_asset_is_ready(app, pulse_graphics_material_to_handle(material)); }
bool pulse_graphics_material_acquire(pulse_app_t app, pulse_material_t handle, pulse_graphics_material_ref* ref);
void pulse_graphics_material_release(pulse_app_t app, pulse_graphics_material_ref* ref);
static void pulse_graphics_material_unload(pulse_app_t app, pulse_material_t material) { pulse_asset_unload(app, pulse_graphics_material_to_handle(material)); }

void pulse_graphics_material_bind_buffer(
    pulse_graphics_material_ref* material,
    uint32_t set, uint32_t binding,
    pulse_graphics_buffer_ref* buffer);

void pulse_graphics_material_bind_texture(
    pulse_graphics_material_ref* material,
    uint32_t set, uint32_t binding,
    pulse_graphics_texture_ref* texture);

void pulse_graphics_material_bind_sampler(
    pulse_graphics_material_ref* material,
    uint32_t set, uint32_t binding,
    pulse_graphics_sampler_ref* sampler);

void pulse_graphics_material_bind_data(
    pulse_graphics_material_ref* material,
    uint32_t set, uint32_t binding,
    size_t size, const void* data);

/* Encoder */

void pulse_encoder_draw(pulse_renderpass_encoder_t* encoder, pulse_material_t material, pulse_mesh_t mesh);
void pulse_encoder_draw_submesh(pulse_renderpass_encoder_t* encoder, pulse_material_t material, pulse_mesh_t mesh, uint32_t idx_count, uint32_t first_idx, uint32_t vtx_count, uint32_t first_vtx);
void pulse_encoder_draw_procedure(pulse_renderpass_encoder_t* encoder, pulse_material_t material, ECGPUPrimitiveTopology topology, uint32_t vertex_count);
void pulse_encoder_dispatch(pulse_renderpass_encoder_t* encoder, pulse_compute_shader_t compute_shader, uint32_t x, uint32_t y, uint32_t z);

void pulse_encoder_set_global_texture(pulse_renderpass_encoder_t* encoder, pulse_texture_t texture, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_buffer(pulse_renderpass_encoder_t* encoder, pulse_buffer_t buffer, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_sampler(pulse_renderpass_encoder_t* encoder, pulse_sampler_t sampler, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_texture_handle(pulse_renderpass_encoder_t* encoder, pulse_texture_handle_t handle, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_buffer_handle(pulse_renderpass_encoder_t* encoder, pulse_buffer_handle_t handle, uint32_t set, uint32_t binding);
void pulse_encoder_set_global_buffer_offset(pulse_renderpass_encoder_t* encoder, pulse_buffer_handle_t handle, uint32_t set, uint32_t binding, uint64_t offset, uint64_t size);
void pulse_encoder_set_viewport(pulse_renderpass_encoder_t* encoder, float x, float y, float w, float h, float min_d, float max_d);
void pulse_encoder_set_scissor(pulse_renderpass_encoder_t* encoder, uint32_t x, uint32_t y, uint32_t w, uint32_t h);
void pulse_encoder_push_constants(pulse_renderpass_encoder_t* encoder, pulse_shader_t shader, const char* name, const void* data);

#ifdef __cplusplus
}
#endif
