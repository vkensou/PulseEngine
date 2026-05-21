#pragma once

#include "rendergraph.h"
#include "renderer.h"
#include <vector>
#include <array>
#include <memory_resource>

namespace HGEGraphics
{
	typedef pulse_index_t index_type_t;
	const pulse_index_t MAX_INDEX = PULSE_MAX_INDEX;

	enum class ResourceType { Texture, Buffer };
	enum class ManageType { Managed, Imported, SubResource };
	enum class DepthBits : uint8_t { D32 = 32, D24 = 24, D16 = 16 };

	enum pass_type {
		PASS_TYPE_HOLDON,
		PASS_TYPE_RENDER,
		PASS_TYPE_COMPUTE,
		PASS_TYPE_UPLOAD_TEXTURE,
		PASS_TYPE_UPLOAD_BUFFER,
		PASS_TYPE_PRESENT,
	};

	struct alignas(8) ColorAttachmentInfo
	{
		unsigned int clearColor = 0;
		uint32_t resourceIndex = 0;
		ECGPULoadAction load_action = CGPU_LOAD_ACTION_DONT_CARE;
		ECGPUStoreAction store_action = CGPU_STORE_ACTION_DISCARD;
		bool valid = false;
	};

	struct alignas(8) DepthAttachmentInfo
	{
		float clearDepth = 0;
		uint8_t clearStencil = 0;
		uint32_t resourceIndex = 0;
		ECGPULoadAction depth_load_action = CGPU_LOAD_ACTION_DONT_CARE;
		ECGPUStoreAction depth_store_action = CGPU_STORE_ACTION_DISCARD;
		ECGPULoadAction stencil_load_action = CGPU_LOAD_ACTION_DONT_CARE;
		ECGPUStoreAction stencil_store_action = CGPU_STORE_ACTION_DISCARD;
		bool valid = false;
	};

	struct ResourceNode
	{
		ResourceNode();

		const char* name;
		ResourceType resourceType;
		ManageType manageType;
		Texture* texture;
		Buffer* buffer;
		uint16_t width;
		uint16_t height;
		uint16_t depth;
		ECGPUTextureFormat format;
		uint8_t mipCount;
		uint8_t arraySize;
		uint32_t size;
		pulse_index_t parent;
		uint8_t mipLevel;
		uint8_t arraySlice;
		ECGPUResourceTypeFlags bufferType;
		ECGPUMemoryUsage memoryUsage;
		bool holdOnLast;
	};

	struct RenderGraphEdge
	{
		const pulse_index_t from;
		const pulse_index_t to;
		const ECGPUResourceStateFlags usage;
	};

	struct RenderPassNode
	{
		RenderPassNode(const char* name, pass_type type, std::pmr::memory_resource* const momory_resource);

		const char* name{ nullptr };
		std::pmr::vector<uint32_t> writes;
		std::pmr::vector<uint32_t> reads;
		void* passdata;
		pass_type type;

		struct render_context_t
		{
			int colorAttachmentCount{ 0 };
			std::array<ColorAttachmentInfo, 8> colorAttachments;
			DepthAttachmentInfo depthAttachment;
			pulse_renderpass_executable_t executable;
		};

		struct compute_context_t
		{
			pulse_renderpass_executable_t executable;
		};

		struct present_context_t
		{
		};

		struct upload_texture_context_t
		{
			pulse_buffer_handle_t staging_buffer;
			pulse_texture_handle_t dest_texture;
			pulse_uploadpass_executable_t executable;
			uint64_t size;
			uint64_t offset;
			void* data;
			uint8_t mipmap;
			uint8_t slice;
		};

		struct upload_buffer_context_t
		{
			pulse_buffer_handle_t staging_buffer;
			pulse_buffer_handle_t dest_buffer;
			pulse_uploadpass_executable_t executable;
			uint64_t size;
			uint64_t offset;
			void* data;
		};

		union
		{
			render_context_t render_context;
			compute_context_t compute_context;
			present_context_t present_context;
			upload_texture_context_t upload_texture_context;
			upload_buffer_context_t upload_buffer_context;
		};
	};

	struct pulse_rendergraph_impl_t
	{
		using allocator_type = std::pmr::polymorphic_allocator<std::byte>;

		pulse_rendergraph_impl_t(size_t estimate_resource_count, size_t estimate_pass_count, size_t estimate_edge_count, Shader* blitShader, CGPUSamplerId blitSampler, std::pmr::memory_resource* const resource);

		std::pmr::vector<ResourceNode> resources;
		std::pmr::vector<RenderPassNode> passes;
		std::pmr::vector<RenderGraphEdge> edges;
		allocator_type allocator;
		Shader* blitShader;
		CGPUSamplerId blitSampler;
		std::pmr::vector<Texture*> imported_textures;
		std::pmr::vector<Buffer*> imported_buffers;
	};

	inline pulse_rendergraph_impl_t* to_impl(pulse_rendergraph_t* h) { return (pulse_rendergraph_impl_t*)h; }
	inline pulse_rendergraph_t* from_impl(pulse_rendergraph_impl_t* impl) { return (pulse_rendergraph_t*)impl; }

}
