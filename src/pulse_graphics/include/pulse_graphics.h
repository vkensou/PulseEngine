#pragma once

#ifndef PULSE_GRAPHICS_API_HEADER_GUARD
#define PULSE_GRAPHICS_API_HEADER_GUARD

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t

#include "cgpu/api.h"
#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PULSE_API
#define PULSE_API
#endif

typedef struct pulse_backbuffer_data_t pulse_backbuffer_data_t;
typedef struct pulse_texture_data_t pulse_texture_data_t;
typedef struct pulse_buffer_data_t pulse_buffer_data_t;

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

#define PULSE_MAX_INDEX (UINT32_MAX - 2)


/**
 * Shader property role
 *
 */
typedef enum EPulseShaderPropertyRole
{
    PULSE_SHADER_PROPERTY_ROLE_MATERIAL,      /** ( 0)                                */
    PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL,  /** ( 1)                                */

    PULSE_SHADER_PROPERTY_ROLE_COUNT

} EPulseShaderPropertyRole;

/**
 * Shader property type
 *
 */
typedef enum EPulseShaderPropertyType
{
    PULSE_SHADER_PROPERTY_TYPE_UNKNOWN,       /** ( 0)                                */
    PULSE_SHADER_PROPERTY_TYPE_FLOAT,         /** ( 1)                                */
    PULSE_SHADER_PROPERTY_TYPE_FLOAT2,        /** ( 2)                                */
    PULSE_SHADER_PROPERTY_TYPE_FLOAT3,        /** ( 3)                                */
    PULSE_SHADER_PROPERTY_TYPE_FLOAT4,        /** ( 4)                                */
    PULSE_SHADER_PROPERTY_TYPE_INT,           /** ( 5)                                */
    PULSE_SHADER_PROPERTY_TYPE_MAT4,          /** ( 6)                                */
    PULSE_SHADER_PROPERTY_TYPE_TEXTURE,       /** ( 7)                                */
    PULSE_SHADER_PROPERTY_TYPE_SAMPLER,       /** ( 8)                                */

    PULSE_SHADER_PROPERTY_TYPE_COUNT

} EPulseShaderPropertyType;

typedef enum EPulseDepthBits
{
    PULSE_DEPTH_BITS_D32 = 32,
    PULSE_DEPTH_BITS_D24 = 24,
    PULSE_DEPTH_BITS_D16 = 16,
} EPulseDepthBits;




DEFINE_PULSE_OBJECT(PulseRenderGraph)

typedef struct PulseRenderPassEncoder PulseRenderPassEncoder;
typedef struct PulseUploadPassEncoder PulseUploadPassEncoder;

/**
 * Rendergraph function pointers
 *
 * @param[in] encoder
 * @param[in] userdata
 *
 */
typedef void (*PulseProcRenderPassExecutable)(PulseRenderPassEncoder* encoder, void* userdata);
typedef void (*PulseProcUploadpassExecutable)(PulseUploadPassEncoder* encoder, void* userdata);
/**
 * Function pointer: render record callback
 *
 * @param[in] app
 * @param[in] graph
 * @param[in] userData
 *
 */
typedef void (*PulseProcRenderRecordCallback)(PulseAppId app, PulseRenderGraphId graph, void* user_data);

struct PulseRenderGraph;
typedef struct PulseRenderGraph PulseRenderGraph;

typedef struct PulseRGTextureHandle
{
    uint32_t             index;

} PulseRGTextureHandle;

typedef struct PulseRGBufferHandle
{
    uint32_t             index;

} PulseRGBufferHandle;

typedef struct PulseRenderPassBuilder
{
    PulseRenderGraphId   render_graph;
    void*                pass_node;
    uint32_t             pass_index;

} PulseRenderPassBuilder;

typedef struct PulseComputePassBuilder
{
    PulseRenderGraphId   render_graph;
    void*                pass_node;
    uint32_t             pass_index;

} PulseComputePassBuilder;

struct PulseRenderPassEncoder;
typedef struct PulseRenderPassEncoder PulseRenderPassEncoder;

struct PulseUploadPassEncoder;
typedef struct PulseUploadPassEncoder PulseUploadPassEncoder;

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
 * Shader property descriptor
 *
 */
