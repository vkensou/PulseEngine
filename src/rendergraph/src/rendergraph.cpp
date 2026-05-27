#include "rendergraph_compiler_internal.h"

#include <cassert>
#include "drawer.h"
#include "pulse_renderer_asset.h"

using namespace HGEGraphics;

// Helper functions
static pulse_texture_handle_t make_texture_handle(uint32_t index)
{
	pulse_texture_handle_t handle;
	handle.index = index;
	return handle;
}

static pulse_buffer_handle_t make_buffer_handle(uint32_t index)
{
	pulse_buffer_handle_t handle;
	handle.index = index;
	return handle;
}

static bool is_valid_dynamic_texture_handle(std::pmr::vector<ResourceNode>& resources, pulse_texture_handle_t handle)
{
	return pulse_rendergraph_texture_handle_valid(handle) && handle.index < resources.size();
}

static bool is_valid_dynamic_buffer_handle(std::pmr::vector<ResourceNode>& resources, pulse_buffer_handle_t handle)
{
	return pulse_rendergraph_buffer_handle_valid(handle) && handle.index < resources.size();
}

static uint32_t get_texture_handle_index(pulse_texture_handle_t handle)
{
	return handle.index;
}

static uint32_t get_buffer_handle_index(pulse_buffer_handle_t handle)
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
pulse_rendergraph_impl_t::pulse_rendergraph_impl_t(size_t estimate_resource_count, size_t estimate_pass_count, size_t estimate_edge_count, pulse_shader_data_t* blitShader, CGPUSamplerId blitSampler, std::pmr::memory_resource* const resource)
	: allocator(resource), resources(resource), passes(resource), edges(resource), blitShader(blitShader), blitSampler(blitSampler), imported_textures(resource), imported_buffers(resource)
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

pulse_rendergraph_t* pulse_rendergraph_create(uint32_t estimate_resource_count, uint32_t estimate_pass_count, uint32_t estimate_edge_count, void* blit_shader, CGPUSamplerId blit_sampler)
{
	auto* impl = new pulse_rendergraph_impl_t(
		estimate_resource_count, estimate_pass_count, estimate_edge_count,
		(pulse_shader_data_t*)blit_shader, blit_sampler,
		std::pmr::new_delete_resource());
	return from_impl(impl);
}

void pulse_rendergraph_destroy(pulse_rendergraph_t* self)
{
	delete to_impl(self);
}

void pulse_rendergraph_reset(pulse_rendergraph_t* self)
{
	auto* impl = to_impl(self);
	impl->resources.clear();
	impl->passes.clear();
	impl->edges.clear();
}

bool pulse_rendergraph_texture_handle_valid(pulse_texture_handle_t handle)
{
	return handle.index != 0;
}

bool pulse_rendergraph_buffer_handle_valid(pulse_buffer_handle_t handle)
{
	return handle.index != 0;
}

pulse_texture_handle_t pulse_rendergraph_declare_texture(pulse_rendergraph_t* self)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_MAX_INDEX);
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

pulse_texture_handle_t pulse_rendergraph_import_texture(pulse_rendergraph_t* self, pulse_texture_data_t* imported)
{
	auto* impl = to_impl(self);
	if (is_valid_dynamic_texture_handle(impl->resources, imported->dynamic_handle))
		return imported->dynamic_handle;

	assert(impl->resources.size() <= PULSE_MAX_INDEX);
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

pulse_texture_handle_t pulse_rendergraph_import_backbuffer(pulse_rendergraph_t* self, pulse_backbuffer_data_t* imported)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resourceNode = impl->resources.back();
	auto texture = &imported->texture;
	texture->p_cur_states[0] = CGPU_RESOURCE_STATE_UNDEFINED;
	texture->states_consistent = true;
	return pulse_rendergraph_import_texture(self, texture);
}

pulse_buffer_handle_t pulse_rendergraph_declare_buffer(pulse_rendergraph_t* self)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resource = impl->resources.back();
	resource.resourceType = ResourceType::Buffer;
	resource.width = 0;
	resource.memoryUsage = CGPU_MEMORY_USAGE_UNKNOWN;
	return make_buffer_handle(impl->resources.size() - 1);
}

