#pragma once

#include "cgpu/api.h"
#include <string>
#include "hash.h"
#include <unordered_map>
#include <memory_resource>
#include "texturepool.h"
#include "renderpasspool.h"
#include "framebufferpool.h"
#include "graphicspipelinepool.h"
#include "computepipelinepool.h"
#include "textureviewpool.h"
#include "bufferpool.h"
#include "descriptorsetpool.h"
#include <optional>
#include "profiler.h"
#include "resource_type.h"
#include "rendergraph.h"
#include "pulse_renderer_asset.h"

namespace HGEGraphics
{
	std::unique_ptr<PulseShaderData> create_shader(CGPUDeviceId device, const uint8_t* vert_data, uint32_t vert_length, const uint8_t* frag_data, uint32_t frag_length, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state);

	std::unique_ptr<PulseShaderData> create_shader_from_libraries(
		CGPUDeviceId device,
		CGPUShaderLibraryId vs_library,
		CGPUShaderLibraryId ps_library,
		const CGPUBlendStateDescriptor& blend_desc,
		const CGPUDepthStateDescriptor& depth_desc,
		const CGPURasterizerStateDescriptor& rasterizer_state);
	void free_shader(PulseShaderData* shader);

	std::unique_ptr<PulseComputeShaderData> create_compute_shader(CGPUDeviceId device, const uint8_t* comp_data, uint32_t comp_length);

	std::unique_ptr<PulseComputeShaderData> create_compute_shader_from_library(
		CGPUDeviceId device,
		CGPUShaderLibraryId cs_library);

	void free_compute_shader(PulseComputeShaderData* compute_shader);

	PulseGraphicsBufferData* create_empty_buffer();
	PulseGraphicsBufferData* create_buffer(CGPUDeviceId device, const CGPUBufferDescriptor& desc);
	void free_buffer(PulseGraphicsBufferData* buffer);

	std::unique_ptr<PulseMeshData> create_empty_mesh();
	void init_mesh(PulseMeshData* mesh, CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader);
	std::unique_ptr<PulseMeshData> create_mesh(CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader);
	std::unique_ptr<PulseMeshData> create_dynamic_mesh(ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride);
	PulseRGBufferHandle declare_dynamic_vertex_buffer(PulseMeshData* mesh, PulseRenderGraphId rg, uint32_t count);
	PulseRGBufferHandle declare_dynamic_index_buffer(PulseMeshData* mesh, PulseRenderGraphId rg, uint32_t count);
	void dynamic_mesh_reset(PulseMeshData* mesh);
	void free_mesh(PulseMeshData* mesh);

	PulseTextureData* create_empty_texture();
	void init_texture(PulseTextureData* texture, CGPUDeviceId device, const CGPUTextureDescriptor& desc);
	PulseTextureData* create_texture(CGPUDeviceId device, const CGPUTextureDescriptor& desc);
	void free_texture(PulseTextureData* texture);

	void init_material(PulseMaterialData* material, CGPUDeviceId device, PulseShaderData* shader);
	void free_material(PulseMaterialData* material);

	void material_mark_dset_binding_dirty(PulseMaterialData* material, uint32_t set_index);
	void material_bindTexture(PulseMaterialData* material, int set, int bind, PulseTextureData* texture);
	void material_bindSampler(PulseMaterialData* material, int set, int bind, PulseSamplerData* sampler);
	void material_bindSampler(PulseMaterialData* material, int set, int bind, CGPUSamplerId sampler);
	void material_bindBuffer(PulseMaterialData* material, int set, int bind, PulseGraphicsBufferData* buffer);
	pulse_material_ubo_column_t* material_find_ubo_column(PulseMaterialData* material, uint32_t set, uint32_t binding);
	void material_ubo_sync_to_gpu(PulseMaterialData* material);
	const uint8_t* material_get_property_data(PulseMaterialData* material, uint32_t set, uint32_t binding, uint32_t offset, uint32_t size);