typedef struct PulseShaderPropertyDesc
{
    const char*          name;
    EPulseShaderPropertyType type;
    EPulseShaderPropertyRole role;
    uint32_t             set;
    uint32_t             binding;
    uint32_t             offset;
    uint32_t             size;

} PulseShaderPropertyDesc;

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
    uint32_t             property_count;
    const PulseShaderPropertyDesc* p_properties;

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
    uint32_t             property_count;
    const PulseShaderPropertyDesc* p_properties;

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
PULSE_API PulseRGTextureHandle pulse_import_window_backbuffer(PulseAppId app, PulseRenderGraphId graph, ecs_entity_t window_entity);

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
PULSE_API PulseComputeShaderHandle pulse_create_compute_shader_from_binary(PulseAppId app, const PulseComputeShaderCreateFromBinaryDesc* desc);
PULSE_API PulseComputeShaderHandle pulse_create_compute_shader_from_file(PulseAppId app, const PulseComputeShaderCreateFromFileDesc* desc);
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
PULSE_API void pulse_update_mesh_vertices(PulseAppId app, PulseMeshHandle* mesh, const void* data, uint32_t count);
PULSE_API void pulse_update_mesh_indices(PulseAppId app, PulseMeshHandle* mesh, const void* data, uint32_t count);
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
 * Material property setters (name-driven, Unity-style)
 *
 * @param[in] name
 * @param[in] value
 *
 */
PULSE_API void pulse_material_set_float4(PulseMaterial* _this, const char* name, HMM_Vec4 value);
PULSE_API void pulse_material_set_mat4(PulseMaterial* _this, const char* name, HMM_Mat4 value);
PULSE_API void pulse_material_set_texture(PulseMaterial* _this, const char* name, PulseTextureHandle texture);
PULSE_API void pulse_material_set_sampler(PulseMaterial* _this, const char* name, PulseSamplerHandle sampler);

/**
 * ======== Rendergraph Functions ========
 * Rendergraph lifecycle
 *
 * @param[in] estimateResourceCount
 * @param[in] estimatePassCount
 * @param[in] estimateEdgeCount
 * @param[in] blitShader
 * @param[in] blitSampler
 *
 */
PULSE_API PulseRenderGraphId pulse_create_render_graph(uint32_t estimate_resource_count, uint32_t estimate_pass_count, uint32_t estimate_edge_count, void* blit_shader, CGPUSamplerId blit_sampler);
PULSE_API void pulse_destroy_render_graph(PulseRenderGraphId render_graph);
PULSE_API void pulse_render_graph_reset(PulseRenderGraphId _this);

/**
 * Rendergraph handle validation
 *
 * @param[in] handle
 *
 */
PULSE_API bool pulse_rgtexture_handle_is_valid(PulseRGTextureHandle handle);
PULSE_API bool pulse_rgbuffer_handle_is_valid(PulseRGBufferHandle handle);

/**
 * Rendergraph resource declaration/import
 *
 */
PULSE_API PulseRGTextureHandle pulse_render_graph_declare_texture(PulseRenderGraphId _this);
PULSE_API PulseRGTextureHandle pulse_render_graph_import_texture(PulseRenderGraphId _this, pulse_texture_data_t* imported);
PULSE_API PulseRGTextureHandle pulse_render_graph_import_backbuffer(PulseRenderGraphId _this, pulse_backbuffer_data_t* imported_backbuffer);
PULSE_API PulseRGBufferHandle pulse_render_graph_declare_buffer(PulseRenderGraphId _this);
PULSE_API PulseRGBufferHandle pulse_render_graph_import_buffer(PulseRenderGraphId _this, pulse_buffer_data_t* imported);
PULSE_API PulseRGBufferHandle pulse_render_graph_import_dynamic_buffer(PulseRenderGraphId _this, void* imported);
PULSE_API PulseRGBufferHandle pulse_render_graph_declare_uniform_buffer_quick(PulseRenderGraphId _this, uint32_t size, void* data);
PULSE_API PulseRGTextureHandle pulse_render_graph_declare_texture_subresource(PulseRenderGraphId _this, PulseRGTextureHandle parent, uint8_t mip_level, uint8_t array_slice);

/**
 * Rendergraph texture properties
 *
 * @param[in] texture
 * @param[in] width
 * @param[in] height
 * @param[in] depth
 *
 */
PULSE_API void pulse_render_graph_texture_set_extent(PulseRenderGraphId _this, PulseRGTextureHandle texture, uint32_t width, uint32_t height, uint32_t depth);
PULSE_API void pulse_render_graph_texture_set_format(PulseRenderGraphId _this, PulseRGTextureHandle texture, ECGPUTextureFormat format);
PULSE_API void pulse_render_graph_texture_set_depth_format(PulseRenderGraphId _this, PulseRGTextureHandle texture, EPulseDepthBits depth_bits, bool need_stencil);
PULSE_API uint32_t pulse_render_graph_texture_get_width(PulseRenderGraphId _this, PulseRGTextureHandle texture);
PULSE_API uint32_t pulse_render_graph_texture_get_height(PulseRenderGraphId _this, PulseRGTextureHandle texture);
PULSE_API uint32_t pulse_render_graph_texture_get_depth(PulseRenderGraphId _this, PulseRGTextureHandle texture);
PULSE_API ECGPUTextureFormat pulse_render_graph_texture_get_format(PulseRenderGraphId _this, PulseRGTextureHandle texture);

