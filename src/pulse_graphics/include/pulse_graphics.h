#pragma once

#ifndef PULSE_GRAPHICS_API_HEADER_GUARD
#define PULSE_GRAPHICS_API_HEADER_GUARD

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t

#include "cgpu/api.h"
#include "pulse_app.h"
#include "pulse_asset.h"
#include "rendergraph.h"

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
#define PULSE_GRAPHICS_PLUGIN_DESC_VERSION 1u

#define PULSE_TYPE_TEXTURE UINT64_C(0x1000)

#define PULSE_TYPE_GRAPHICS_BUFFER UINT64_C(0x1001)

#define PULSE_TYPE_SAMPLER UINT64_C(0x1002)

#define PULSE_TYPE_SHADER_LIBRARY UINT64_C(0x1003)

#define PULSE_TYPE_SHADER UINT64_C(0x1004)

#define PULSE_TYPE_COMPUTE_SHADER UINT64_C(0x1005)

#define PULSE_TYPE_MESH UINT64_C(0x1006)

#define PULSE_TYPE_MATERIAL UINT64_C(0x1007)








/**
 * Function pointer: render record callback
 *
 * @param[in] app
 * @param[in] graph
 * @param[in] userData
 *
 */
typedef void (*PulseProcRenderRecordCallback)(PulseAppId app, pulse_rendergraph_t* graph, void* user_data);

/**
 * Asset handle types (value structs with index+generation)
 *
 */
typedef struct PulseShaderHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseShaderHandle;

typedef struct PulseShaderLibraryHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseShaderLibraryHandle;

typedef struct PulseComputeShaderHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseComputeShaderHandle;

typedef struct PulseMeshHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseMeshHandle;

typedef struct PulseTextureHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseTextureHandle;

typedef struct PulseGraphicsBufferHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseGraphicsBufferHandle;

typedef struct PulseMaterialHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseMaterialHandle;

typedef struct PulseSamplerHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseSamplerHandle;

/**
 * Asset ref types (handle + data ptr)
 *
 */
typedef struct PulseShader
{
    PulseShaderHandle    handle;
    void*                ptr;

} PulseShader;

typedef struct PulseShaderLibrary
{
    PulseShaderLibraryHandle handle;
    void*                ptr;

} PulseShaderLibrary;

typedef struct PulseComputeShader
{
    PulseComputeShaderHandle handle;
    void*                ptr;

} PulseComputeShader;

typedef struct PulseGraphicsBuffer
{
    PulseGraphicsBufferHandle handle;
    void*                ptr;

} PulseGraphicsBuffer;

typedef struct PulseTexture
{
    PulseTextureHandle   handle;
    void*                ptr;

} PulseTexture;

typedef struct PulseMesh
{
    PulseMeshHandle      handle;
    void*                ptr;

} PulseMesh;

typedef struct PulseMaterial
{
    PulseMaterialHandle  handle;
    void*                ptr;

} PulseMaterial;

typedef struct PulseSampler
{
    PulseSamplerHandle   handle;
    void*                ptr;

} PulseSampler;

/**
 * Plugin descriptor
 *
 */
typedef struct PulseGraphicsPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    ECGPUBackend         backend;
    ECGPUTextureFormat   swapchain_format;
    uint32_t             image_count;
    bool                 enable_debug_layer;
    bool                 enable_gpu_based_validation;
    bool                 enable_vsync;
    PulseProcRenderRecordCallback record_callback;
    void*                record_user_data;

} PulseGraphicsPluginDesc;

/**
 * Renderer state (opaque at C API level)
 *
 */
typedef struct PulseRenderer
{
    CGPUInstanceId       instance;
    CGPUAdapterId        adapter;
    CGPUDeviceId         device;
    CGPUQueueId          graphics_queue;
    CGPUQueueId          present_queue;
    CGPURenderPassId     render_pass;
    ECGPUBackend         backend;
    ECGPUTextureFormat   swapchain_format;
    uint32_t             image_count;
    uint64_t             frame_index;

} PulseRenderer;

/**
 * Window surface
 *
 */
typedef struct PulseSurface
{
    CGPUInstanceId       instance;
    CGPUSurfaceId        surface;

} PulseSurface;

/**
 * Window swapchain
 *
 */