pulse_buffer_handle_t pulse_rendergraph_import_buffer(pulse_rendergraph_t* self, pulse_buffer_data_t* imported)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_MAX_INDEX);
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

pulse_buffer_handle_t pulse_rendergraph_import_dynamic_buffer(pulse_rendergraph_t* self, void* imported)
{
	auto* impl = to_impl(self);
	auto* buf = (pulse_buffer_data_t*)imported;
	if (is_valid_dynamic_buffer_handle(impl->resources, buf->dynamic_handle))
		return buf->dynamic_handle;

	assert(impl->resources.size() <= PULSE_MAX_INDEX);
	impl->resources.push_back(ResourceNode());
	auto& resource = impl->resources.back();
	resource.resourceType = ResourceType::Buffer;
	resource.width = 0;
	resource.memoryUsage = CGPU_MEMORY_USAGE_UNKNOWN;
	auto handle = buf->dynamic_handle = make_buffer_handle(impl->resources.size() - 1);
	impl->imported_buffers.push_back(buf);
	return handle;
}

pulse_buffer_handle_t pulse_rendergraph_declare_uniform_buffer_quick(pulse_rendergraph_t* self, uint32_t size, void* data)
{
	auto* impl = to_impl(self);
	assert(impl->resources.size() <= PULSE_MAX_INDEX);
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
	pulse_buffer_handle_t ubo_handle = make_buffer_handle(impl->resources.size() - 1);
	pulse_rendergraph_add_uploadbufferpass_ex(self, "quick upload ubo", ubo_handle, size, 0, data, nullptr, 0, nullptr);
	return ubo_handle;
}

pulse_texture_handle_t pulse_rendergraph_declare_texture_subresource(pulse_rendergraph_t* self, pulse_texture_handle_t parent_handle, uint8_t mipmap, uint8_t slice)
{
	auto* impl = to_impl(self);
	assert(pulse_rendergraph_texture_handle_valid(parent_handle));
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
	pulse_texture_handle_t h;
	h.index = (pulse_index_t)(impl->resources.size() - 1);
	return h;
}

void pulse_rendergraph_texture_set_extent(pulse_rendergraph_t* self, pulse_texture_handle_t texture, uint32_t width, uint32_t height, uint32_t depth)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	resourceNode.width = width;
	resourceNode.height = height;
	resourceNode.depth = depth;
}

void pulse_rendergraph_texture_set_format(pulse_rendergraph_t* self, pulse_texture_handle_t texture, ECGPUTextureFormat format)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	resourceNode.format = format;
}

void pulse_rendergraph_texture_set_depth_format(pulse_rendergraph_t* self, pulse_texture_handle_t texture, pulse_depth_bits_t depthBits, bool needStencil)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto FormatUtil_GetDepthStencilFormat = [](pulse_depth_bits_t depthBits, bool needStencil) -> ECGPUTextureFormat
	{
		if (depthBits == PULSE_DEPTH_D32 && needStencil)
			return CGPU_TEXTURE_FORMAT_D32_SFLOAT_S8_UINT;
		else if (depthBits == PULSE_DEPTH_D32 && !needStencil)
			return CGPU_TEXTURE_FORMAT_D32_SFLOAT;
		else if (depthBits == PULSE_DEPTH_D24 && needStencil)
			return CGPU_TEXTURE_FORMAT_D24_UNORM_S8_UINT;
		else if (depthBits == PULSE_DEPTH_D24 && !needStencil)
			return CGPU_TEXTURE_FORMAT_X8_D24_UNORM_PACK32;
		else if (depthBits == PULSE_DEPTH_D16 && needStencil)
			return CGPU_TEXTURE_FORMAT_D16_UNORM_S8_UINT;
		else if (depthBits == PULSE_DEPTH_D16 && !needStencil)
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

uint32_t pulse_rendergraph_texture_get_width(pulse_rendergraph_t* self, pulse_texture_handle_t texture)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	return resourceNode.width;
}

uint32_t pulse_rendergraph_texture_get_height(pulse_rendergraph_t* self, pulse_texture_handle_t texture)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	return resourceNode.height;
}

