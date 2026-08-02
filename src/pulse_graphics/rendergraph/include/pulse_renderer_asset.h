#pragma once

#include "cgpu/api.h"
#include "resource_type.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

struct PulseTextureData
{
	CGPUTextureId handle;
	CGPUTextureViewId view;
	uint32_t cur_state_count;
	ECGPUResourceStateFlags* p_cur_states;
	bool states_consistent;
	bool prepared;
	PulseRGTextureHandle dynamic_handle;
};

struct PulseGraphicsBufferData
{
	CGPUBufferId handle;
	ECGPUResourceTypeFlags type;
	ECGPUResourceStateFlags cur_state;
	PulseRGBufferHandle dynamic_handle;
};

struct PulseMeshData
{
	CGPUVertexLayout vertex_layout;
	CGPUVertexAttribute* p_vertex_attributes;
	ECGPUPrimitiveTopology prim_topology;
	uint32_t vertex_stride;
	uint32_t index_stride;
	uint32_t vertices_count;
	uint32_t index_count;
	PulseGraphicsBufferData* vertex_buffer;
	PulseGraphicsBufferData* index_buffer;
	bool prepared;
};

struct pulse_shader_property_t
{
    const char* name;
    int type;
    int role;
    uint32_t set;
    uint32_t binding;
    uint32_t offset;
    uint32_t size;
};

struct pulse_shader_ubo_info_t
{
    uint64_t layout_hash;
    uint32_t set;
    uint32_t binding;
    uint32_t ubo_size;
	bool material_managed;
	bool renderer_managed;
};

struct pulse_shader_set_info_t
{
    uint32_t set_index;
    bool renderer_managed;
    uint64_t layout_hash;
};

struct PulseShaderData
{
	CGPURootSignatureId root_sig;
	CGPUShaderEntryDescriptor vs;
	CGPUShaderEntryDescriptor ps;
	CGPUBlendStateDescriptor blend_desc;
	uint32_t blend_attachment_states_count;
	CGPUBlendAttachmentState* p_blend_attachment_states;
	CGPUDepthStateDescriptor depth_desc;
	CGPURasterizerStateDescriptor rasterizer_state;
	uint32_t property_count;
	pulse_shader_property_t* p_properties;
	uint32_t ubo_info_count;
	pulse_shader_ubo_info_t* p_ubo_infos;
	uint32_t set_info_count;
	pulse_shader_set_info_t* p_set_infos;
};

struct PulseComputeShaderData
{
	CGPURootSignatureId root_sig;
	CGPUShaderEntryDescriptor cs;
};

struct pulse_material_bind_buffer_t
{
	int set;
	int bind;
	PulseGraphicsBufferData* buffer;
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
	PulseTextureData* texture;
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

struct pulse_material_ubo_column_t
{
	uint32_t set;
	uint32_t binding;
	uint8_t* cpu_data;
	uint32_t size;
	bool dirty;
	PulseGraphicsBufferData* gpu_buffer;
};

struct pulse_material_ubo_columns_t
{
	int size;
	int capacity;
	pulse_material_ubo_column_t* data;
};

struct pulse_material_owned_buffer_array_t
{
	int size;
	int capacity;
	PulseGraphicsBufferData** data;
};

struct pulse_material_descriptor_set_t
{
    uint32_t set_index;
    CGPUDescriptorSetId handle;
    uint64_t data_hash;
    bool binding_dirty;
};

struct pulse_material_descriptor_set_array_t
{
    int size;
    int capacity;
    pulse_material_descriptor_set_t* data;
};

struct PulseMaterialData
{
	CGPUDeviceId device;
	PulseShaderData* shader;
	pulse_material_bind_buffer_array_t buffers;
	pulse_material_bind_texture_array_t textures;
	pulse_material_bind_sampler_array_t samplers;
	pulse_material_ubo_columns_t uboColumns;
	pulse_material_owned_buffer_array_t ownedBuffers;
	pulse_material_descriptor_set_array_t materialDsets;
};

struct pulse_backbuffer_data_t
{
	PulseTextureData texture;
};

typedef struct PulseShaderLibraryData {
    CGPUShaderLibraryId library;
} PulseShaderLibraryData;

typedef struct PulseSamplerData {
    CGPUSamplerId handle;
} PulseSamplerData;

static inline const pulse_shader_property_t* pulse_find_shader_property(const PulseShaderData* shader, const char* name)
{
	if (!shader || !shader->p_properties || !name) return nullptr;
	for (uint32_t i = 0; i < shader->property_count; ++i) {
		if (shader->p_properties[i].name && strcmp(shader->p_properties[i].name, name) == 0)
			return &shader->p_properties[i];
	}
	return nullptr;
}

#ifdef __cplusplus
}
#endif