typedef struct PulseSwapchain
{
    CGPUDeviceId         device;
    CGPUSwapChainId      swapchain;
    CGPUTextureViewId*   backbuffer_views;
    CGPUFramebufferId*   framebuffers;
    CGPUSemaphoreId*     image_available_semaphores;
    CGPUSemaphoreId*     render_finished_semaphores;
    uint32_t             backbuffer_count;
    uint32_t             width;
    uint32_t             height;
    uint32_t             current_backbuffer_index;
    pulse_backbuffer_data_t* backbuffers;
    pulse_backbuffer_data_t* current_backbuffer;

} PulseSwapchain;

/**
 * Record callback descriptor
 *
 */
typedef struct PulseRenderRecordCallbackDesc
{
    PulseProcRenderRecordCallback callback;
    void*                user_data;
    int32_t              priority;

} PulseRenderRecordCallbackDesc;

/**
 * Shader library create desc
 *
 */
typedef struct PulseShaderLibraryCreateDesc
{
    const void*          code;
    uint32_t             code_size;

} PulseShaderLibraryCreateDesc;

/**
 * Shader library load desc
 *
 */
typedef struct PulseShaderLibraryLoadDesc
{
    const char*          path;

} PulseShaderLibraryLoadDesc;

/**
 * Shader create from binary desc
 *
 */
typedef struct PulseShaderCreateFromBinaryDesc
{
    const void*          vs_data;
    uint32_t             vs_size;
    const void*          fs_data;
    uint32_t             fs_size;
    CGPUBlendStateDescriptor blend_desc;
    CGPUDepthStateDescriptor depth_desc;
    CGPURasterizerStateDescriptor rasterizer_state;

} PulseShaderCreateFromBinaryDesc;

/**
 * Shader create from file desc
 *
 */
typedef struct PulseShaderCreateFromFileDesc
{
    const char*          vert_path;
    const char*          frag_path;
    CGPUBlendStateDescriptor blend_desc;
    CGPUDepthStateDescriptor depth_desc;
    CGPURasterizerStateDescriptor rasterizer_state;

} PulseShaderCreateFromFileDesc;

/**
 * Compute shader create from binary desc
 *
 */
typedef struct PulseComputeShaderCreateFromBinaryDesc
{
    const void*          cs_data;
    uint32_t             cs_size;

} PulseComputeShaderCreateFromBinaryDesc;

/**
 * Compute shader create from file desc
 *
 */
typedef struct PulseComputeShaderCreateFromFileDesc
{
    const char*          cs_path;

} PulseComputeShaderCreateFromFileDesc;

/**
 * Buffer create desc
 *
 */
typedef struct PulseGraphicsBufferCreateDesc
{
    CGPUBufferDescriptor desc;
    uint64_t             data_size;
    const void*          data;

} PulseGraphicsBufferCreateDesc;

/**
 * Sampler create desc
 *
 */
typedef struct PulseSamplerCreateDesc
{
    CGPUSamplerDescriptor desc;

} PulseSamplerCreateDesc;

/**
 * Texture create desc
 *
 */
typedef struct PulseTextureCreateDesc
{
    CGPUTextureDescriptor desc;
    uint64_t             pixel_data_size;
    const void*          pixel_data;
    bool                 generate_mipmaps;

} PulseTextureCreateDesc;

/**
 * Texture load desc
 *
 */
typedef struct PulseTextureLoadDesc
{
    const char*          filepath;
    bool                 generate_mipmaps;

} PulseTextureLoadDesc;

/**
 * Mesh create from data desc
 *
 */
typedef struct PulseMeshCreateFromDataDesc
{
    uint32_t             vertex_count;
    uint32_t             vertex_stride;
    CGPUVertexLayout     layout;
    const uint8_t*       vertex_data;
    bool                 update_vertex_data_from_compute_shader;
    ECGPUPrimitiveTopology topology;
    uint32_t             index_count;
    uint32_t             index_stride;
    const uint8_t*       index_data;
    bool                 update_index_data_from_compute_shader;

} PulseMeshCreateFromDataDesc;

/**
 * Mesh create dynamic desc
 *
 */
typedef struct PulseMeshCreateDynamicDesc
{
    ECGPUPrimitiveTopology topology;
    CGPUVertexLayout     layout;
    uint32_t             index_stride;

} PulseMeshCreateDynamicDesc;

/**
 * Material create desc
 *
 */
typedef struct PulseMaterialCreateDesc
{
    PulseShaderHandle    shader;

} PulseMaterialCreateDesc;


// ECS component declarations
extern ECS_COMPONENT_DECLARE(PulseRenderer);
extern ECS_COMPONENT_DECLARE(PulseSurface);
extern ECS_COMPONENT_DECLARE(PulseSwapchain);

