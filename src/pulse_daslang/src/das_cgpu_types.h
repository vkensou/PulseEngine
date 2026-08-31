#pragma once

#include <cgpu/api.h>

#include "daScript/ast/ast_typefactory.h"

// Shared type factory declarations for the minimal CGPU struct bindings.
// The enum and struct annotations live in ModuleCGPU (das_cgpu.cpp); other
// modules embedding these structs (e.g. PulseShaderCreateFromFileDesc in
// ModulePulse) need the specializations declared here.

MAKE_EXTERNAL_TYPE_FACTORY(CGPUBlendAttachmentState, CGPUBlendAttachmentState);
MAKE_EXTERNAL_TYPE_FACTORY(CGPUBlendStateDescriptor, CGPUBlendStateDescriptor);
MAKE_EXTERNAL_TYPE_FACTORY(CGPUDepthStateDescriptor, CGPUDepthStateDescriptor);
MAKE_EXTERNAL_TYPE_FACTORY(CGPURasterizerStateDescriptor, CGPURasterizerStateDescriptor);
