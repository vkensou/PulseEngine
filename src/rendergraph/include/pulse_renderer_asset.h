#pragma once

#include "cgpu/api.h"
#include "resource_type.h"

#ifdef __cplusplus
extern "C" {
#endif

struct pulse_texture_data_t
{
	CGPUTextureId handle;
	CGPUTextureViewId view;
	uint32_t cur_state_count;
	ECGPUResourceStateFlags* p_cur_states;
	bool states_consistent;
	bool prepared;
	pulse_texture_handle_t dynamic_handle;
};

struct pulse_buffer_data_t
{
	CGPUBufferId handle;
	ECGPUResourceTypeFlags type;
	ECGPUResourceStateFlags cur_state;
	pulse_buffer_handle_t dynamic_handle;
};

struct pulse_mesh_data_t
{
	CGPUVertexLayout vertex_layout;
	CGPUVertexAttribute* p_vertex_attributes;
	ECGPUPrimitiveTopology prim_topology;
	uint32_t vertex_stride;
	uint32_t index_stride;
	uint32_t vertices_count;
	uint32_t index_count;
	pulse_buffer_data_t* vertex_buffer;
	pulse_buffer_data_t* index_buffer;
	bool prepared;
};

struct pulse_shader_data_t
{
	CGPURootSignatureId root_sig;
	CGPUShaderEntryDescriptor vs;
	CGPUShaderEntryDescriptor ps;
	CGPUBlendStateDescriptor blend_desc;
	uint32_t blend_attachment_states_count;
	CGPUBlendAttachmentState* p_blend_attachment_states;
	CGPUDepthStateDescriptor depth_desc;
	CGPURasterizerStateDescriptor rasterizer_state;
};

struct pulse_compute_shader_data_t
{
	CGPURootSignatureId root_sig;
	CGPUShaderEntryDescriptor cs;
};

struct pulse_material_bind_buffer_t
{
	int set;
	int bind;
	pulse_buffer_data_t* buffer;
};

struct pulse_material_bind_buffer_array_t
{
	int size;
	int capacity;
	pulse_material_bind_buffer_t* data;
};

struct pulse_material_bind_texture_t
{
	int set;
	int bind;
	pulse_texture_data_t* texture;
};

struct pulse_material_bind_texture_array_t
{
	int size;
	int capacity;
	pulse_material_bind_texture_t* data;
};

struct pulse_material_bind_sampler_t
{
	int set;
	int bind;
	CGPUSamplerId sampler;
};

struct pulse_material_bind_sampler_array_t
{
	int size;
	int capacity;
	pulse_material_bind_sampler_t* data;
};

struct pulse_material_owned_buffer_array_t
{
	int size;
	int capacity;
	pulse_buffer_data_t** data;
};

struct pulse_material_data_t
{
	CGPUDeviceId device;
	pulse_shader_data_t* shader;
	pulse_material_bind_buffer_array_t buffers;
	pulse_material_bind_texture_array_t textures;
	pulse_material_bind_sampler_array_t samplers;
	pulse_material_owned_buffer_array_t ownedBuffers;
};

struct pulse_backbuffer_data_t
{
	pulse_texture_data_t texture;
};

typedef struct pulse_shader_library_data {
    CGPUShaderLibraryId library;
} pulse_shader_library_data_t;

typedef struct pulse_sampler_data {
    CGPUSamplerId handle;
} pulse_sampler_data_t;

#ifdef __cplusplus
}
#endif
