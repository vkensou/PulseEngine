#pragma once

#include <stdint.h>
#include "cgpu/api.h"
#include "renderer.h"

struct PulseTextureData;
struct PulseGraphicsBufferData;
struct PulseMeshData;
struct PulseShaderData;
struct PulseComputeShaderData;
struct PulseMaterialData;

namespace HGEGraphics
{
	void set_viewport(RenderPassEncoder* encoder, float x, float y, float width, float height, float min_depth, float max_depth);
	void set_scissor(RenderPassEncoder* encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
	void push_constants(RenderPassEncoder* encoder, PulseShaderData* shader, const char* name, const void* data);
	void draw(RenderPassEncoder* encoder, PulseShaderData* shader, PulseMeshData* mesh);
	void draw_submesh(RenderPassEncoder* encoder, PulseShaderData* shader, PulseMeshData* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex);
	void draw_procedure(RenderPassEncoder* encoder, PulseShaderData* shader, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count);
	void draw(RenderPassEncoder* encoder, PulseMaterialData* material, PulseMeshData* mesh);
	void draw_submesh(RenderPassEncoder* encoder, PulseMaterialData* material, PulseMeshData* mesh, uint32_t index_count, uint32_t first_index, uint32_t vertex_count, uint32_t first_vertex);
	void draw_procedure(RenderPassEncoder* encoder, PulseMaterialData* material, ECGPUPrimitiveTopology mesh_topology, uint32_t vertex_count);
	void dispatch(RenderPassEncoder* encoder, PulseComputeShaderData* shader, uint32_t thread_x, uint32_t thread_y, uint32_t thread_z);
	void set_global_texture(RenderPassEncoder* encoder, PulseTextureData* texture, int set, int slot);
	void set_global_texture_handle(RenderPassEncoder* encoder, PulseRGTextureHandle texture, int set, int slot);
	void set_global_sampler(RenderPassEncoder* encoder, CGPUSamplerId sampler, int set, int slot);
	void set_global_buffer(RenderPassEncoder* encoder, PulseGraphicsBufferData* buffer, int set, int slot);
	void set_global_dynamic_buffer(RenderPassEncoder* encoder, PulseRGBufferHandle buffer, int set, int slot);
	void set_global_buffer_with_offset_size(RenderPassEncoder* encoder, PulseRGBufferHandle buffer, int set, int slot, uint64_t offset, uint64_t size);
	void upload(UploadEncoder* encoder, uint64_t offset, uint64_t length, const void* data);
}
