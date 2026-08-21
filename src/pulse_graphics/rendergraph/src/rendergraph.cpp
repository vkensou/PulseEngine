#include "rendergraph_compiler_internal.h"

#include <cassert>
#include "drawer.h"
#include "pulse_renderer_asset.h"

using namespace HGEGraphics;

// Helper functions
static PulseRGTextureHandle make_texture_handle(uint32_t index)
{
	PulseRGTextureHandle handle;
	handle.index = index;
	return handle;
}

static PulseRGBufferHandle make_buffer_handle(uint32_t index)
{
	PulseRGBufferHandle handle;
	handle.index = index;
	return handle;
}

static bool is_valid_dynamic_texture_handle(std::pmr::vector<ResourceNode>& resources, PulseRGTextureHandle handle)
{
	return pulse_rgtexture_handle_is_valid(handle) && handle.index < resources.size();
}

static bool is_valid_dynamic_buffer_handle(std::pmr::vector<ResourceNode>& resources, PulseRGBufferHandle handle)
{
	return pulse_rgbuffer_handle_is_valid(handle) && handle.index < resources.size();
}

static uint32_t get_texture_handle_index(PulseRGTextureHandle handle)
{
	return handle.index;
}

static uint32_t get_buffer_handle_index(PulseRGBufferHandle handle)
{
	return handle.index;
}

static void allocate_passdata(pulse_rendergraph_impl_t* self, RenderPassNode* passNode, size_t passdata_size, void** passdata)
{
	if (passdata_size > 0)
	{
		auto pd = self->allocator.allocate_bytes(passdata_size);
		passNode->passdata = *passdata = pd;
	}
	else
		passNode->passdata = nullptr;
}

// pulse_rendergraph_impl_t constructor
pulse_rendergraph_impl_t::pulse_rendergraph_impl_t(PulseAssetSystemId asset_system, size_t estimate_resource_count, size_t estimate_pass_count, size_t estimate_edge_count, PulseShaderData* blitShader, CGPUSamplerId blitSampler, std::pmr::memory_resource* const resource)
	: allocator(resource), asset_system(asset_system), resources(resource), passes(resource), edges(resource), blitShader(blitShader), blitSampler(blitSampler), imported_textures(resource), imported_buffers(resource)
{
	resources.reserve(estimate_resource_count);
	resources.push_back({});
	passes.reserve(estimate_pass_count);
	edges.reserve(estimate_edge_count);
}

// ResourceNode constructor
ResourceNode::ResourceNode()
	: name(nullptr), resourceType(ResourceType::Texture), manageType(ManageType::Managed), width(0), height(0), depth(0), format(ECGPUTextureFormat::CGPU_TEXTURE_FORMAT_UNDEFINED), texture(nullptr), buffer(nullptr), holdOnLast(false), bufferType(CGPU_RESOURCE_TYPE_NONE), memoryUsage(CGPU_MEMORY_USAGE_UNKNOWN), size(0), mipCount(0), arraySize(0), parent(0), mipLevel(0), arraySlice(0)
{
}

// RenderPassNode constructor
RenderPassNode::RenderPassNode(const char* name, pass_type type, std::pmr::memory_resource* const momory_resource)
	: name(name), writes(momory_resource), reads(momory_resource), type(type), passdata(nullptr), upload_buffer_context({ 0 })
{
	if (type == PASS_TYPE_RENDER)
	{
		render_context = {};
	}
}

// C API implementations
extern "C" {

PulseRenderGraphId pulse_create_render_graph(PulseAssetSystemId asset_system, uint32_t estimate_resource_count, uint32_t estimate_pass_count, uint32_t estimate_edge_count, void* blit_shader, CGPUSamplerId blit_sampler)
{
	auto* impl = new pulse_rendergraph_impl_t(
		asset_system,
		estimate_resource_count, estimate_pass_count, estimate_edge_count,
		(PulseShaderData*)blit_shader, blit_sampler,
		std::pmr::new_delete_resource());
	return from_impl(impl);
}

void pulse_destroy_render_graph(PulseRenderGraphId self)
{
	delete to_impl(self);
}

void pulse_render_graph_reset(PulseRenderGraphId self)
{
	auto* impl = to_impl(self);
	impl->resources.clear();
	impl->passes.clear();
	impl->edges.clear();
}

bool pulse_rgtexture_handle_is_valid(PulseRGTextureHandle handle)
{
	return handle.index != 0;
}

bool pulse_rgbuffer_handle_is_valid(PulseRGBufferHandle handle)
{
	return handle.index != 0;
}

PulseRGTextureHandle pulse_render_graph_declare_texture(PulseRenderGraphId self)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resourceNode = impl->resources.back();
	resourceNode.width = 0;
	resourceNode.height = 0;
	resourceNode.depth = 0;
	resourceNode.mipCount = 1;
	resourceNode.arraySize = 1;
	resourceNode.mipLevel = 0;
	resourceNode.arraySlice = 0;
	return make_texture_handle(impl->resources.size() - 1);
}

