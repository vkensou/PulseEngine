#include "das_cgpu.h"
#include "das_cgpu_types.h"

#include "daScript/misc/platform.h"
#include "daScript/ast/ast.h"
#include "daScript/ast/ast_interop.h"
#include "daScript/ast/ast_handle.h"
#include "daScript/ast/ast_typefactory_bind.h"
#include "daScript/simulate/bind_enum.h"

#include <cgpu/api.h>

// Minimal CGPU bindings used by the PulseEngine snake module.
// Only the enums and descriptor structs required to build the color shader
// pipeline are exposed; extend this file as more graphics features are needed.

DAS_BIND_ENUM_CAST(ECGPUBlendFactor);
DAS_BIND_ENUM_CAST(ECGPUBlendOp);
DAS_BIND_ENUM_CAST(ECGPUColorMaskFlagBits);
DAS_BIND_ENUM_CAST(ECGPUCompareOp);
DAS_BIND_ENUM_CAST(ECGPUCullModeFlagBits);
DAS_BIND_ENUM_CAST(ECGPUFrontFace);
DAS_BIND_ENUM_CAST(ECGPUFillMode);
DAS_BIND_ENUM_CAST(ECGPUStencilOp);

DAS_BASE_BIND_ENUM(ECGPUBlendFactor, ECGPUBlendFactor,
	CGPU_BLEND_FACTOR_ZERO,
	CGPU_BLEND_FACTOR_ONE);

DAS_BASE_BIND_ENUM(ECGPUBlendOp, ECGPUBlendOp,
	CGPU_BLEND_OP_ADD,
	CGPU_BLEND_OP_SUBTRACT,
	CGPU_BLEND_OP_REVERSE_SUBTRACT,
	CGPU_BLEND_OP_MIN,
	CGPU_BLEND_OP_MAX);

DAS_BASE_BIND_ENUM(ECGPUColorMaskFlagBits, ECGPUColorMaskFlagBits,
	CGPU_COLOR_MASK_R,
	CGPU_COLOR_MASK_G,
	CGPU_COLOR_MASK_B,
	CGPU_COLOR_MASK_A,
	CGPU_COLOR_MASK_RGB,
	CGPU_COLOR_MASK_RGBA);

DAS_BASE_BIND_ENUM(ECGPUCompareOp, ECGPUCompareOp,
	CGPU_COMPARE_OP_NEVER,
	CGPU_COMPARE_OP_LESS,
	CGPU_COMPARE_OP_EQUAL,
	CGPU_COMPARE_OP_LESS_EQUAL,
	CGPU_COMPARE_OP_GREATER,
	CGPU_COMPARE_OP_NOT_EQUAL,
	CGPU_COMPARE_OP_GREATER_EQUAL,
	CGPU_COMPARE_OP_ALWAYS);

DAS_BASE_BIND_ENUM(ECGPUCullModeFlagBits, ECGPUCullModeFlagBits,
	CGPU_CULL_MODE_NONE,
	CGPU_CULL_MODE_BACK,
	CGPU_CULL_MODE_FRONT,
	CGPU_CULL_MODE_BOTH);

DAS_BASE_BIND_ENUM(ECGPUFrontFace, ECGPUFrontFace,
	CGPU_FRONT_FACE_COUNTER_CLOCKWISE,
	CGPU_FRONT_FACE_CLOCK_WISE);

DAS_BASE_BIND_ENUM(ECGPUFillMode, ECGPUFillMode,
	CGPU_FILL_MODE_SOLID,
	CGPU_FILL_MODE_WIRE_FRAME);

DAS_BASE_BIND_ENUM(ECGPUStencilOp, ECGPUStencilOp,
	CGPU_STENCIL_OP_KEEP,
	CGPU_STENCIL_OP_ZERO,
	CGPU_STENCIL_OP_REPLACE,
	CGPU_STENCIL_OP_INCREMENT_AND_CLAMP,
	CGPU_STENCIL_OP_DECREMENT_AND_CLAMP,
	CGPU_STENCIL_OP_INVERT,
	CGPU_STENCIL_OP_INCREMENT_AND_WRAP,
	CGPU_STENCIL_OP_DECREMENT_AND_WRAP);

IMPLEMENT_EXTERNAL_TYPE_FACTORY(CGPUBlendAttachmentState, CGPUBlendAttachmentState);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CGPUBlendStateDescriptor, CGPUBlendStateDescriptor);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CGPUDepthStateDescriptor, CGPUDepthStateDescriptor);
IMPLEMENT_EXTERNAL_TYPE_FACTORY(CGPURasterizerStateDescriptor, CGPURasterizerStateDescriptor);