// ---- inline helpers for asset handle types ----

static inline PulseAssetSystemId pulse_get_graphics_asset_system(PulseAppId app) {
    return pulse_get_asset_system(app);
}

// Shader
static inline PulseAssetHandle pulse_shader_to_handle(PulseShaderHandle shader) {
    PulseAssetHandle h = { PULSE_TYPE_SHADER, shader.index, shader.generation };
    return h;
}
static inline bool pulse_shader_is_alive(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_shader_to_handle(shader));
}
static inline bool pulse_shader_is_ready(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_shader_to_handle(shader));
}
static inline void pulse_unload_shader(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_shader_to_handle(shader));
}

// ShaderLibrary
static inline PulseAssetHandle pulse_shader_library_to_handle(PulseShaderLibraryHandle lib) {
    PulseAssetHandle h = { PULSE_TYPE_SHADER_LIBRARY, lib.index, lib.generation };
    return h;
}
static inline bool pulse_shader_library_is_alive(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_shader_library_to_handle(lib));
}
static inline bool pulse_shader_library_is_ready(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_shader_library_to_handle(lib));
}
static inline void pulse_unload_shader_library(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_shader_library_to_handle(lib));
}

// ComputeShader
static inline PulseAssetHandle pulse_compute_shader_to_handle(PulseComputeShaderHandle cs) {
    PulseAssetHandle h = { PULSE_TYPE_COMPUTE_SHADER, cs.index, cs.generation };
    return h;
}
static inline bool pulse_compute_shader_is_alive(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_compute_shader_to_handle(cs));
}
static inline bool pulse_compute_shader_is_ready(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_compute_shader_to_handle(cs));
}
static inline void pulse_unload_compute_shader(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_compute_shader_to_handle(cs));
}

// Buffer
static inline PulseAssetHandle pulse_graphics_buffer_to_handle(PulseGraphicsBufferHandle buffer) {
    PulseAssetHandle h = { PULSE_TYPE_GRAPHICS_BUFFER, buffer.index, buffer.generation };
    return h;
}
static inline bool pulse_graphics_buffer_is_alive(PulseAppId app, PulseGraphicsBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_buffer_to_handle(buffer));
}
static inline bool pulse_graphics_buffer_is_ready(PulseAppId app, PulseGraphicsBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_buffer_to_handle(buffer));
}
static inline void pulse_unload_graphics_buffer(PulseAppId app, PulseGraphicsBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_buffer_to_handle(buffer));
}

// Sampler
static inline PulseAssetHandle pulse_sampler_to_handle(PulseSamplerHandle sampler) {
    PulseAssetHandle h = { PULSE_TYPE_SAMPLER, sampler.index, sampler.generation };
    return h;
}
static inline bool pulse_sampler_is_alive(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_sampler_to_handle(sampler));
}
static inline bool pulse_sampler_is_ready(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_sampler_to_handle(sampler));
}
static inline void pulse_unload_sampler(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_sampler_to_handle(sampler));
}

// Texture
static inline PulseAssetHandle pulse_texture_to_handle(PulseTextureHandle texture) {
    PulseAssetHandle h = { PULSE_TYPE_TEXTURE, texture.index, texture.generation };
    return h;
}
static inline bool pulse_texture_is_alive(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_texture_to_handle(texture));
}
static inline bool pulse_texture_is_ready(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_texture_to_handle(texture));
}
static inline void pulse_unload_texture(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_texture_to_handle(texture));
}

// Mesh
static inline PulseAssetHandle pulse_mesh_to_handle(PulseMeshHandle mesh) {
    PulseAssetHandle h = { PULSE_TYPE_MESH, mesh.index, mesh.generation };
    return h;
}
static inline bool pulse_mesh_is_alive(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_mesh_to_handle(mesh));
}
static inline bool pulse_mesh_is_ready(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_mesh_to_handle(mesh));
}
static inline void pulse_unload_mesh(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_mesh_to_handle(mesh));
}

// Material
static inline PulseAssetHandle pulse_material_to_handle(PulseMaterialHandle material) {
    PulseAssetHandle h = { PULSE_TYPE_MATERIAL, material.index, material.generation };
    return h;
}
static inline bool pulse_material_is_alive(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_material_to_handle(material));
}
static inline bool pulse_material_is_ready(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_material_to_handle(material));
}
static inline void pulse_unload_material(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_material_to_handle(material));
}


