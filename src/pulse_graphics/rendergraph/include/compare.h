#pragma once

#include "cgpu/api.h"
#include <string.h>

namespace HGEGraphics
{
	inline bool name_equal(const char* a, const char* b)
	{
		if (a == b) return true;
		if (!a || !b) return false;
		return strcmp(a, b) == 0;
	}

	inline bool operator==(const CGPUVertexLayout& a, const CGPUVertexLayout& b)
	{
		if (a.attribute_count != b.attribute_count)
			return false;
		if (a.p_attributes == b.p_attributes) return true;
		if (!a.p_attributes || !b.p_attributes) return false;
		for (uint32_t i = 0; i < a.attribute_count; ++i)
		{
			const auto& A = a.p_attributes[i];
			const auto& B = b.p_attributes[i];
			if ((A.semantic_name != B.semantic_name) &&
				(A.semantic_name == nullptr || B.semantic_name == nullptr || strcmp(A.semantic_name, B.semantic_name) != 0))
				return false;
			if (A.array_size != B.array_size || A.format != B.format || A.binding != B.binding ||
				A.offset != B.offset || A.elem_stride != B.elem_stride || A.rate != B.rate)
				return false;
		}
		return true;
	}

	inline bool operator==(const CGPUBlendStateDescriptor& a, const CGPUBlendStateDescriptor& b)
	{
		if (a.attachment_count != b.attachment_count || a.alpha_to_coverage != b.alpha_to_coverage || a.independent_blend != b.independent_blend)
			return false;
		if (a.p_attachments == b.p_attachments) return true;
		if (!a.p_attachments || !b.p_attachments) return false;
		for (uint32_t i = 0; i < a.attachment_count; ++i)
		{
			const auto& A = a.p_attachments[i];
			const auto& B = b.p_attachments[i];
			if (A.enable != B.enable || A.src_factor != B.src_factor || A.dst_factor != B.dst_factor ||
				A.src_alpha_factor != B.src_alpha_factor || A.dst_alpha_factor != B.dst_alpha_factor ||
				A.blend_op != B.blend_op || A.blend_alpha_op != B.blend_alpha_op || A.color_mask != B.color_mask)
				return false;
		}
		return true;
	}

	inline bool operator==(const CGPUDepthStateDescriptor& a, const CGPUDepthStateDescriptor& b)
	{
		return a.depth_test == b.depth_test && a.depth_write == b.depth_write && a.depth_op == b.depth_op && a.stencil_test == b.stencil_test
			&& a.stencil_read_mask == b.stencil_read_mask && a.stencil_write_mask == b.stencil_write_mask && a.stencil_front_op == b.stencil_front_op
			&& a.stencil_front_fail_op == b.stencil_front_fail_op && a.depth_front_fail_op == b.depth_front_fail_op && a.stencil_front_pass_op == b.stencil_front_pass_op
			&& a.stencil_back_op == b.stencil_back_op && a.stencil_back_fail_op == b.stencil_back_fail_op && a.depth_back_fail_op == b.depth_back_fail_op 
			&& a.stencil_back_pass_op == b.stencil_back_pass_op;
	}

	inline bool operator==(const CGPURasterizerStateDescriptor& a, const CGPURasterizerStateDescriptor& b)
	{
		return a.cull_mode == b.cull_mode && a.depth_bias == b.depth_bias
			&& a.slope_scaled_depth_bias == b.slope_scaled_depth_bias && a.fill_mode == b.fill_mode 
			&& a.front_face == b.front_face && a.enable_multi_sample == b.enable_multi_sample
			&& a.enable_scissor == b.enable_scissor && a.enable_depth_clamp == b.enable_depth_clamp;
	}

	inline bool operator==(const CGPUDescriptorData& a, const CGPUDescriptorData& b)
	{
		return name_equal(a.name, b.name) && a.binding == b.binding && a.binding_type == b.binding_type
			&& ((a.binding_type != CGPU_RESOURCE_TYPE_UNIFORM_BUFFER && a.binding_type != CGPU_RESOURCE_TYPE_BUFFER
				&& a.binding_type != CGPU_RESOURCE_TYPE_BUFFER_RAW && a.binding_type != CGPU_RESOURCE_TYPE_RW_BUFFER
				&& a.binding_type != CGPU_RESOURCE_TYPE_RW_BUFFER_RAW)
				|| (a.params.buffers_params.sizes == b.params.buffers_params.sizes && a.params.buffers_params.offsets == b.params.buffers_params.offsets))
			&& a.resources.ptrs == b.resources.ptrs && a.count == b.count;
	}