struct CGPUBlendAttachmentStateAnnotation final : das::ManagedStructureAnnotation<CGPUBlendAttachmentState>
{
	CGPUBlendAttachmentStateAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("CGPUBlendAttachmentState", ml, "CGPUBlendAttachmentState")
	{
		addField<DAS_BIND_MANAGED_FIELD(enable)>("enable");
		addField<DAS_BIND_MANAGED_FIELD(src_factor)>("src_factor");
		addField<DAS_BIND_MANAGED_FIELD(dst_factor)>("dst_factor");
		addField<DAS_BIND_MANAGED_FIELD(src_alpha_factor)>("src_alpha_factor");
		addField<DAS_BIND_MANAGED_FIELD(dst_alpha_factor)>("dst_alpha_factor");
		addField<DAS_BIND_MANAGED_FIELD(blend_op)>("blend_op");
		addField<DAS_BIND_MANAGED_FIELD(blend_alpha_op)>("blend_alpha_op");
		addField<DAS_BIND_MANAGED_FIELD(color_mask)>("color_mask");
	}
};

struct CGPUBlendStateDescriptorAnnotation final : das::ManagedStructureAnnotation<CGPUBlendStateDescriptor>
{
	CGPUBlendStateDescriptorAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("CGPUBlendStateDescriptor", ml, "CGPUBlendStateDescriptor")
	{
		addField<DAS_BIND_MANAGED_FIELD(attachment_count)>("attachment_count");
		addField<DAS_BIND_MANAGED_FIELD(p_attachments)>("p_attachments");
		addField<DAS_BIND_MANAGED_FIELD(alpha_to_coverage)>("alpha_to_coverage");
		addField<DAS_BIND_MANAGED_FIELD(independent_blend)>("independent_blend");
	}
};

struct CGPUDepthStateDescriptorAnnotation final : das::ManagedStructureAnnotation<CGPUDepthStateDescriptor>
{
	CGPUDepthStateDescriptorAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("CGPUDepthStateDescriptor", ml, "CGPUDepthStateDescriptor")
	{
		addField<DAS_BIND_MANAGED_FIELD(depth_test)>("depth_test");
		addField<DAS_BIND_MANAGED_FIELD(depth_write)>("depth_write");
		addField<DAS_BIND_MANAGED_FIELD(depth_op)>("depth_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_test)>("stencil_test");
		addField<DAS_BIND_MANAGED_FIELD(stencil_read_mask)>("stencil_read_mask");
		addField<DAS_BIND_MANAGED_FIELD(stencil_write_mask)>("stencil_write_mask");
		addField<DAS_BIND_MANAGED_FIELD(stencil_front_op)>("stencil_front_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_front_fail_op)>("stencil_front_fail_op");
		addField<DAS_BIND_MANAGED_FIELD(depth_front_fail_op)>("depth_front_fail_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_front_pass_op)>("stencil_front_pass_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_back_op)>("stencil_back_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_back_fail_op)>("stencil_back_fail_op");
		addField<DAS_BIND_MANAGED_FIELD(depth_back_fail_op)>("depth_back_fail_op");
		addField<DAS_BIND_MANAGED_FIELD(stencil_back_pass_op)>("stencil_back_pass_op");
	}
};

struct CGPURasterizerStateDescriptorAnnotation final : das::ManagedStructureAnnotation<CGPURasterizerStateDescriptor>
{
	CGPURasterizerStateDescriptorAnnotation(das::ModuleLibrary& ml)
		: ManagedStructureAnnotation("CGPURasterizerStateDescriptor", ml, "CGPURasterizerStateDescriptor")
	{
		addField<DAS_BIND_MANAGED_FIELD(cull_mode)>("cull_mode");
		addField<DAS_BIND_MANAGED_FIELD(depth_bias)>("depth_bias");
		addField<DAS_BIND_MANAGED_FIELD(slope_scaled_depth_bias)>("slope_scaled_depth_bias");
		addField<DAS_BIND_MANAGED_FIELD(fill_mode)>("fill_mode");
		addField<DAS_BIND_MANAGED_FIELD(front_face)>("front_face");
		addField<DAS_BIND_MANAGED_FIELD(enable_multi_sample)>("enable_multi_sample");
		addField<DAS_BIND_MANAGED_FIELD(enable_scissor)>("enable_scissor");
		addField<DAS_BIND_MANAGED_FIELD(enable_depth_clamp)>("enable_depth_clamp");
	}
};

namespace das
{
	ModuleCGPU::ModuleCGPU()
		: Module("cgpu")
	{
		ModuleLibrary lib(this);
		lib.addBuiltInModule();

		addEnumeration(new ::EnumerationECGPUBlendFactor());
		addEnumeration(new ::EnumerationECGPUBlendOp());
		addEnumeration(new ::EnumerationECGPUColorMaskFlagBits());
		addEnumeration(new ::EnumerationECGPUCompareOp());
		addEnumeration(new ::EnumerationECGPUCullModeFlagBits());
		addEnumeration(new ::EnumerationECGPUFrontFace());
		addEnumeration(new ::EnumerationECGPUFillMode());
		addEnumeration(new ::EnumerationECGPUStencilOp());

		addAnnotation(new CGPUBlendAttachmentStateAnnotation(lib));
		addAnnotation(new CGPUBlendStateDescriptorAnnotation(lib));
		addAnnotation(new CGPUDepthStateDescriptorAnnotation(lib));
		addAnnotation(new CGPURasterizerStateDescriptorAnnotation(lib));
	}
}

REGISTER_MODULE_IN_NAMESPACE(ModuleCGPU, das);