PulseRGTextureHandle pulse_render_graph_import_texture_impl(PulseRenderGraphId self, PulseTextureData* imported)
{
	auto* impl = to_impl(self);
	if (is_valid_dynamic_texture_handle(impl->resources, imported->dynamic_handle))
		return imported->dynamic_handle;

	assert(impl->resources.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resourceNode = impl->resources.back();
	resourceNode.texture = imported;
	resourceNode.manageType = ManageType::Imported;
	resourceNode.width = imported->handle->info->width;
	resourceNode.height = imported->handle->info->height;
	resourceNode.depth = imported->handle->info->depth;
	resourceNode.format = imported->handle->info->format;
	resourceNode.mipCount = imported->handle->info->mip_levels;
	resourceNode.arraySize = imported->handle->info->array_size_minus_one + 1;
	resourceNode.mipLevel = 0;
	resourceNode.arraySlice = 0;
	auto handle = imported->dynamic_handle = make_texture_handle(impl->resources.size() - 1);
	impl->imported_textures.push_back(imported);
	return handle;
}

PulseRGTextureHandle pulse_render_graph_import_texture(PulseRenderGraphId self, PulseTextureHandle imported)
{
	auto* impl = to_impl(self);
	PulseTextureData* data = nullptr;
	if (impl->asset_system)
	{
		void* ptr = nullptr;
		if (pulse_asset_system_borrow(impl->asset_system, pulse_texture_to_handle(imported), &ptr, nullptr))
			data = static_cast<PulseTextureData*>(ptr);
	}
	if (!data)
		return {};
	return pulse_render_graph_import_texture_impl(self, data);
}

PulseRGTextureHandle pulse_render_graph_import_backbuffer(PulseRenderGraphId self, pulse_backbuffer_data_t* imported)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resourceNode = impl->resources.back();
	auto texture = &imported->texture;
	texture->p_cur_states[0] = CGPU_RESOURCE_STATE_UNDEFINED;
	texture->states_consistent = true;
	return pulse_render_graph_import_texture_impl(self, texture);
}

PulseRGBufferHandle pulse_render_graph_declare_buffer(PulseRenderGraphId self)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resource = impl->resources.back();
	resource.resourceType = ResourceType::Buffer;
	resource.width = 0;
	resource.memoryUsage = CGPU_MEMORY_USAGE_UNKNOWN;
	return make_buffer_handle(impl->resources.size() - 1);
}

PulseRGBufferHandle pulse_render_graph_import_buffer_impl(PulseRenderGraphId self, PulseGraphicsBufferData* imported)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resourceNode = impl->resources.back();
	resourceNode.resourceType = ResourceType::Buffer;
	resourceNode.width = 0;
	resourceNode.memoryUsage = CGPU_MEMORY_USAGE_UNKNOWN;
	resourceNode.buffer = imported;
	resourceNode.manageType = ManageType::Imported;
	resourceNode.size = imported->handle->info->size;
	resourceNode.bufferType = imported->type;
	resourceNode.memoryUsage = (ECGPUMemoryUsage)imported->handle->info->memory_usage;
	return make_buffer_handle(impl->resources.size() - 1);
}

PulseRGBufferHandle pulse_render_graph_import_buffer(PulseRenderGraphId self, PulseGraphicsBufferHandle imported)
{
	auto* impl = to_impl(self);
	PulseGraphicsBufferData* data = nullptr;
	if (impl->asset_system)
	{
		void* ptr = nullptr;
		if (pulse_asset_system_borrow(impl->asset_system, pulse_graphics_buffer_to_handle(imported), &ptr, nullptr))
			data = static_cast<PulseGraphicsBufferData*>(ptr);
	}
	if (!data)
		return {};
	return pulse_render_graph_import_buffer_impl(self, data);
}