	inline bool compare(CGPUDescriptorData a[], CGPUDescriptorData b[], size_t length)
	{
		for (size_t i = 0; i < length; ++i)
		{
			if (!(a[i] == b[i]))
				return false;
		}
		return true;
	}
}

namespace std
{
	template <>
	struct equal_to<CGPUBufferDescriptor>{
		bool operator()(const CGPUBufferDescriptor& a, const CGPUBufferDescriptor& b) const {
			return a.size == b.size && a.count_buffer == b.count_buffer && HGEGraphics::name_equal(a.name, b.name) && a.descriptors == b.descriptors 
				&& a.memory_usage == b.memory_usage && a.format == b.format && a.flags == b.flags && a.first_element == b.first_element 
				&& a.element_count == b.element_count && a.element_stride == b.element_stride && a.owner_queue == b.owner_queue 
				&& a.start_state == b.start_state && a.prefer_on_device == b.prefer_on_device && a.prefer_on_host == b.prefer_on_host;
		}
	};

	template <>
	struct equal_to<CGPUDescriptorSetDescriptor> {
		bool operator()(const CGPUDescriptorSetDescriptor& a, const CGPUDescriptorSetDescriptor& b) const {
			return a.root_signature == b.root_signature && a.set_index == b.set_index;
		}
	};

	template <>
	struct equal_to<CGPUFramebufferDescriptor> {
		bool operator()(const CGPUFramebufferDescriptor& a, const CGPUFramebufferDescriptor& b) const {
			return a.renderpass == b.renderpass && a.attachment_count == b.attachment_count
				&& a.width == b.width && a.height == b.height && a.layers == b.layers
				&& a.p_attachments[0] == b.p_attachments[0]
				&& a.p_attachments[1] == b.p_attachments[1]
				&& a.p_attachments[2] == b.p_attachments[2]
				&& a.p_attachments[3] == b.p_attachments[3]
				&& a.p_attachments[4] == b.p_attachments[4]
				&& a.p_attachments[5] == b.p_attachments[5]
				&& a.p_attachments[6] == b.p_attachments[6]
				&& a.p_attachments[7] == b.p_attachments[7]
				&& a.p_attachments[8] == b.p_attachments[8];
		}
	};

	template <>
	struct equal_to<CGPUColorAttachment> {
		bool operator()(const CGPUColorAttachment& a, const CGPUColorAttachment& b) const {
			return a.format == b.format && a.load_action == b.load_action && a.store_action == b.store_action;
		}
	};

	template <>
	struct equal_to<CGPUDepthStencilAttachment> {
		bool operator()(const CGPUDepthStencilAttachment& a, const CGPUDepthStencilAttachment& b) const {
			return a.format == b.format && a.depth_load_action == b.depth_load_action && a.depth_store_action == b.depth_store_action && a.stencil_load_action == b.stencil_load_action && a.stencil_store_action == b.stencil_store_action;
		}
	};

	template <>
	struct equal_to<CGPURenderPassDescriptor> {
		bool operator()(const CGPURenderPassDescriptor& a, const CGPURenderPassDescriptor& b) const {
			return !(bool)memcmp(&a, &b, sizeof(CGPURenderPassDescriptor));
		}
	};

	template <>
	struct equal_to<CGPUTextureViewDescriptor> {
		bool operator()(const CGPUTextureViewDescriptor& a, const CGPUTextureViewDescriptor& b) const {
			return HGEGraphics::name_equal(a.name, b.name) && a.texture == b.texture && a.format == b.format && a.usages == b.usages 
				&& a.aspects == b.aspects && a.dims == b.dims && a.base_array_layer == b.base_array_layer
				&& a.array_layer_count == b.array_layer_count && a.base_mip_level == b.base_mip_level 
				&& a.mip_level_count == b.mip_level_count;
		}
	};
}