	void init_backbuffer(pulse_backbuffer_data_t* backbuffer, CGPUSwapChainId swapchain, int index);
	void free_backbuffer(pulse_backbuffer_data_t* backbuffer);

	struct RenderPassEncoder;

	struct ShaderTextureBinder
	{
		PulseTextureData* texture;
		PulseRGTextureHandle texture_handle;
		int set, bind;
	};

	struct ShaderSamplerBinder
	{
		CGPUSamplerId sampler;
		int set, bind;
	};

	struct ShaderBufferBinder
	{
		PulseGraphicsBufferData* buffer;
		PulseRGBufferHandle buffer_handle;
		int set, bind;
		uint64_t offset, size;
	};

	struct ExecutorContext
	{
		std::pmr::memory_resource* memory_resource = nullptr;
		CgpuTexturePool texturePool;
		RenerPassPool renderPassPool;
		FramebufferPool framebufferPool;
		GraphicsPipelinePool pipelinePool;
		ComputePipelinePool computePipelinePool;
		TextureViewPool textureViewPool;
		BufferPool bufferPool;
		CGPUCommandPoolId cmdPool = { CGPU_NULLPTR };
		std::pmr::vector<CGPUCommandBufferId> cmds;
		std::pmr::vector<CGPUCommandBufferId> allocated_cmds;
		DescriptorSetPool descriptorSetPool;
		std::pmr::vector<DescriptorSet*> allocated_dsets;
		CGPUDeviceId device = { CGPU_NULLPTR };
		uint64_t timestamp = { 0 };
		Profiler* profiler = nullptr;
		double gpuTicksPerSecond = 0;
		CGPUTextureViewId default_texture = CGPU_NULLPTR;
		CGPUSamplerId default_sampler = CGPU_NULLPTR;
		bool support_shading_rate;

		ExecutorContext(CGPUDeviceId device, CGPUQueueId gfx_queue, bool profile, std::pmr::memory_resource* memory_resource);

		void newFrame();

		CGPUCommandBufferId requestCmd();

		void destroy();
		void pre_destroy();
	};

	struct CompiledRenderGraph;
	struct RenderPassEncoder
	{
		CGPURenderPassEncoderId encoder;
		CGPUComputePassEncoderId compute_encoder;
		CGPUStateBufferId state_buffer;
		CGPURasterStateEncoderId raster_state_encoder;
		CGPURenderPassId render_pass;
		uint32_t subpass;
		uint32_t render_target_count;
		ExecutorContext* context;
		CompiledRenderGraph* compiled_graph;
		CGPURenderPipelineId last_render_pipeline;
		CGPUComputePipelineId last_compute_pipeline;
		CGPUDescriptorData last_bind_resources[4][64]{ 0 };
		CGPUTextureViewId last_textureviews[4][64]{ 0 };
		CGPUSamplerId last_samplers[4][64]{ 0 };
		CGPUBufferId last_buffers[4][64]{ 0 };
		uint64_t last_buffer_offset_sizes[4][128]{ 0 };
		CGPUTextureViewId textureviews[64]{ 0 };
		CGPUSamplerId samplers[64]{ 0 };
		CGPUBufferId buffers[64]{ 0 };
		uint64_t buffer_offset_sizes[128]{ 0 };
		CGPUBufferId last_vertex_buffer;
		CGPUBufferId last_index_buffer;
		uint32_t last_vertex_buffer_stride;
		uint32_t last_index_buffer_stride;
		PulseMaterialData* last_material;
		PulseShaderData* last_shader;
		uint64_t last_set_layout_hashes[4]{ 0 };
		std::pmr::vector<ShaderTextureBinder> global_texture_table;
		std::pmr::vector<ShaderSamplerBinder> global_sampler_table;
		std::pmr::vector<ShaderBufferBinder> global_buffer_table;
	};

	struct UploadEncoder
	{
		uint64_t size;
		void* address;
	};
}