PulseRGBufferHandle pulse_render_graph_import_dynamic_buffer(PulseRenderGraphId self, void* imported)
{
	auto* impl = to_impl(self);
	auto* buf = (PulseGraphicsBufferData*)imported;
	if (is_valid_dynamic_buffer_handle(impl->resources, buf->dynamic_handle))
		return buf->dynamic_handle;

	assert(impl->resources.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resource = impl->resources.back();
	resource.resourceType = ResourceType::Buffer;
	resource.width = 0;
	resource.memoryUsage = CGPU_MEMORY_USAGE_UNKNOWN;
	auto handle = buf->dynamic_handle = make_buffer_handle(impl->resources.size() - 1);
	impl->imported_buffers.push_back(buf);
	return handle;
}

PulseRGBufferHandle pulse_render_graph_import_dynamic_mesh_vertex_buffer(PulseRenderGraphId self, PulseMeshHandle imported, uint32_t count)
{
	auto* impl = to_impl(self);
    PulseMeshData* data = nullptr;
	if (impl->asset_system)
    {
		void* ptr = nullptr;
		if (pulse_asset_system_borrow(impl->asset_system, pulse_mesh_to_handle(imported), &ptr, nullptr))
			data = static_cast<PulseMeshData*>(ptr);
    }
    if (!data)
        return {};

    return declare_dynamic_vertex_buffer(data, self, count);
}

PulseRGBufferHandle pulse_render_graph_import_dynamic_mesh_index_buffer(PulseRenderGraphId self, PulseMeshHandle imported, uint32_t count)
{
	auto* impl = to_impl(self);
    PulseMeshData* data = nullptr;
	if (impl->asset_system)
    {
		void* ptr = nullptr;
		if (pulse_asset_system_borrow(impl->asset_system, pulse_mesh_to_handle(imported), &ptr, nullptr))
			data = static_cast<PulseMeshData*>(ptr);
    }
    if (!data)
        return {};

    return declare_dynamic_index_buffer(data, self, count);
}

PulseRGBufferHandle pulse_render_graph_declare_uniform_buffer_quick(PulseRenderGraphId self, uint32_t size, void* data)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	auto nextPowerOfTwo = [](uint32_t n) -> uint32_t
		{
			if (n == 0)
			{
				return 1;
			}

			n--;

			n |= n >> 1;
			n |= n >> 2;
			n |= n >> 4;
			n |= n >> 8;
			n |= n >> 16;

			return n + 1;
		};

	impl->resources.push_back(ResourceNode());
	auto& resource = impl->resources.back();
	resource.resourceType = ResourceType::Buffer;
	resource.memoryUsage = CGPU_MEMORY_USAGE_UNKNOWN;
	resource.size = nextPowerOfTwo(size);
	resource.bufferType = CGPU_RESOURCE_TYPE_UNIFORM_BUFFER;
	resource.memoryUsage = ECGPUMemoryUsage::CGPU_MEMORY_USAGE_GPU_ONLY;
	PulseRGBufferHandle ubo_handle = make_buffer_handle(impl->resources.size() - 1);
	pulse_render_graph_add_uploadbufferpass_ex(self, "quick upload ubo", ubo_handle, size, 0, data, nullptr, 0, nullptr);
	return ubo_handle;
}

PulseRGTextureHandle pulse_render_graph_declare_texture_subresource(PulseRenderGraphId self, PulseRGTextureHandle parent_handle, uint8_t mipmap, uint8_t slice)
{
	auto* impl = to_impl(self);
	assert(pulse_rgtexture_handle_is_valid(parent_handle));
	uint32_t parent = get_texture_handle_index(parent_handle);
	ResourceNode* textureNode = &impl->resources[parent];
	assert(textureNode->resourceType == ResourceType::Texture);

	while (textureNode->parent != 0)
	{
		parent = textureNode->parent;
		textureNode = &impl->resources[parent];
		assert(textureNode->resourceType == ResourceType::Texture);
	}

	impl->resources.push_back(ResourceNode());
	auto& resourceNode = impl->resources.back();
	resourceNode.resourceType = ResourceType::Texture;
	resourceNode.manageType = ManageType::SubResource;
	resourceNode.width = textureNode->width;
	resourceNode.height = textureNode->height;
	resourceNode.depth = textureNode->depth;
	resourceNode.format = textureNode->format;
	resourceNode.mipCount = textureNode->mipCount;
	resourceNode.arraySize = textureNode->arraySize;
	resourceNode.parent = parent;
	resourceNode.mipLevel = mipmap;
	resourceNode.arraySlice = slice;
	PulseRGTextureHandle h;
	h.index = (uint32_t)(impl->resources.size() - 1);
	return h;
}

