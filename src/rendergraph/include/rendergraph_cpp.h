#pragma once

#include "rendergraph.h"
#include "renderer.h"
#include "rendergraph_compiler.h"
#include "rendergraph_executor.h"
#include <memory_resource>

namespace HGEGraphics
{

	typedef void (*renderpass_executable)(RenderPassEncoder* encoder, void* userdata);
	typedef void (*uploadpass_executable)(UploadEncoder* encoder, void* userdata);

	struct texture_handle_t
	{
		pulse_index_t index;
		texture_handle_t() : index(0) {}
		texture_handle_t(pulse_index_t i) : index(i) {}
		texture_handle_t(pulse_texture_handle_t h) : index(h.index) {}
		operator pulse_texture_handle_t() const { pulse_texture_handle_t h = { index }; return h; }
	};

	struct buffer_handle_t
	{
		pulse_index_t index;
		buffer_handle_t() : index(0) {}
		buffer_handle_t(pulse_index_t i) : index(i) {}
		buffer_handle_t(pulse_buffer_handle_t h) : index(h.index) {}
		operator pulse_buffer_handle_t() const { pulse_buffer_handle_t h = { index }; return h; }
	};

	struct rendergraph_t
	{
		pulse_rendergraph_t* m_handle;

		rendergraph_t() : m_handle(nullptr) {}

		rendergraph_t(size_t estimate_resource_count, size_t estimate_pass_count, size_t estimate_edge_count, pulse_shader_data_t* blitShader, CGPUSamplerId blitSampler, std::pmr::memory_resource* const resource)
		{
			auto* impl = new pulse_rendergraph_impl_t(estimate_resource_count, estimate_pass_count, estimate_edge_count, blitShader, blitSampler, resource);
			m_handle = from_impl(impl);
		}

		~rendergraph_t()
		{
			if (m_handle)
				pulse_rendergraph_destroy(m_handle);
		}

		rendergraph_t(const rendergraph_t&) = delete;
		rendergraph_t& operator=(const rendergraph_t&) = delete;

		rendergraph_t(rendergraph_t&& other) noexcept : m_handle(other.m_handle)
		{
			other.m_handle = nullptr;
		}

		rendergraph_t& operator=(rendergraph_t&& other) noexcept
		{
			if (this != &other)
			{
				if (m_handle)
					pulse_rendergraph_destroy(m_handle);
				m_handle = other.m_handle;
				other.m_handle = nullptr;
			}
			return *this;
		}

		operator pulse_rendergraph_t* () const { return m_handle; }
	};

	struct renderpass_builder_t
	{
		pulse_renderpass_builder_t m_handle;
	};

	// Handle validity
	inline bool rendergraph_texture_handle_valid(texture_handle_t handle)
	{
		pulse_texture_handle_t h = { handle.index };
		return pulse_rendergraph_texture_handle_valid(h);
	}

	inline bool rendergraph_buffer_handle_valid(buffer_handle_t handle)
	{
		pulse_buffer_handle_t h = { handle.index };
		return pulse_rendergraph_buffer_handle_valid(h);
	}

	// Resource declaration
	inline texture_handle_t rendergraph_declare_texture(rendergraph_t* self)
	{
		pulse_texture_handle_t h = pulse_rendergraph_declare_texture(*self);
		return { h.index };
	}

	inline texture_handle_t rendergraph_import_texture(rendergraph_t* self, pulse_texture_data_t* imported)
	{
		pulse_texture_handle_t h = pulse_rendergraph_import_texture(*self, imported);
		return { h.index };
	}

	inline texture_handle_t rendergraph_import_backbuffer(rendergraph_t* self, Backbuffer* imported)
	{
		pulse_texture_handle_t h = pulse_rendergraph_import_backbuffer(*self, imported);
		return { h.index };
	}

	inline buffer_handle_t rendergraph_declare_buffer(rendergraph_t* self)
	{
		pulse_buffer_handle_t h = pulse_rendergraph_declare_buffer(*self);
		return { h.index };
	}

	inline buffer_handle_t rendergraph_import_buffer(rendergraph_t* self, pulse_buffer_data_t* imported)
	{
		pulse_buffer_handle_t h = pulse_rendergraph_import_buffer(*self, imported);
		return { h.index };
	}

	inline buffer_handle_t rendergraph_import_dynamic_buffer(rendergraph_t* self, pulse_buffer_data_t* imported)
	{
		pulse_buffer_handle_t h = pulse_rendergraph_import_dynamic_buffer(*self, imported);
		return { h.index };
	}

	inline buffer_handle_t rendergraph_declare_uniform_buffer_quick(rendergraph_t* self, uint32_t size, void* data)
	{
		pulse_buffer_handle_t h = pulse_rendergraph_declare_uniform_buffer_quick(*self, size, data);
		return { h.index };
	}