uint32_t pulse_rendergraph_texture_get_depth(pulse_rendergraph_t* self, pulse_texture_handle_t texture)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	return resourceNode.depth;
}

ECGPUTextureFormat pulse_rendergraph_texture_get_format(pulse_rendergraph_t* self, pulse_texture_handle_t texture)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_texture_handle(impl->resources, texture));
	auto& resourceNode = impl->resources[get_texture_handle_index(texture)];
	assert(resourceNode.resourceType == ResourceType::Texture);
	return resourceNode.format;
}

void pulse_rendergraph_buffer_set_size(pulse_rendergraph_t* self, pulse_buffer_handle_t buffer, uint32_t size)
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

void pulse_rendergraph_buffer_set_type(pulse_rendergraph_t* self, pulse_buffer_handle_t buffer, ECGPUResourceTypeFlags type)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_buffer_handle(impl->resources, buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	resourceNode.bufferType = type;
}

void pulse_rendergraph_buffer_set_usage(pulse_rendergraph_t* self, pulse_buffer_handle_t buffer, ECGPUMemoryUsage usage)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_buffer_handle(impl->resources, buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	resourceNode.memoryUsage = usage;
}

void pulse_rendergraph_buffer_set_hold_on_last(pulse_rendergraph_t* self, pulse_buffer_handle_t buffer)
{
	auto* impl = to_impl(self);
	assert(is_valid_dynamic_buffer_handle(impl->resources, buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	resourceNode.holdOnLast = true;
}

pulse_renderpass_builder_t pulse_rendergraph_add_renderpass(pulse_rendergraph_t* self, const char* name)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_MAX_INDEX);
	impl->passes.emplace_back(name, PASS_TYPE_RENDER, impl->allocator.resource());
	pulse_renderpass_builder_t builder;
	builder.render_graph = self;
	builder.pass_node = &(impl->passes.back());
	builder.pass_index = (pulse_index_t)(impl->passes.size() - 1);
	return builder;
}

pulse_renderpass_builder_t pulse_rendergraph_add_computepass(pulse_rendergraph_t* self, const char* name)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_MAX_INDEX);
	impl->passes.emplace_back(name, PASS_TYPE_COMPUTE, impl->allocator.resource());
	pulse_renderpass_builder_t builder;
	builder.render_graph = self;
	builder.pass_node = &(impl->passes.back());
	builder.pass_index = (pulse_index_t)(impl->passes.size() - 1);
	return builder;
}

pulse_renderpass_builder_t pulse_rendergraph_add_holdpass(pulse_rendergraph_t* self, const char* name)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_MAX_INDEX);
	impl->passes.emplace_back(name, PASS_TYPE_HOLDON, impl->allocator.resource());
	pulse_renderpass_builder_t builder;
	builder.render_graph = self;
	builder.pass_node = &(impl->passes.back());
	builder.pass_index = (pulse_index_t)(impl->passes.size() - 1);
	return builder;
}

void pulse_rendergraph_add_uploadtexturepass(pulse_rendergraph_t* self, const char* name, pulse_texture_handle_t texture, uint8_t mipmap, uint8_t slice, pulse_uploadpass_executable_t executable, uint32_t passdata_size, void** out_passdata)
{
	pulse_rendergraph_add_uploadtexturepass_ex(self, name, texture, mipmap, slice, 0, 0, nullptr, executable, passdata_size, out_passdata);
}