void pulse_render_graph_texture_set_extent(PulseRenderGraphId self, PulseRGTextureHandle texture, uint32_t width, uint32_t height, uint32_t depth)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	resourceNode.width = width;
	resourceNode.height = height;
	resourceNode.depth = depth;
}

void pulse_render_graph_texture_set_format(PulseRenderGraphId self, PulseRGTextureHandle texture, ECGPUTextureFormat format)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	resourceNode.format = format;
}

void pulse_render_graph_texture_set_depth_format(PulseRenderGraphId self, PulseRGTextureHandle texture, EPulseDepthBits depthBits, bool needStencil)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto FormatUtil_GetDepthStencilFormat = [](EPulseDepthBits depthBits, bool needStencil) -> ECGPUTextureFormat
	{
		if (depthBits == PULSE_DEPTH_BITS_D32 && needStencil)
			return CGPU_TEXTURE_FORMAT_D32_SFLOAT_S8_UINT;
		else if (depthBits == PULSE_DEPTH_BITS_D32 && !needStencil)
			return CGPU_TEXTURE_FORMAT_D32_SFLOAT;
		else if (depthBits == PULSE_DEPTH_BITS_D24 && needStencil)
			return CGPU_TEXTURE_FORMAT_D24_UNORM_S8_UINT;
		else if (depthBits == PULSE_DEPTH_BITS_D24 && !needStencil)
			return CGPU_TEXTURE_FORMAT_X8_D24_UNORM_PACK32;
		else if (depthBits == PULSE_DEPTH_BITS_D16 && needStencil)
			return CGPU_TEXTURE_FORMAT_D16_UNORM_S8_UINT;
		else if (depthBits == PULSE_DEPTH_BITS_D16 && !needStencil)
			return CGPU_TEXTURE_FORMAT_D16_UNORM;
		else
			return CGPU_TEXTURE_FORMAT_UNDEFINED;
	};

	auto format = FormatUtil_GetDepthStencilFormat(depthBits, needStencil);
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	if (format != CGPU_TEXTURE_FORMAT_UNDEFINED)
		resourceNode.format = format;
	else
		resourceNode.format = CGPU_TEXTURE_FORMAT_UNDEFINED;
}

uint32_t pulse_render_graph_texture_get_width(PulseRenderGraphId self, PulseRGTextureHandle texture)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	return resourceNode.width;
}

uint32_t pulse_render_graph_texture_get_height(PulseRenderGraphId self, PulseRGTextureHandle texture)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	return resourceNode.height;
}

uint32_t pulse_render_graph_texture_get_depth(PulseRenderGraphId self, PulseRGTextureHandle texture)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	return resourceNode.depth;
}

ECGPUTextureFormat pulse_render_graph_texture_get_format(PulseRenderGraphId self, PulseRGTextureHandle texture)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	return resourceNode.format;
}

void pulse_render_graph_buffer_set_size(PulseRenderGraphId self, PulseRGBufferHandle buffer, uint32_t size)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_buffer_handle(impl->resources, buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);

	auto nextPowerOfTwo = [](uint32_t n) -> uint32_t
		{
			if (n == 0)
			{
				return 1;
			}

			n--;

			n |= n >> 1;
			n |= n >> 2;
			n |= n >> 4;
			n |= n >> 8;
			n |= n >> 16;

			return n + 1;
		};

	resourceNode.size = nextPowerOfTwo(size);
}

void pulse_render_graph_buffer_set_type(PulseRenderGraphId self, PulseRGBufferHandle buffer, ECGPUResourceTypeFlags type)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_buffer_handle(impl->resources, buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	resourceNode.bufferType = type;
}

void pulse_render_graph_buffer_set_usage(PulseRenderGraphId self, PulseRGBufferHandle buffer, ECGPUMemoryUsage usage)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_buffer_handle(impl->resources, buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	resourceNode.memoryUsage = usage;
}

void pulse_render_graph_buffer_set_hold_on_last(PulseRenderGraphId self, PulseRGBufferHandle buffer)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_buffer_handle(impl->resources, buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	resourceNode.holdOnLast = true;
}