/**
 * ======== Functions ========
 * Plugin
 *
 */
PULSE_API PulseGraphicsPluginDesc pulse_graphics_plugin_desc_default(void);
PULSE_API EPulseResult pulse_add_graphics_plugin(PulseAppId app, const PulseGraphicsPluginDesc* desc);

/**
 * Renderer
 *
 * @param[in] app
 *
 */
PULSE_API const PulseRenderer* pulse_get_renderer(PulseAppId app);
PULSE_API const PulseSurface* pulse_get_surface(PulseAppId app, ecs_entity_t entity);
PULSE_API const PulseSwapchain* pulse_get_swapchain(PulseAppId app, ecs_entity_t entity);

/**
 * Render record callbacks
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API EPulseResult pulse_add_render_record_callback(PulseAppId app, const PulseRenderRecordCallbackDesc* desc);
PULSE_API EPulseResult pulse_remove_render_record_callback(PulseAppId app, PulseProcRenderRecordCallback callback);
PULSE_API pulse_texture_handle_t pulse_import_window_backbuffer(PulseAppId app, pulse_rendergraph_t* graph, ecs_entity_t window_entity);

/**
 * Shader library
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API PulseShaderLibraryHandle pulse_create_shader_library(PulseAppId app, const PulseShaderLibraryCreateDesc* desc);
PULSE_API PulseShaderLibraryHandle pulse_load_shader_library(PulseAppId app, const PulseShaderLibraryLoadDesc* desc);
PULSE_API bool pulse_acquire_shader_library(PulseAppId app, PulseShaderLibraryHandle handle, PulseShaderLibrary* out_ref);
PULSE_API void pulse_release_shader_library(PulseAppId app, PulseShaderLibrary* ref);

/**
 * Shader
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API PulseShaderHandle pulse_create_shader_from_binary(PulseAppId app, const PulseShaderCreateFromBinaryDesc* desc);
PULSE_API PulseShaderHandle pulse_create_shader_from_file(PulseAppId app, const PulseShaderCreateFromFileDesc* desc);
PULSE_API bool pulse_acquire_shader(PulseAppId app, PulseShaderHandle handle, PulseShader* out_ref);
PULSE_API void pulse_release_shader(PulseAppId app, PulseShader* ref);

/**
 * Compute shader
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API PulseComputeShaderHandle pulse_graphics_compute_shader_create_from_binary(PulseAppId app, const PulseComputeShaderCreateFromBinaryDesc* desc);
PULSE_API PulseComputeShaderHandle pulse_graphics_compute_shader_create_from_file(PulseAppId app, const PulseComputeShaderCreateFromFileDesc* desc);
PULSE_API bool pulse_acquire_compute_shader(PulseAppId app, PulseComputeShaderHandle handle, PulseComputeShader* out_ref);
PULSE_API void pulse_release_compute_shader(PulseAppId app, PulseComputeShader* ref);

/**
 * Buffer
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API PulseGraphicsBufferHandle pulse_create_graphics_buffer(PulseAppId app, const PulseGraphicsBufferCreateDesc* desc);
PULSE_API bool pulse_acquire_graphics_buffer(PulseAppId app, PulseGraphicsBufferHandle handle, PulseGraphicsBuffer* out_ref);
PULSE_API void pulse_release_graphics_buffer(PulseAppId app, PulseGraphicsBuffer* ref);

/**
 * Sampler
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API PulseSamplerHandle pulse_create_sampler(PulseAppId app, const PulseSamplerCreateDesc* desc);
PULSE_API bool pulse_acquire_sampler(PulseAppId app, PulseSamplerHandle handle, PulseSampler* out_ref);
PULSE_API void pulse_release_sampler(PulseAppId app, PulseSampler* ref);

/**
 * Texture
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API PulseTextureHandle pulse_create_texture(PulseAppId app, const PulseTextureCreateDesc* desc);
PULSE_API PulseTextureHandle pulse_load_texture(PulseAppId app, const PulseTextureLoadDesc* desc);
PULSE_API bool pulse_acquire_texture(PulseAppId app, PulseTextureHandle handle, PulseTexture* out_ref);
PULSE_API void pulse_release_texture(PulseAppId app, PulseTexture* ref);

/**
 * Mesh
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API PulseMeshHandle pulse_create_mesh_from_data(PulseAppId app, const PulseMeshCreateFromDataDesc* desc);
PULSE_API PulseMeshHandle pulse_create_mesh_dynamic(PulseAppId app, const PulseMeshCreateDynamicDesc* desc);
PULSE_API PulseMeshHandle pulse_load_mesh(PulseAppId app, const char* filepath);
PULSE_API void pulse_mesh_update_vertices(PulseAppId app, PulseMeshHandle* mesh, const void* data, uint32_t count);
PULSE_API void pulse_mesh_update_indices(PulseAppId app, PulseMeshHandle* mesh, const void* data, uint32_t count);
PULSE_API bool pulse_acquire_mesh(PulseAppId app, PulseMeshHandle handle, PulseMesh* out_ref);
PULSE_API void pulse_release_mesh(PulseAppId app, PulseMesh* ref);

/**
 * Material
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_API PulseMaterialHandle pulse_create_material(PulseAppId app, const PulseMaterialCreateDesc* desc);
PULSE_API bool pulse_acquire_material(PulseAppId app, PulseMaterialHandle handle, PulseMaterial* out_ref);
PULSE_API void pulse_release_material(PulseAppId app, PulseMaterial* ref);

/**
 * Material bind
 *
 * @param[in] set
 * @param[in] binding
 * @param[in] buffer
 *
 */