void pulse_rendergraph_add_uploadtexturepass_ex(pulse_rendergraph_t* self, const char* name, pulse_texture_handle_t texture, uint8_t mipmap, uint8_t slice, uint64_t size, uint64_t offset, void* data, pulse_uploadpass_executable_t executable, uint32_t passdata_size, void** out_passdata)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_MAX_INDEX);
	auto& pass = impl->passes.emplace_back(name, PASS_TYPE_UPLOAD_TEXTURE, impl->allocator.resource());
	int passIndex = (int)(impl->passes.size() - 1);

	assert(pulse_rendergraph_texture_handle_valid(texture));
	auto& textureNode = impl->resources[get_texture_handle_index(texture)];
	assert(textureNode.resourceType == ResourceType::Texture);

	pulse_texture_handle_t usedTexture;
	ResourceNode* usedTextureNode;

	if (textureNode.mipCount == 1 && textureNode.arraySize == 1)
	{
		usedTexture = texture;
		usedTextureNode = &textureNode;
	}
	else
	{
		usedTexture = pulse_rendergraph_declare_texture_subresource(self, texture, mipmap, slice);
		usedTextureNode = &impl->resources[get_texture_handle_index(usedTexture)];
	}

	pass.upload_texture_context.dest_texture = usedTexture;
	auto write_edge = pulse_rendergraph_add_edge(self, passIndex, get_texture_handle_index(usedTexture), CGPU_RESOURCE_STATE_COPY_DEST);
	pass.writes.push_back(write_edge);

	auto staging_buffer = pulse_rendergraph_declare_buffer(self);
	auto mipedSize = [](uint64_t sz, uint64_t mip) { return std::max<uint64_t>(sz >> mip, 1ull); };
	const uint64_t xBlocksCount = mipedSize(usedTextureNode->width, mipmap) / FormatUtil_WidthOfBlock(usedTextureNode->format);
	const uint64_t yBlocksCount = mipedSize(usedTextureNode->height, mipmap) / FormatUtil_HeightOfBlock(usedTextureNode->format);
	const uint64_t zBlocksCount = mipedSize(usedTextureNode->depth, mipmap);
	const uint64_t bufferSize = xBlocksCount * yBlocksCount * zBlocksCount * FormatUtil_BitSizeOfBlock(usedTextureNode->format) / 8;
	assert(bufferSize >= size + offset);
	pulse_rendergraph_buffer_set_size(self, staging_buffer, (uint32_t)bufferSize);
	pulse_rendergraph_buffer_set_type(self, staging_buffer, CGPU_RESOURCE_TYPE_NONE);
	pulse_rendergraph_buffer_set_usage(self, staging_buffer, CGPU_MEMORY_USAGE_CPU_ONLY);
	pulse_rendergraph_buffer_set_hold_on_last(self, staging_buffer);
	pass.upload_texture_context.staging_buffer = staging_buffer;
	auto read_edge = pulse_rendergraph_add_edge(self, get_buffer_handle_index(staging_buffer), passIndex, CGPU_RESOURCE_STATE_COPY_SOURCE);
	pass.reads.push_back(read_edge);

	pass.upload_texture_context.executable = executable;
	allocate_passdata(impl, &pass, passdata_size, out_passdata);
	pass.upload_texture_context.size = size;
	pass.upload_texture_context.offset = offset;
	pass.upload_texture_context.data = data;
	pass.upload_texture_context.mipmap = mipmap;
	pass.upload_texture_context.slice = slice;
}

void pulse_rendergraph_add_uploadbufferpass(pulse_rendergraph_t* self, const char* name, pulse_buffer_handle_t buffer, pulse_uploadpass_executable_t executable, uint32_t passdata_size, void** out_passdata)
{
	pulse_rendergraph_add_uploadbufferpass_ex(self, name, buffer, 0, 0, nullptr, executable, passdata_size, out_passdata);
}

