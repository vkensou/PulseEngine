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

struct pulse_material_data_t
{
	struct BindBuffer
	{
		int set;
		int bind;
		pulse_buffer_data_t* buffer;
	};

	struct BindTexture
	{
		int set;
		int bind;
		pulse_texture_data_t* texture;
	};

	struct BindSampler
	{
		int set;
		int bind;
		CGPUSamplerId sampler;
	};

	CGPUDeviceId device;
	pulse_shader_data_t* shader;
	int buffers_size;
	int buffers_capacity;
	BindBuffer* buffers_data;
	int textures_size;
	int textures_capacity;
	BindTexture* textures_data;
	int samplers_size;
	int samplers_capacity;
	BindSampler* samplers_data;
	int ownedBuffers_size;
	int ownedBuffers_capacity;
	pulse_buffer_data_t** ownedBuffers_data;
};

#ifdef __cplusplus
}
#endif