PULSE_API void pulse_material_bind_buffer(PulseMaterial* _this, uint32_t set, uint32_t binding, PulseGraphicsBuffer buffer);
PULSE_API void pulse_material_bind_texture(PulseMaterial* _this, uint32_t set, uint32_t binding, PulseTexture texture);
PULSE_API void pulse_material_bind_sampler(PulseMaterial* _this, uint32_t set, uint32_t binding, PulseSampler sampler);
PULSE_API void pulse_material_bind_data(PulseMaterial* _this, uint32_t set, uint32_t binding, size_t size, const void* data);

/**
 * Encoder
 *
 * @param[in] encoder
 * @param[in] material
 * @param[in] mesh
 *
 */
PULSE_API void pulse_renderpass_encoder_draw(pulse_renderpass_encoder_t* encoder, PulseMaterial material, PulseMesh mesh);
PULSE_API void pulse_renderpass_encoder_draw_submesh(pulse_renderpass_encoder_t* encoder, PulseMaterial material, PulseMesh mesh, uint32_t idx_count, uint32_t first_idx, uint32_t vtx_count, uint32_t first_vtx);
PULSE_API void pulse_renderpass_encoder_draw_procedure(pulse_renderpass_encoder_t* encoder, PulseMaterial material, ECGPUPrimitiveTopology topology, uint32_t vertex_count);
PULSE_API void pulse_renderpass_encoder_dispatch(pulse_renderpass_encoder_t* encoder, PulseComputeShader compute_shader, uint32_t x, uint32_t y, uint32_t z);
PULSE_API void pulse_renderpass_encoder_set_global_texture(pulse_renderpass_encoder_t* encoder, PulseTexture texture, uint32_t set, uint32_t binding);
PULSE_API void pulse_renderpass_encoder_set_global_buffer(pulse_renderpass_encoder_t* encoder, PulseGraphicsBuffer buffer, uint32_t set, uint32_t binding);
PULSE_API void pulse_renderpass_encoder_set_global_sampler(pulse_renderpass_encoder_t* encoder, PulseSampler sampler, uint32_t set, uint32_t binding);
PULSE_API void pulse_renderpass_encoder_set_global_texture_handle(pulse_renderpass_encoder_t* encoder, pulse_texture_handle_t handle, uint32_t set, uint32_t binding);
PULSE_API void pulse_renderpass_encoder_set_global_buffer_handle(pulse_renderpass_encoder_t* encoder, pulse_buffer_handle_t handle, uint32_t set, uint32_t binding);
PULSE_API void pulse_renderpass_encoder_set_global_buffer_offset(pulse_renderpass_encoder_t* encoder, pulse_buffer_handle_t handle, uint32_t set, uint32_t binding, uint64_t offset, uint64_t size);
PULSE_API void pulse_renderpass_encoder_set_viewport(pulse_renderpass_encoder_t* encoder, float x, float y, float width, float height, float min_depth, float max_depth);
PULSE_API void pulse_renderpass_encoder_set_scissor(pulse_renderpass_encoder_t* encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
PULSE_API void pulse_renderpass_encoder_push_constants(pulse_renderpass_encoder_t* encoder, PulseShader shader, const char* name, const void* data);

#ifdef __cplusplus
}
#endif

#endif // PULSE_GRAPHICS_API_HEADER_GUARD