/**
 * Rendergraph buffer properties
 *
 * @param[in] buffer
 * @param[in] size_
 *
 */
PULSE_API void pulse_render_graph_buffer_set_size(PulseRenderGraphId _this, PulseRGBufferHandle buffer, uint32_t size);
PULSE_API void pulse_render_graph_buffer_set_type(PulseRenderGraphId _this, PulseRGBufferHandle buffer, ECGPUResourceTypeFlags type);
PULSE_API void pulse_render_graph_buffer_set_usage(PulseRenderGraphId _this, PulseRGBufferHandle buffer, ECGPUMemoryUsage usage);
PULSE_API void pulse_render_graph_buffer_set_hold_on_last(PulseRenderGraphId _this, PulseRGBufferHandle buffer);

/**
 * Rendergraph pass management
 *
 * @param[in] name
 *
 */
PULSE_API PulseRenderPassBuilder pulse_render_graph_add_render_pass(PulseRenderGraphId _this, const char* name);
PULSE_API PulseComputePassBuilder pulse_render_graph_add_compute_pass(PulseRenderGraphId _this, const char* name);
PULSE_API PulseRenderPassBuilder pulse_render_graph_add_holdpass(PulseRenderGraphId _this, const char* name);
PULSE_API void pulse_render_graph_add_uploadtexturepass(PulseRenderGraphId _this, const char* name, PulseRGTextureHandle texture, uint8_t mip_level, uint8_t slice, PulseProcUploadpassExecutable executable, uint32_t passdata_size, void** out_passdata);
PULSE_API void pulse_render_graph_add_uploadtexturepass_ex(PulseRenderGraphId _this, const char* name, PulseRGTextureHandle texture, uint8_t mip_level, uint8_t slice, uint64_t size, uint64_t offset, void* data, PulseProcUploadpassExecutable executable, uint32_t passdata_size, void** out_passdata);
PULSE_API void pulse_render_graph_add_uploadbufferpass(PulseRenderGraphId _this, const char* name, PulseRGBufferHandle buffer, PulseProcUploadpassExecutable executable, uint32_t passdata_size, void** out_passdata);
PULSE_API void pulse_render_graph_add_uploadbufferpass_ex(PulseRenderGraphId _this, const char* name, PulseRGBufferHandle buffer, uint64_t size, uint64_t offset, void* data, PulseProcUploadpassExecutable executable, uint32_t passdata_size, void** out_passdata);
PULSE_API void pulse_render_graph_add_generate_mipmap(PulseRenderGraphId _this, PulseRGTextureHandle texture, uint8_t from_mip_level);
PULSE_API void pulse_render_graph_present(PulseRenderGraphId _this, PulseRGTextureHandle texture);
PULSE_API uint32_t pulse_render_graph_add_edge(PulseRenderGraphId _this, uint32_t from, uint32_t to, ECGPUResourceStateFlags usage);

/**
 * RenderPass builder
 *
 * @param[in] texture
 * @param[in] loadAction
 * @param[in] clearColor
 * @param[in] storeAction
 *
 */
PULSE_API void pulse_render_pass_builder_add_color_attachment(PulseRenderPassBuilder* _this, PulseRGTextureHandle texture, ECGPULoadAction load_action, uint32_t clear_color, ECGPUStoreAction store_action);
PULSE_API void pulse_render_pass_builder_add_depth_attachment(PulseRenderPassBuilder* _this, PulseRGTextureHandle texture, ECGPULoadAction depth_load_action, float clear_depth, ECGPUStoreAction depth_store_action, ECGPULoadAction stencil_load_action, uint8_t clear_stencil, ECGPUStoreAction stencil_store_action);
PULSE_API void pulse_render_pass_builder_sample(PulseRenderPassBuilder* _this, PulseRGTextureHandle texture);
PULSE_API void pulse_render_pass_builder_use_buffer(PulseRenderPassBuilder* _this, PulseRGBufferHandle buffer);
PULSE_API void pulse_render_pass_builder_use_buffer_as(PulseRenderPassBuilder* _this, PulseRGBufferHandle buffer, ECGPUResourceStateFlags state);
PULSE_API void pulse_render_pass_builder_set_executable(PulseRenderPassBuilder* _this, PulseProcRenderPassExecutable executable, uint32_t passdata_size, void** out_passdata);

