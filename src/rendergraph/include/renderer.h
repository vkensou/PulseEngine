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

namespace HGEGraphics
{
	struct rendergraph_t;

	struct Shader
	{
		~Shader();

		CGPURootSignatureId root_sig;
		CGPUShaderEntryDescriptor vs;
		CGPUShaderEntryDescriptor ps;
		CGPUBlendStateDescriptor blend_desc;
		std::vector<CGPUBlendAttachmentState> blend_attachment_states;
		CGPUDepthStateDescriptor depth_desc;
		CGPURasterizerStateDescriptor rasterizer_state;
	};

	std::unique_ptr<Shader> create_shader(CGPUDeviceId device, const uint8_t* vert_data, uint32_t vert_length, const uint8_t* frag_data, uint32_t frag_length, const CGPUBlendStateDescriptor& blend_desc, const CGPUDepthStateDescriptor& depth_desc, const CGPURasterizerStateDescriptor& rasterizer_state);

	struct ComputeShader
	{
		~ComputeShader();

		CGPURootSignatureId root_sig;
		CGPUShaderEntryDescriptor cs;
	};

	std::unique_ptr<ComputeShader> create_compute_shader(CGPUDeviceId device, const uint8_t* comp_data, uint32_t comp_length);

	struct Buffer
	{
		~Buffer();

		CGPUBufferId handle;
		ECGPUResourceTypeFlags type;
		ECGPUResourceStateFlags cur_state;
		buffer_handle_t dynamic_handle;
	};

	std::unique_ptr<Buffer> create_buffer(CGPUDeviceId device, const CGPUBufferDescriptor& desc);

	struct Mesh
	{
		CGPUVertexLayout vertex_layout;
		std::vector<CGPUVertexAttribute> vertex_attributes;
		ECGPUPrimitiveTopology prim_topology;
		uint32_t vertex_stride;
		uint32_t index_stride;
		uint32_t vertices_count;
		uint32_t index_count;
		std::unique_ptr<Buffer> vertex_buffer;
		std::unique_ptr<Buffer> index_buffer;
		bool prepared;
	};

	std::unique_ptr<Mesh> create_empty_mesh();
	void init_mesh(Mesh* mesh, CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader);
	std::unique_ptr<Mesh> create_mesh(CGPUDeviceId device, uint32_t vertex_count, uint32_t index_count, ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride, bool update_vertex_data_from_compute_shader, bool update_index_data_from_compute_shader);
	std::unique_ptr<Mesh> create_dynamic_mesh(ECGPUPrimitiveTopology prim_topology, const CGPUVertexLayout& vertex_layout, uint32_t index_stride);
	buffer_handle_t declare_dynamic_vertex_buffer(Mesh* mesh, rendergraph_t* rg, uint32_t count);
	buffer_handle_t declare_dynamic_index_buffer(Mesh* mesh, rendergraph_t* rg, uint32_t count);
	void dynamic_mesh_reset(Mesh* mesh);

	struct Texture
	{
		~Texture();

		CGPUTextureId handle;
		CGPUTextureViewId view;
		std::vector<ECGPUResourceStateFlags> cur_states;
		bool states_consistent;
		bool prepared;
		texture_handle_t dynamic_handle;
	};

	std::unique_ptr<Texture> create_empty_texture();
	void init_texture(Texture* texture, CGPUDeviceId device, const CGPUTextureDescriptor& desc);
	std::unique_ptr<Texture> create_texture(CGPUDeviceId device, const CGPUTextureDescriptor& desc);

	class Material
	{
	public:
		struct BindBuffer
		{
			int set;
			int bind;
			Buffer* buffer;
		};

		struct BindTexture
		{
			int set;
			int bind;
			Texture* texture;
		};

		struct BindSampler
		{
			int set;
			int bind;
			CGPUSamplerId sampler;
		};

		CGPUDeviceId device;
		Shader* shader;
		std::vector<BindBuffer> buffers;
		std::vector<BindTexture> textures;
		std::vector<BindSampler> samplers;
		std::vector<std::unique_ptr<Buffer>> ownedBuffers;

	public:
		Material(CGPUDeviceId device, Shader* shader);
		~Material();

		void bindTexture(int set, int bind, Texture* texture);
		void bindSampler(int set, int bind, CGPUSamplerId sampler);

		template<typename T>
		void bindBuffer(int set, int bind, const T& data)
		{
			bindBuffer(set, bind, sizeof(T), &data);
		}

		void bindBuffer(int set, int bind, size_t size, const void* data);
	};

	struct Backbuffer
	{
		~Backbuffer();

		Texture texture;
	};

	void init_backbuffer(Backbuffer* backbuffer, CGPUSwapChainId swapchain, int index);

	struct RenderPassEncoder;

	struct ShaderTextureBinder
	{
		Texture* texture;
		texture_handle_t texture_handle;
		int set, bind;
	};

	struct ShaderSamplerBinder
	{
		CGPUSamplerId sampler;
		int set, bind;
	};

	struct ShaderBufferBinder
	{
		Buffer* buffer;
		buffer_handle_t buffer_handle;
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
		std::pmr::vector<ShaderTextureBinder> global_texture_table;
		std::pmr::vector<ShaderSamplerBinder> global_sampler_table;
		std::pmr::vector<ShaderBufferBinder> global_buffer_table;
		DescriptorSetPool descriptorSetPool;
		std::pmr::vector<DescriptorSet*> allocated_dsets;
		CGPUDeviceId device = { CGPU_NULLPTR };
		uint64_t timestamp = { 0 };
		Profiler* profiler = nullptr;
		double gpuTicksPerSecond = 0;
		CGPUTextureViewId default_texture = CGPU_NULLPTR;
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
		uint64_t last_buffer_offset_sizes[4][128]{ 0 };
		CGPUTextureViewId textureviews[64]{ 0 };
		CGPUSamplerId samplers[64]{ 0 };
		CGPUBufferId buffers[64]{ 0 };
		uint64_t buffer_offset_sizes[128]{ 0 };
		CGPUBufferId last_vertex_buffer;
		CGPUBufferId last_index_buffer;
		uint32_t last_vertex_buffer_stride;
		uint32_t last_index_buffer_stride;
	};

	struct UploadEncoder
	{
		uint64_t size;
		void* address;
	};
}