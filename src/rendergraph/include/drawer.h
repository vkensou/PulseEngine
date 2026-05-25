#pragma once

#include "stdint.h"
#include "resource_type.h"
#include "cgpu/api.h"
#include "renderer.h"

struct pulse_texture_data_t;
struct pulse_buffer_data_t;
struct pulse_mesh_data_t;
struct pulse_shader_data_t;
struct pulse_compute_shader_data_t;
struct pulse_material_data_t;

namespace HGEGraphics
{
	struct RenderPassEncoder;
	struct UploadEncoder;

	void set_viewport(RenderPassEncoder* encoder, float x, float y, float width, float height, float min_depth, float max_depth);
	void set_scissor(RenderPassEncoder* encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	void push_constants(RenderPassEncoder* encoder, pulse_shader_data_t* shader, const char* name, const void* data);
	void draw(RenderPassEncoder* encoder, pulse_shader_data_t* shader, pulse_mesh_data_t* mesh);
	void draw_submesh(RenderPassEncoder* encoder, pulse_shader_data_t* shader, pulse_mesh_data_t* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex);
	void draw_procedure(RenderPassEncoder* encoder, pulse_shader_data_t* shader, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count);
	void draw(RenderPassEncoder* encoder, pulse_material_data_t* material, pulse_mesh_data_t* mesh);
	void draw_submesh(RenderPassEncoder* encoder, pulse_material_data_t* material, pulse_mesh_data_t* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex);
	void draw_procedure(RenderPassEncoder* encoder, pulse_material_data_t* material, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count);
	void dispatch(RenderPassEncoder* encoder, pulse_compute_shader_data_t* shader, uint32_t thread_x, uint32_t thread_y, uint32_t thread_z);
	void set_global_texture(RenderPassEncoder* encoder, pulse_texture_data_t* texture, int set, int slot);
	void set_global_texture_handle(RenderPassEncoder* encoder, pulse_texture_handle_t texture, int set, int slot);
	void set_global_sampler(RenderPassEncoder* encoder, CGPUSamplerId sampler, int set, int slot);
	void set_global_buffer(RenderPassEncoder* encoder, pulse_buffer_data_t* buffer, int set, int slot);
	void set_global_dynamic_buffer(RenderPassEncoder* encoder, pulse_buffer_handle_t buffer, int set, int slot);
	void set_global_buffer_with_offset_size(RenderPassEncoder* encoder, pulse_buffer_handle_t buffer, int set, int slot, uint64_t offset, uint64_t size);
	void upload(UploadEncoder* encoder, uint64_t offset, uint64_t length, void* data);
}