/**
 * ComputePass builder
 *
 * @param[in] texture
 *
 */
PULSE_API void pulse_compute_pass_builder_sample(PulseComputePassBuilder* _this, PulseRGTextureHandle texture);
PULSE_API void pulse_compute_pass_builder_use_buffer(PulseComputePassBuilder* _this, PulseRGBufferHandle buffer);
PULSE_API void pulse_compute_pass_builder_use_buffer_as(PulseComputePassBuilder* _this, PulseRGBufferHandle buffer, ECGPUResourceStateFlags state);
PULSE_API void pulse_compute_pass_builder_readwrite_texture(PulseComputePassBuilder* _this, PulseRGTextureHandle texture);
PULSE_API void pulse_compute_pass_builder_readwrite_buffer(PulseComputePassBuilder* _this, PulseRGBufferHandle buffer);
PULSE_API void pulse_compute_pass_builder_set_executable(PulseComputePassBuilder* _this, PulseProcRenderPassExecutable executable, uint32_t passdata_size, void** out_passdata);

/**
 * Encoder
 *
 * @param[in] material
 * @param[in] mesh
 *
 */
PULSE_API void pulse_render_pass_encoder_draw(PulseRenderPassEncoder* _this, PulseMaterial material, PulseMesh mesh);
PULSE_API void pulse_render_pass_encoder_draw_submesh(PulseRenderPassEncoder* _this, PulseMaterial material, PulseMesh mesh, uint32_t idx_count, uint32_t first_idx, uint32_t vtx_count, uint32_t first_vtx);
PULSE_API void pulse_render_pass_encoder_draw_procedure(PulseRenderPassEncoder* _this, PulseMaterial material, ECGPUPrimitiveTopology topology, uint32_t vertex_count);
PULSE_API void pulse_render_pass_encoder_dispatch(PulseRenderPassEncoder* _this, PulseComputeShader compute_shader, uint32_t x, uint32_t y, uint32_t z);
PULSE_API void pulse_render_pass_encoder_set_global_texture(PulseRenderPassEncoder* _this, PulseTexture texture, uint32_t set, uint32_t binding);
PULSE_API void pulse_render_pass_encoder_set_global_buffer(PulseRenderPassEncoder* _this, PulseGraphicsBuffer buffer, uint32_t set, uint32_t binding);
PULSE_API void pulse_render_pass_encoder_set_global_sampler(PulseRenderPassEncoder* _this, PulseSampler sampler, uint32_t set, uint32_t binding);
PULSE_API void pulse_render_pass_encoder_set_global_texture_handle(PulseRenderPassEncoder* _this, PulseRGTextureHandle handle, uint32_t set, uint32_t binding);
PULSE_API void pulse_render_pass_encoder_set_global_buffer_handle(PulseRenderPassEncoder* _this, PulseRGBufferHandle handle, uint32_t set, uint32_t binding);
PULSE_API void pulse_render_pass_encoder_set_global_buffer_offset(PulseRenderPassEncoder* _this, PulseRGBufferHandle handle, uint32_t set, uint32_t binding, uint64_t offset, uint64_t size);
PULSE_API void pulse_render_pass_encoder_set_viewport(PulseRenderPassEncoder* _this, float x, float y, float width, float height, float min_depth, float max_depth);
PULSE_API void pulse_render_pass_encoder_set_scissor(PulseRenderPassEncoder* _this, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
PULSE_API void pulse_render_pass_encoder_push_constants(PulseRenderPassEncoder* _this, PulseShader shader, const char* name, const void* data);
PULSE_API CGPUBufferId pulse_render_pass_encoder_resolve_buffer(PulseRenderPassEncoder* _this, PulseRGBufferHandle buffer_handle);
PULSE_API CGPUTextureViewId pulse_render_pass_encoder_resolve_texture_view(PulseRenderPassEncoder* _this, PulseRGTextureHandle texture_handle);

static PULSE_FORCEINLINE bool ShaderPropertyIsUniform(EPulseShaderPropertyType const arg) {
    switch(arg) {
        case PULSE_SHADER_PROPERTY_TYPE_FLOAT: return true;
        case PULSE_SHADER_PROPERTY_TYPE_FLOAT2: return true;
        case PULSE_SHADER_PROPERTY_TYPE_FLOAT3: return true;
        case PULSE_SHADER_PROPERTY_TYPE_FLOAT4: return true;
        case PULSE_SHADER_PROPERTY_TYPE_INT: return true;
        case PULSE_SHADER_PROPERTY_TYPE_MAT4: return true;
        default: return false;
    }
    return false;
}


#ifdef __cplusplus
}
#endif

#endif // PULSE_GRAPHICS_API_HEADER_GUARD
