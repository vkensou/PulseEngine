#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "cgpu/api.h"
#include "pulse_asset.h"
#include "rendergraph.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_GRAPHIC_PLUGIN_DESC_VERSION 1u

#define PULSE_TYPE_SHADER          UINT64_C(0x1000)
#define PULSE_TYPE_COMPUTE_SHADER  UINT64_C(0x1001)
#define PULSE_TYPE_MESH            UINT64_C(0x1002)
#define PULSE_TYPE_TEXTURE         UINT64_C(0x1003)
#define PULSE_TYPE_BUFFER          UINT64_C(0x1004)
#define PULSE_TYPE_MATERIAL        UINT64_C(0x1005)
#define PULSE_TYPE_SAMPLER         UINT64_C(0x1006)

typedef struct pulse_shader_t   { pulse_asset_handle asset; } pulse_shader_t;
typedef struct pulse_mesh_t     { pulse_asset_handle asset; } pulse_mesh_t;
typedef struct pulse_texture_t  { pulse_asset_handle asset; } pulse_texture_t;
typedef struct pulse_buffer_t   { pulse_asset_handle asset; } pulse_buffer_t;
typedef struct pulse_material_t { pulse_asset_handle asset; } pulse_material_t;
typedef struct pulse_sampler_t  { pulse_asset_handle asset; } pulse_sampler_t;

typedef struct pulse_shader_data {
    CGPURootSignatureId root_sig;
    CGPUShaderEntryDescriptor vs;
    CGPUShaderEntryDescriptor ps;
    CGPUBlendStateDescriptor blend_desc;
    CGPUBlendAttachmentState blend_attachments[8];
    CGPUDepthStateDescriptor depth_desc;
    CGPURasterizerStateDescriptor rasterizer_state;
} pulse_shader_data_t;

typedef struct pulse_compute_shader_data {
    CGPURootSignatureId root_sig;
    CGPUShaderEntryDescriptor cs;
} pulse_compute_shader_data_t;

typedef struct pulse_mesh_data {
    CGPUVertexLayout vertex_layout;
    uint32_t vertex_stride;
    uint32_t index_stride;
    uint32_t vertices_count;
    uint32_t index_count;
    ECGPUPrimitiveTopology prim_topology;
    CGPUBufferId vertex_buffer;
    CGPUBufferId index_buffer;
    bool has_index_buffer;
} pulse_mesh_data_t;

typedef struct pulse_texture_data {
    CGPUTextureId handle;
    CGPUTextureViewId view;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
    uint32_t mip_levels;
    ECGPUTextureFormat format;
} pulse_texture_data_t;

typedef struct pulse_buffer_data {
    CGPUBufferId handle;
    ECGPUResourceTypeFlags type;
    uint64_t size;
} pulse_buffer_data_t;

typedef struct pulse_material_data {
    pulse_asset_handle shader;
} pulse_material_data_t;

typedef struct pulse_sampler_data {
    CGPUSamplerId handle;
} pulse_sampler_data_t;

typedef struct pulse_graphic_plugin_desc {
    uint32_t struct_size;
    uint32_t version;
} pulse_graphic_plugin_desc;

pulse_graphic_plugin_desc pulse_graphic_plugin_desc_default(void);
pulse_result_t pulse_graphic_add_plugin(pulse_app_t app, const pulse_graphic_plugin_desc* desc);

bool pulse_graphic_is_available(pulse_app_t app, pulse_shader_t handle);

pulse_shader_t pulse_graphic_shader_create_from_binary(
    pulse_app_t app,
    const void* vs_data, uint32_t vs_size,
    const void* fs_data, uint32_t fs_size,
    const CGPUBlendStateDescriptor* blend_desc,
    const CGPUDepthStateDescriptor* depth_desc,
    const CGPURasterizerStateDescriptor* rasterizer_state);

pulse_shader_t pulse_graphic_compute_shader_create_from_binary(
    pulse_app_t app,
    const void* cs_data, uint32_t cs_size);

pulse_shader_t pulse_graphic_shader_load(
    pulse_app_t app,
    const char* vert_path,
    const char* frag_path,
    const CGPUBlendStateDescriptor* blend_desc,
    const CGPUDepthStateDescriptor* depth_desc,
    const CGPURasterizerStateDescriptor* rasterizer_state);

pulse_shader_t pulse_graphic_compute_shader_load(
    pulse_app_t app,
    const char* comp_path);

pulse_shader_data_t* pulse_graphic_shader_acquire(pulse_app_t app, pulse_shader_t* handle);
pulse_compute_shader_data_t* pulse_graphic_compute_shader_acquire(pulse_app_t app, pulse_shader_t* handle);
void pulse_graphic_shader_release(pulse_app_t app, pulse_shader_t* handle);

pulse_buffer_t pulse_graphic_buffer_create(
    pulse_app_t app,
    const CGPUBufferDescriptor* desc,
    const void* data, uint64_t data_size);

pulse_buffer_data_t* pulse_graphic_buffer_acquire(pulse_app_t app, pulse_buffer_t* handle);
void pulse_graphic_buffer_release(pulse_app_t app, pulse_buffer_t* handle);

pulse_sampler_t pulse_graphic_sampler_create(
    pulse_app_t app,
    const CGPUSamplerDescriptor* desc);

pulse_sampler_data_t* pulse_graphic_sampler_acquire(pulse_app_t app, pulse_sampler_t* handle);
void pulse_graphic_sampler_release(pulse_app_t app, pulse_sampler_t* handle);

pulse_texture_t pulse_graphic_texture_create_from_data(
    pulse_app_t app,
    const CGPUTextureDescriptor* desc,
    const void* pixel_data, uint64_t pixel_data_size);

pulse_texture_t pulse_graphic_texture_load(
    pulse_app_t app,
    const char* filepath,
    bool mipmap);

pulse_texture_data_t* pulse_graphic_texture_acquire(pulse_app_t app, pulse_texture_t* handle);
void pulse_graphic_texture_release(pulse_app_t app, pulse_texture_t* handle);

pulse_mesh_t pulse_graphic_mesh_create_from_data(
    pulse_app_t app,
    const void* vertex_data, uint32_t vertex_count, uint32_t vertex_stride,
    const void* index_data,  uint32_t index_count,  uint32_t index_stride,
    ECGPUPrimitiveTopology topology,
    const CGPUVertexLayout* layout);

pulse_mesh_t pulse_graphic_mesh_create_dynamic(
    pulse_app_t app,
    uint32_t max_vertex_count, uint32_t vertex_stride,
    uint32_t max_index_count,  uint32_t index_stride,
    ECGPUPrimitiveTopology topology,
    const CGPUVertexLayout* layout);

pulse_mesh_t pulse_graphic_mesh_load(
    pulse_app_t app,
    const char* filepath);

void pulse_graphic_mesh_update_vertices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count);
void pulse_graphic_mesh_update_indices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count);

pulse_mesh_data_t* pulse_graphic_mesh_acquire(pulse_app_t app, pulse_mesh_t* handle);
void pulse_graphic_mesh_release(pulse_app_t app, pulse_mesh_t* handle);

#ifdef __cplusplus
}
#endif