	inline texture_handle_t rendergraph_declare_texture_subresource(rendergraph_t* self, texture_handle_t parent, uint8_t mip_level, uint8_t array_slice)
	{
		pulse_texture_handle_t ph = { parent.index };
		pulse_texture_handle_t h = pulse_rendergraph_declare_texture_subresource(*self, ph, mip_level, array_slice);
		return { h.index };
	}

	// Texture properties
	inline void rg_texture_set_extent(rendergraph_t* self, texture_handle_t texture, uint32_t width, uint32_t height, uint32_t depth = 1)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_rendergraph_texture_set_extent(*self, h, width, height, depth);
	}

	inline void rg_texture_set_format(rendergraph_t* self, texture_handle_t texture, ECGPUTextureFormat format)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_rendergraph_texture_set_format(*self, h, format);
	}

	inline void rg_texture_set_depth_format(rendergraph_t* self, texture_handle_t texture, DepthBits depthBits, bool needStencil)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_rendergraph_texture_set_depth_format(*self, h, (pulse_depth_bits_t)depthBits, needStencil);
	}

	inline uint32_t rg_texture_get_width(rendergraph_t* self, texture_handle_t texture)
	{
		pulse_texture_handle_t h = { texture.index };
		return pulse_rendergraph_texture_get_width(*self, h);
	}

	inline uint32_t rg_texture_get_height(rendergraph_t* self, texture_handle_t texture)
	{
		pulse_texture_handle_t h = { texture.index };
		return pulse_rendergraph_texture_get_height(*self, h);
	}

	inline uint32_t rg_texture_get_depth(rendergraph_t* self, texture_handle_t texture)
	{
		pulse_texture_handle_t h = { texture.index };
		return pulse_rendergraph_texture_get_depth(*self, h);
	}

	inline ECGPUTextureFormat rg_texture_get_format(rendergraph_t* self, texture_handle_t texture)
	{
		pulse_texture_handle_t h = { texture.index };
		return pulse_rendergraph_texture_get_format(*self, h);
	}

	// Buffer properties
	inline void rg_buffer_set_size(rendergraph_t* self, buffer_handle_t buffer, uint32_t size)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_rendergraph_buffer_set_size(*self, h, size);
	}

	inline void rg_buffer_set_type(rendergraph_t* self, buffer_handle_t buffer, ECGPUResourceTypeFlags type)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_rendergraph_buffer_set_type(*self, h, type);
	}

	inline void rg_buffer_set_usage(rendergraph_t* self, buffer_handle_t buffer, ECGPUMemoryUsage usage)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_rendergraph_buffer_set_usage(*self, h, usage);
	}

	inline void rg_buffer_set_hold_on_last(rendergraph_t* self, buffer_handle_t buffer)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_rendergraph_buffer_set_hold_on_last(*self, h);
	}

	// Pass creation
	inline void rendergraph_reset(rendergraph_t* self)
	{
		pulse_rendergraph_reset(*self);
	}

	inline renderpass_builder_t rendergraph_add_renderpass(rendergraph_t* self, const char* name)
	{
		pulse_renderpass_builder_t b = pulse_rendergraph_add_renderpass(*self, name);
		return { b };
	}

	inline renderpass_builder_t rendergraph_add_computepass(rendergraph_t* self, const char* name)
	{
		pulse_renderpass_builder_t b = pulse_rendergraph_add_computepass(*self, name);
		return { b };
	}

	inline renderpass_builder_t rendergraph_add_holdpass(rendergraph_t* self, const char* name)
	{
		pulse_renderpass_builder_t b = pulse_rendergraph_add_holdpass(*self, name);
		return { b };
	}

	inline void rendergraph_add_uploadtexturepass(rendergraph_t* self, const char* name, texture_handle_t texture, uint8_t mip_level, uint8_t slice, uploadpass_executable executable, size_t passdata_size, void** out_passdata)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_rendergraph_add_uploadtexturepass(*self, name, h, mip_level, slice, (pulse_uploadpass_executable_t)executable, (uint32_t)passdata_size, out_passdata);
	}

	inline void rendergraph_add_uploadtexturepass_ex(rendergraph_t* self, const char* name, texture_handle_t texture, uint8_t mip_level, uint8_t slice, uint64_t size, uint64_t offset, void* data, uploadpass_executable executable, size_t passdata_size, void** out_passdata)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_rendergraph_add_uploadtexturepass_ex(*self, name, h, mip_level, slice, size, offset, data, (pulse_uploadpass_executable_t)executable, (uint32_t)passdata_size, out_passdata);
	}

	inline void rendergraph_add_uploadbufferpass(rendergraph_t* self, const char* name, buffer_handle_t buffer, uploadpass_executable executable, size_t passdata_size, void** out_passdata)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_rendergraph_add_uploadbufferpass(*self, name, h, (pulse_uploadpass_executable_t)executable, (uint32_t)passdata_size, out_passdata);
	}

	inline void rendergraph_add_uploadbufferpass_ex(rendergraph_t* self, const char* name, buffer_handle_t buffer, uint64_t size, uint64_t offset, void* data, uploadpass_executable executable, size_t passdata_size, void** out_passdata)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_rendergraph_add_uploadbufferpass_ex(*self, name, h, size, offset, data, (pulse_uploadpass_executable_t)executable, (uint32_t)passdata_size, out_passdata);
	}

	inline void rendergraph_add_generate_mipmap(rendergraph_t* self, texture_handle_t texture, uint8_t from_mip_level)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_rendergraph_add_generate_mipmap(*self, h, from_mip_level);
	}

	inline void rendergraph_present(rendergraph_t* self, texture_handle_t texture)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_rendergraph_present(*self, h);
	}

	inline uint32_t rendergraph_add_edge(rendergraph_t* self, index_type_t from, index_type_t to, ECGPUResourceStateFlags usage)
	{
		return pulse_rendergraph_add_edge(*self, from, to, usage);
	}

	// Builder operations
	inline void renderpass_add_color_attachment(renderpass_builder_t* self, texture_handle_t texture, ECGPULoadAction load_action, uint32_t clear_color, ECGPUStoreAction store_action)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_renderpass_add_color_attachment(&self->m_handle, h, load_action, clear_color, store_action);
	}

	inline void renderpass_add_depth_attachment(renderpass_builder_t* self, texture_handle_t texture, ECGPULoadAction depth_load_action, float clear_depth, ECGPUStoreAction depth_store_action, ECGPULoadAction stencil_load_action, uint8_t clear_stencil, ECGPUStoreAction stencil_store_action)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_renderpass_add_depth_attachment(&self->m_handle, h, depth_load_action, clear_depth, depth_store_action, stencil_load_action, clear_stencil, stencil_store_action);
	}

	inline void renderpass_sample(renderpass_builder_t* self, texture_handle_t texture)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_renderpass_sample(&self->m_handle, h);
	}

	inline void renderpass_use_buffer(renderpass_builder_t* self, buffer_handle_t buffer)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_renderpass_use_buffer(&self->m_handle, h);
	}

	inline void renderpass_use_buffer_as(renderpass_builder_t* self, buffer_handle_t buffer, ECGPUResourceStateFlags state)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_renderpass_use_buffer_as(&self->m_handle, h, state);
	}

	inline void renderpass_set_executable(renderpass_builder_t* self, renderpass_executable executable, size_t passdata_size, void** out_passdata)
	{
		pulse_renderpass_set_executable(&self->m_handle, (pulse_renderpass_executable_t)executable, (uint32_t)passdata_size, out_passdata);
	}

	inline void computepass_sample(renderpass_builder_t* self, texture_handle_t texture)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_computepass_sample(&self->m_handle, h);
	}

	inline void computepass_use_buffer(renderpass_builder_t* self, buffer_handle_t buffer)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_computepass_use_buffer(&self->m_handle, h);
	}

	inline void computepass_use_buffer_as(renderpass_builder_t* self, buffer_handle_t buffer, ECGPUResourceStateFlags state)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_computepass_use_buffer_as(&self->m_handle, h, state);
	}

	inline void computepass_readwrite_texture(renderpass_builder_t* self, texture_handle_t texture)
	{
		pulse_texture_handle_t h = { texture.index };
		pulse_computepass_readwrite_texture(&self->m_handle, h);
	}

	inline void computepass_readwrite_buffer(renderpass_builder_t* self, buffer_handle_t buffer)
	{
		pulse_buffer_handle_t h = { buffer.index };
		pulse_computepass_readwrite_buffer(&self->m_handle, h);
	}

	inline void computepass_set_executable(renderpass_builder_t* self, renderpass_executable executable, size_t passdata_size, void** out_passdata)
	{
		pulse_computepass_set_executable(&self->m_handle, (pulse_renderpass_executable_t)executable, (uint32_t)passdata_size, out_passdata);
	}

	// Resolution
	inline CGPUBufferId rendergraph_resolve_buffer(RenderPassEncoder* encoder, buffer_handle_t buffer_handle)
	{
		pulse_buffer_handle_t h = { buffer_handle.index };
		return pulse_rendergraph_resolve_buffer((pulse_renderpass_encoder_t*)encoder, h);
	}

	inline CGPUTextureViewId rendergraph_resolve_texture_view(RenderPassEncoder* encoder, texture_handle_t texture_handle)
	{
		pulse_texture_handle_t h = { texture_handle.index };
		return pulse_rendergraph_resolve_texture_view((pulse_renderpass_encoder_t*)encoder, h);
	}

}