PulseRenderPassBuilder pulse_render_graph_add_render_pass(PulseRenderGraphId self, const char* name)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->passes.emplace_back(name, PASS_TYPE_RENDER, impl->allocator.resource());
	PulseRenderPassBuilder builder;
	builder.render_graph = self;
	builder.pass_node = &(impl->passes.back());
	builder.pass_index = (uint32_t)(impl->passes.size() - 1);
	return builder;
}

PulseComputePassBuilder pulse_render_graph_add_compute_pass(PulseRenderGraphId self, const char* name)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->passes.emplace_back(name, PASS_TYPE_COMPUTE, impl->allocator.resource());
	PulseComputePassBuilder builder;
	builder.render_graph = self;
	builder.pass_node = &(impl->passes.back());
	builder.pass_index = (uint32_t)(impl->passes.size() - 1);
	return builder;
}

PulseRenderPassBuilder pulse_render_graph_add_holdpass(PulseRenderGraphId self, const char* name)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	impl->passes.emplace_back(name, PASS_TYPE_HOLDON, impl->allocator.resource());
	PulseRenderPassBuilder builder;
	builder.render_graph = self;
	builder.pass_node = &(impl->passes.back());
	builder.pass_index = (uint32_t)(impl->passes.size() - 1);
	return builder;
}

void pulse_render_graph_add_uploadtexturepass(PulseRenderGraphId self, const char* name, PulseRGTextureHandle texture, uint8_t mipmap, uint8_t slice, PulseProcUploadpassExecutable executable, uint32_t passdata_size, void** out_passdata)
{
	pulse_render_graph_add_uploadtexturepass_ex(self, name, texture, mipmap, slice, 0, 0, nullptr, executable, passdata_size, out_passdata);
}

void pulse_render_graph_add_uploadtexturepass_ex(PulseRenderGraphId self, const char* name, PulseRGTextureHandle texture, uint8_t mipmap, uint8_t slice, uint64_t size, uint64_t offset, void* data, PulseProcUploadpassExecutable executable, uint32_t passdata_size, void** out_passdata)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	auto& pass = impl->passes.emplace_back(name, PASS_TYPE_UPLOAD_TEXTURE, impl->allocator.resource());
	int passIndex = (int)(impl->passes.size() - 1);

	assert(pulse_rgtexture_handle_is_valid(texture));
	auto& textureNode = impl->resources[get_texture_handle_index(texture)];
	assert(textureNode.resourceType == ResourceType::Texture);

	PulseRGTextureHandle usedTexture;
	ResourceNode* usedTextureNode;

	if (textureNode.mipCount == 1 && textureNode.arraySize == 1)
	{
		usedTexture = texture;
		usedTextureNode = &textureNode;
	}
	else
	{
		usedTexture = pulse_render_graph_declare_texture_subresource(self, texture, mipmap, slice);
		usedTextureNode = &impl->resources[get_texture_handle_index(usedTexture)];
	}

	pass.upload_texture_context.dest_texture = usedTexture;
	auto write_edge = pulse_render_graph_add_edge(self, passIndex, get_texture_handle_index(usedTexture), CGPU_RESOURCE_STATE_COPY_DEST);
	pass.writes.push_back(write_edge);

	auto staging_buffer = pulse_render_graph_declare_buffer(self);
	auto mipedSize = [](uint64_t sz, uint64_t mip) { return std::max<uint64_t>(sz >> mip, 1ull); };
	const uint64_t xBlocksCount = mipedSize(usedTextureNode->width, mipmap) / FormatUtil_WidthOfBlock(usedTextureNode->format);
	const uint64_t yBlocksCount = mipedSize(usedTextureNode->height, mipmap) / FormatUtil_HeightOfBlock(usedTextureNode->format);
	const uint64_t zBlocksCount = mipedSize(usedTextureNode->depth, mipmap);
	const uint64_t bufferSize = xBlocksCount * yBlocksCount * zBlocksCount * FormatUtil_BitSizeOfBlock(usedTextureNode->format) / 8;
	assert(bufferSize >= size + offset);
	pulse_render_graph_buffer_set_size(self, staging_buffer, (uint32_t)bufferSize);
	pulse_render_graph_buffer_set_type(self, staging_buffer, CGPU_RESOURCE_TYPE_NONE);
	pulse_render_graph_buffer_set_usage(self, staging_buffer, CGPU_MEMORY_USAGE_CPU_ONLY);
	pulse_render_graph_buffer_set_hold_on_last(self, staging_buffer);
	pass.upload_texture_context.staging_buffer = staging_buffer;
	auto read_edge = pulse_render_graph_add_edge(self, get_buffer_handle_index(staging_buffer), passIndex, CGPU_RESOURCE_STATE_COPY_SOURCE);
	pass.reads.push_back(read_edge);

	pass.upload_texture_context.executable = executable;
	allocate_passdata(impl, &pass, passdata_size, out_passdata);
	pass.upload_texture_context.size = size;
	pass.upload_texture_context.offset = offset;
	pass.upload_texture_context.data = data;
	pass.upload_texture_context.mipmap = mipmap;
	pass.upload_texture_context.slice = slice;
}