void pulse_rendergraph_add_uploadbufferpass_ex(pulse_rendergraph_t* self, const char* name, pulse_buffer_handle_t buffer, uint64_t size, uint64_t offset, void* data, pulse_uploadpass_executable_t executable, uint32_t passdata_size, void** out_passdata)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_MAX_INDEX);
	auto& pass = impl->passes.emplace_back(name, PASS_TYPE_UPLOAD_BUFFER, impl->allocator.resource());
	int passIndex = (int)(impl->passes.size() - 1);

	assert(pulse_rendergraph_buffer_handle_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	pass.upload_buffer_context.dest_buffer = buffer;
	auto write_edge = pulse_rendergraph_add_edge(self, passIndex, get_buffer_handle_index(buffer), CGPU_RESOURCE_STATE_COPY_DEST);
	pass.writes.push_back(write_edge);

	auto staging_buffer = pulse_rendergraph_declare_buffer(self);
	assert(resourceNode.size >= size + offset);
	pulse_rendergraph_buffer_set_size(self, staging_buffer, resourceNode.size);
	pulse_rendergraph_buffer_set_type(self, staging_buffer, CGPU_RESOURCE_TYPE_NONE);
	pulse_rendergraph_buffer_set_usage(self, staging_buffer, CGPU_MEMORY_USAGE_CPU_ONLY);
	pulse_rendergraph_buffer_set_hold_on_last(self, staging_buffer);
	pass.upload_buffer_context.staging_buffer = staging_buffer;
	auto read_edge = pulse_rendergraph_add_edge(self, get_buffer_handle_index(staging_buffer), passIndex, CGPU_RESOURCE_STATE_COPY_SOURCE);
	pass.reads.push_back(read_edge);

	pass.upload_buffer_context.executable = (decltype(pass.upload_buffer_context.executable))executable;
	allocate_passdata(impl, &pass, passdata_size, out_passdata);
	pass.upload_buffer_context.size = size;
	pass.upload_buffer_context.offset = offset;
	pass.upload_buffer_context.data = data;
}

void pulse_rendergraph_add_generate_mipmap(pulse_rendergraph_t* self, pulse_texture_handle_t texture, uint8_t from_mipmap)
{
	auto* impl = to_impl(self);
	assert(pulse_rendergraph_texture_handle_valid(texture));
	auto& textureNode = impl->resources[get_texture_handle_index(texture)];
	assert(textureNode.resourceType == ResourceType::Texture && textureNode.manageType != ManageType::SubResource);
	assert(textureNode.arraySize == 1);
	if (textureNode.mipCount == 1)
		return;

	auto mip0 = pulse_rendergraph_declare_texture_subresource(self, texture, 0, 0);
	auto last = mip0;
	for (size_t i = from_mipmap; i < textureNode.mipCount; ++i)
	{
		auto mipi = pulse_rendergraph_declare_texture_subresource(self, texture, (uint8_t)i, 0);

		auto passBuilder = pulse_rendergraph_add_renderpass(self, "generate mip");
		pulse_renderpass_add_color_attachment(&passBuilder, mipi, CGPU_LOAD_ACTION_DONT_CARE, 0, CGPU_STORE_ACTION_STORE);
		pulse_renderpass_sample(&passBuilder, last);

		struct BlitMipmapPassData
		{
			pulse_shader_data_t* blitShader;
			CGPUSamplerId blitSampler;
			pulse_texture_handle_t source;
		};
		BlitMipmapPassData* passdata = nullptr;
		pulse_renderpass_set_executable(&passBuilder, [](pulse_renderpass_encoder_t* encoder, void* userdata)
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

void pulse_rendergraph_present(pulse_rendergraph_t* self, pulse_texture_handle_t texture)
{
	auto* impl = to_impl(self);
	assert(impl->passes.size() <= PULSE_MAX_INDEX);
	auto& passNode = impl->passes.emplace_back("Present", PASS_TYPE_PRESENT, impl->allocator.resource());
	int passIndex = (int)(impl->passes.size() - 1);
	auto edge = pulse_rendergraph_add_edge(self, get_texture_handle_index(texture), passIndex, CGPU_RESOURCE_STATE_PRESENT);
	passNode.reads.push_back(edge);
}

uint32_t pulse_rendergraph_add_edge(pulse_rendergraph_t* self, pulse_index_t from, pulse_index_t to, ECGPUResourceStateFlags usage)
{
	auto* impl = to_impl(self);
	impl->edges.emplace_back(from, to, usage);
	return (uint32_t)(impl->edges.size() - 1);
}

// Builder operations
void pulse_renderpass_add_color_attachment(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture, ECGPULoadAction load_action, uint32_t clear_color, ECGPUStoreAction store_action)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(passNode->type == PASS_TYPE_RENDER);
	assert(passNode->render_context.colorAttachmentCount <= (int)passNode->render_context.colorAttachments.size());

	auto edge = pulse_rendergraph_add_edge(self->render_graph, self->pass_index, get_texture_handle_index(texture), CGPU_RESOURCE_STATE_RENDER_TARGET);
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

void pulse_renderpass_add_depth_attachment(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture, ECGPULoadAction depth_load_action, float clear_depth, ECGPUStoreAction depth_store_action, ECGPULoadAction stencil_load_action, uint8_t clear_stencil, ECGPUStoreAction stencil_store_action)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(passNode->type == PASS_TYPE_RENDER);
	assert(!passNode->render_context.depthAttachment.valid);

	auto edge1 = pulse_rendergraph_add_edge(self->render_graph, get_texture_handle_index(texture), self->pass_index, CGPU_RESOURCE_STATE_UNDEFINED);
	passNode->reads.push_back(edge1);
	auto edge2 = pulse_rendergraph_add_edge(self->render_graph, self->pass_index, get_texture_handle_index(texture), CGPU_RESOURCE_STATE_DEPTH_WRITE);
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

void pulse_renderpass_sample(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto edge = pulse_rendergraph_add_edge(self->render_graph, get_texture_handle_index(texture), self->pass_index, CGPU_RESOURCE_STATE_SHADER_RESOURCE);
	passNode->reads.push_back(edge);
}

void pulse_renderpass_use_buffer(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rendergraph_buffer_handle_valid(buffer));
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

	auto edge = pulse_rendergraph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, state);
	passNode->reads.push_back(edge);
}

void pulse_renderpass_use_buffer_as(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer, ECGPUResourceStateFlags state)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rendergraph_buffer_handle_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	assert(state != CGPU_RESOURCE_STATE_UNDEFINED);

	auto edge = pulse_rendergraph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, state);
	passNode->reads.push_back(edge);
}

