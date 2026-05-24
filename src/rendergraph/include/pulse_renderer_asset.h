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

#ifdef __cplusplus
}
#endif