void pulse_render_graph_add_uploadbufferpass(PulseRenderGraphId self, const char* name, PulseRGBufferHandle buffer, PulseProcUploadpassExecutable executable, uint32_t passdata_size, void** out_passdata)
{
	pulse_render_graph_add_uploadbufferpass_ex(self, name, buffer, 0, 0, nullptr, executable, passdata_size, out_passdata);
}

void pulse_render_graph_add_uploadbufferpass_ex(PulseRenderGraphId self, const char* name, PulseRGBufferHandle buffer, uint64_t size, uint64_t offset, void* data, PulseProcUploadpassExecutable executable, uint32_t passdata_size, void** out_passdata)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	auto& pass = impl->passes.emplace_back(name, PASS_TYPE_UPLOAD_BUFFER, impl->allocator.resource());
	int passIndex = (int)(impl->passes.size() - 1);

	assert(pulse_rgbuffer_handle_is_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	pass.upload_buffer_context.dest_buffer = buffer;
	auto write_edge = pulse_render_graph_add_edge(self, passIndex, get_buffer_handle_index(buffer), CGPU_RESOURCE_STATE_COPY_DEST);
	pass.writes.push_back(write_edge);

	auto staging_buffer = pulse_render_graph_declare_buffer(self);
	assert(resourceNode.size >= size + offset);
	pulse_render_graph_buffer_set_size(self, staging_buffer, resourceNode.size);
	pulse_render_graph_buffer_set_type(self, staging_buffer, CGPU_RESOURCE_TYPE_NONE);
	pulse_render_graph_buffer_set_usage(self, staging_buffer, CGPU_MEMORY_USAGE_CPU_ONLY);
	pulse_render_graph_buffer_set_hold_on_last(self, staging_buffer);
	pass.upload_buffer_context.staging_buffer = staging_buffer;
	auto read_edge = pulse_render_graph_add_edge(self, get_buffer_handle_index(staging_buffer), passIndex, CGPU_RESOURCE_STATE_COPY_SOURCE);
	pass.reads.push_back(read_edge);

	pass.upload_buffer_context.executable = (decltype(pass.upload_buffer_context.executable))executable;
	allocate_passdata(impl, &pass, passdata_size, out_passdata);
	pass.upload_buffer_context.size = size;
	pass.upload_buffer_context.offset = offset;
	pass.upload_buffer_context.data = data;
}