void pulse_renderpass_set_executable(pulse_renderpass_builder_t* self, pulse_renderpass_executable_t executable, uint32_t passdata_size, void** out_passdata)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	passNode->render_context.executable = executable;
	allocate_passdata(impl, passNode, passdata_size, out_passdata);
}

void pulse_computepass_sample(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto edge = pulse_rendergraph_add_edge(self->render_graph, get_texture_handle_index(texture), self->pass_index, CGPU_RESOURCE_STATE_SHADER_RESOURCE);
	passNode->reads.push_back(edge);
}

void pulse_computepass_use_buffer(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rendergraph_buffer_handle_valid(buffer));
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

	auto edge = pulse_rendergraph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, state);
	passNode->reads.push_back(edge);
}

void pulse_computepass_use_buffer_as(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer, ECGPUResourceStateFlags state)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rendergraph_buffer_handle_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);
	assert(state != CGPU_RESOURCE_STATE_UNDEFINED);

	auto edge = pulse_rendergraph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, state);
	passNode->reads.push_back(edge);
}

void pulse_computepass_readwrite_texture(pulse_renderpass_builder_t* self, pulse_texture_handle_t texture)
{
	// No-op for now (same as original)
}

void pulse_computepass_readwrite_buffer(pulse_renderpass_builder_t* self, pulse_buffer_handle_t buffer)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	assert(pulse_rendergraph_buffer_handle_valid(buffer));
	auto& resourceNode = impl->resources[get_buffer_handle_index(buffer)];
	assert(resourceNode.resourceType == ResourceType::Buffer);

	auto edge = pulse_rendergraph_add_edge(self->render_graph, get_buffer_handle_index(buffer), self->pass_index, CGPU_RESOURCE_STATE_UNORDERED_ACCESS);
	passNode->reads.push_back(edge);
	auto edge2 = pulse_rendergraph_add_edge(self->render_graph, self->pass_index, get_buffer_handle_index(buffer), CGPU_RESOURCE_STATE_UNORDERED_ACCESS);
	passNode->writes.push_back(edge2);
}

void pulse_computepass_set_executable(pulse_renderpass_builder_t* self, pulse_renderpass_executable_t executable, uint32_t passdata_size, void** out_passdata)
{
	auto* passNode = (RenderPassNode*)self->pass_node;
	auto* impl = to_impl(self->render_graph);
	passNode->compute_context.executable = executable;
	allocate_passdata(impl, passNode, passdata_size, out_passdata);
}

} // extern "C"
