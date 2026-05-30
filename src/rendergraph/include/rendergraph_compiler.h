#pragma once

#include "rendergraph_compiler_internal.h"

namespace HGEGraphics
{
	class TextureWrap;
	class BufferWrap;

	struct CompiledResourceNode
	{
		CompiledResourceNode(const char* name, ManageType type, uint16_t width, uint16_t height, uint16_t depth, ECGPUTextureFormat format, pulse_texture_data_t* texture, uint8_t mipCount, uint8_t arraySize, pulse_index_t parent, uint8_t mipLevel, uint8_t arraySlice);
		CompiledResourceNode(const char* name, ManageType type, uint32_t size, pulse_buffer_data_t* imported_buffer, ECGPUResourceTypeFlags bufferType, ECGPUMemoryUsage memoryUsage);
		CompiledResourceNode();

		const char* name;
		const ResourceType resourceType;
		const ManageType manageType;
		TextureWrap* managered_texture;
		pulse_texture_data_t* imported_texture;
		const uint16_t width;
		const uint16_t height;
		const uint16_t depth;
		const ECGPUTextureFormat format;
		pulse_buffer_data_t* imported_buffer;
		BufferWrap* managed_buffer;
		const uint32_t size;
		const ECGPUResourceTypeFlags bufferType;
		const ECGPUMemoryUsage memoryUsage;
		uint8_t mipCount;;
		uint8_t arraySize;
		pulse_index_t parent;
		uint8_t mipLevel;
		uint8_t arraySlice;
	};

	struct CompiledEdge
	{
		pulse_index_t index;
		ECGPUResourceStateFlags usage;
	};

	struct CompiledRenderPassNode
	{
		CompiledRenderPassNode(const char* name, std::pmr::memory_resource* const memory_resource);
		CompiledRenderPassNode();

		const char* name{ nullptr };
		pass_type type;
		std::pmr::vector<CompiledEdge> writes;
		std::pmr::vector<CompiledEdge> reads;
		std::pmr::vector<pulse_index_t> devirtualize;
		std::pmr::vector<pulse_index_t> destroy;
		void* passdata;
		int colorAttachmentCount{ 0 };
		std::array<ColorAttachmentInfo, 8> colorAttachments;
		DepthAttachmentInfo depthAttachment;
		pulse_renderpass_executable_t executable;
		uint16_t staging_buffer;
		uint16_t dest_texture;
		uint16_t dest_buffer;
		pulse_uploadpass_executable_t uploadTextureExecutable;
		uint64_t size, offset;
		void* data;
		uint8_t mipmap;
		uint8_t slice;
	};

	struct CompiledRenderGraph
	{
		CompiledRenderGraph(std::pmr::memory_resource* const memory_resource);
		std::pmr::vector<CompiledResourceNode> resources;
		std::pmr::vector<CompiledRenderPassNode> passes;
	};

	struct Compiler
	{
		static CompiledRenderGraph Compile(pulse_rendergraph_t* renderGraph, std::pmr::memory_resource* const memory_resource);
	};

}