void pulse_render_graph_add_generate_mipmap(PulseRenderGraphId self, PulseRGTextureHandle texture, uint8_t from_mipmap)
{
	auto* impl = to_impl(self);
	assert(pulse_rgtexture_handle_is_valid(texture));
	auto& textureNode = impl->resources[get_texture_handle_index(texture)];
	assert(textureNode.resourceType == ResourceType::Texture && textureNode.manageType != ManageType::SubResource);
	assert(textureNode.arraySize == 1);
	if (textureNode.mipCount == 1)
		return;

	auto mip0 = pulse_render_graph_declare_texture_subresource(self, texture, 0, 0);
	auto last = mip0;
	for (size_t i = from_mipmap; i < textureNode.mipCount; ++i)
	{
		auto mipi = pulse_render_graph_declare_texture_subresource(self, texture, (uint8_t)i, 0);

		auto passBuilder = pulse_render_graph_add_render_pass(self, "generate mip");
		pulse_render_pass_builder_add_color_attachment(&passBuilder, mipi, CGPU_LOAD_ACTION_DONT_CARE, 0, CGPU_STORE_ACTION_STORE);
		pulse_render_pass_builder_sample(&passBuilder, last);

		struct BlitMipmapPassData
		{
			PulseShaderData* blitShader;
			CGPUSamplerId blitSampler;
			PulseRGTextureHandle source;
		};
		BlitMipmapPassData* passdata = nullptr;
		pulse_render_pass_builder_set_executable(&passBuilder, [](PulseRenderPassEncoder* encoder, void* userdata)
			{
				BlitMipmapPassData* resolved_passdata = (BlitMipmapPassData*)userdata;
				auto* rgEncoder = (RenderPassEncoder*)encoder;
				set_global_texture_handle(rgEncoder, resolved_passdata->source, 0, 0);
				set_global_sampler(rgEncoder, resolved_passdata->blitSampler, 0, 1);
				draw_procedure(rgEncoder, resolved_passdata->blitShader, CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
			}, sizeof(BlitMipmapPassData), (void**)&passdata);
		passdata->blitShader = impl->blitShader;
		passdata->blitSampler = impl->blitSampler;
		passdata->source = last;
		last = mipi;
	}
}

void pulse_render_graph_present(PulseRenderGraphId self, PulseRGTextureHandle texture)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_RENDER_GRAPH_MAX_INDEX);
	auto& passNode = impl->passes.emplace_back("Present", PASS_TYPE_PRESENT, impl->allocator.resource());
	int passIndex = (int)(impl->passes.size() - 1);
	auto edge = pulse_render_graph_add_edge(self, get_texture_handle_index(texture), passIndex, CGPU_RESOURCE_STATE_PRESENT);
	passNode.reads.push_back(edge);
}

uint32_t pulse_render_graph_add_edge(PulseRenderGraphId self, uint32_t from, uint32_t to, ECGPUResourceStateFlags usage)
{
	auto* impl = to_impl(self);
	impl->edges.emplace_back(from, to, usage);
	return (uint32_t)(impl->edges.size() - 1);
}

// Builder operations
void pulse_render_pass_builder_add_color_attachment(PulseRenderPassBuilder* self, PulseRGTextureHandle texture, ECGPULoadAction load_action, uint32_t clear_color, ECGPUStoreAction store_action)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(passNode->type == PASS_TYPE_RENDER);
	assert(passNode->render_context.colorAttachmentCount <= (int)passNode->render_context.colorAttachments.size());

	auto edge = pulse_render_graph_add_edge(self->render_graph, self->pass_index, get_texture_handle_index(texture), CGPU_RESOURCE_STATE_RENDER_TARGET);
	passNode->writes.push_back(edge);
	passNode->render_context.colorAttachments[passNode->render_context.colorAttachmentCount++] =
	{
		.clearColor = clear_color,
		.resourceIndex = get_texture_handle_index(texture),
		.load_action = load_action,
		.store_action = store_action,
		.valid = true,
	};
}

void pulse_render_pass_builder_add_depth_attachment(PulseRenderPassBuilder* self, PulseRGTextureHandle texture, ECGPULoadAction depth_load_action, float clear_depth, ECGPUStoreAction depth_store_action, ECGPULoadAction stencil_load_action, uint8_t clear_stencil, ECGPUStoreAction stencil_store_action)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(passNode->type == PASS_TYPE_RENDER);
	assert(!passNode->render_context.depthAttachment.valid);

	auto edge1 = pulse_render_graph_add_edge(self->render_graph, get_texture_handle_index(texture), self->pass_index, CGPU_RESOURCE_STATE_UNDEFINED);
	passNode->reads.push_back(edge1);
	auto edge2 = pulse_render_graph_add_edge(self->render_graph, self->pass_index, get_texture_handle_index(texture), CGPU_RESOURCE_STATE_DEPTH_WRITE);
	passNode->writes.push_back(edge2);
	passNode->render_context.depthAttachment =
	{
		.clearDepth = clear_depth,
		.clearStencil = clear_stencil,
		.resourceIndex = get_texture_handle_index(texture),
		.depth_load_action = depth_load_action,
		.depth_store_action = depth_store_action,
		.stencil_load_action = stencil_load_action,
		.stencil_store_action = stencil_store_action,
		.valid = true,
	};
}

void pulse_render_pass_builder_sample(PulseRenderPassBuilder* self, PulseRGTextureHandle texture)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto edge = pulse_render_graph_add_edge(self->render_graph, get_texture_handle_index(texture), self->pass_index, CGPU_RESOURCE_STATE_SHADER_RESOURCE);
	passNode->reads.push_back(edge);
}

void pulse_render_pass_builder_use_buffer(PulseRenderPassBuilder* self, PulseRGBufferHandle buffer)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rgbuffer_handle_is_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);

	ECGPUResourceStateFlags state = CGPU_RESOURCE_STATE_UNDEFINED;
	if (resourceNode.bufferType & CGPU_RESOURCE_TYPE_VERTEX_BUFFER)
		state = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	else if (resourceNode.bufferType & CGPU_RESOURCE_TYPE_INDEX_BUFFER)
		state = CGPU_RESOURCE_STATE_INDEX_BUFFER;
	else if (resourceNode.bufferType & CGPU_RESOURCE_TYPE_UNIFORM_BUFFER)
		state = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	assert(state != CGPU_RESOURCE_STATE_UNDEFINED);

	auto edge = pulse_render_graph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, state);
	passNode->reads.push_back(edge);
}

void pulse_render_pass_builder_use_buffer_as(PulseRenderPassBuilder* self, PulseRGBufferHandle buffer, ECGPUResourceStateFlags state)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rgbuffer_handle_is_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	assert(state != CGPU_RESOURCE_STATE_UNDEFINED);

	auto edge = pulse_render_graph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, state);
	passNode->reads.push_back(edge);
}

void pulse_render_pass_builder_set_executable(PulseRenderPassBuilder* self, PulseProcRenderPassExecutable executable, uint32_t passdata_size, void** out_passdata)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	passNode->render_context.executable = executable;
	allocate_passdata(impl, passNode, passdata_size, out_passdata);
}

void pulse_computepass_sample(PulseRenderPassBuilder* self, PulseRGTextureHandle texture)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto edge = pulse_render_graph_add_edge(self->render_graph, get_texture_handle_index(texture), self->pass_index, CGPU_RESOURCE_STATE_SHADER_RESOURCE);
	passNode->reads.push_back(edge);
}

void pulse_compute_pass_builder_use_buffer(PulseComputePassBuilder* self, PulseRGBufferHandle buffer)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rgbuffer_handle_is_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);

	ECGPUResourceStateFlags state = CGPU_RESOURCE_STATE_UNDEFINED;
	if (resourceNode.bufferType == CGPU_RESOURCE_TYPE_VERTEX_BUFFER)
		state = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	else if (resourceNode.bufferType == CGPU_RESOURCE_TYPE_INDEX_BUFFER)
		state = CGPU_RESOURCE_STATE_INDEX_BUFFER;
	else if (resourceNode.bufferType == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER)
		state = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	else if (resourceNode.bufferType == CGPU_RESOURCE_TYPE_RW_BUFFER)
		state = CGPU_RESOURCE_STATE_UNORDERED_ACCESS;
	assert(state != CGPU_RESOURCE_STATE_UNDEFINED);

	auto edge = pulse_render_graph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, state);
	passNode->reads.push_back(edge);
}

void pulse_compute_pass_builder_use_buffer_as(PulseComputePassBuilder* self, PulseRGBufferHandle buffer, ECGPUResourceStateFlags state)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rgbuffer_handle_is_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	assert(state != CGPU_RESOURCE_STATE_UNDEFINED);

	auto edge = pulse_render_graph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, state);
	passNode->reads.push_back(edge);
}

void pulse_compute_pass_builder_readwrite_texture(PulseComputePassBuilder* self, PulseRGTextureHandle texture)
{
	// No-op for now (same as original)
}

void pulse_compute_pass_builder_readwrite_buffer(PulseComputePassBuilder* self, PulseRGBufferHandle buffer)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rgbuffer_handle_is_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);

	auto edge = pulse_render_graph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, CGPU_RESOURCE_STATE_UNORDERED_ACCESS);
	passNode->reads.push_back(edge);
	auto edge2 = pulse_render_graph_add_edge(self->render_graph, self->pass_index, get_buffer_handle_index(buffer), CGPU_RESOURCE_STATE_UNORDERED_ACCESS);
	passNode->writes.push_back(edge2);
}

void pulse_compute_pass_builder_set_executable(PulseComputePassBuilder* self, PulseProcRenderPassExecutable executable, uint32_t passdata_size, void** out_passdata)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	passNode->compute_context.executable = executable;
	allocate_passdata(impl, passNode, passdata_size, out_passdata);
}

} // extern "C"
