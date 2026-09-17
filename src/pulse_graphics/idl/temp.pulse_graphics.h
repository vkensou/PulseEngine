#pragma once

#ifndef PULSE_GRAPHICS_API_HEADER_GUARD
#define PULSE_GRAPHICS_API_HEADER_GUARD
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-attributes"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wattributes"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:5030)
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t
#include "pulse_platform.h"

#include "cgpu/api.h"
#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_math.h"

#if defined(PULSE_GRAPHICS_MODULE_BUILD)
#  define PULSE_GRAPHICS_API PULSE_EXPORT
#else
#  define PULSE_GRAPHICS_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pulse_backbuffer_data_t pulse_backbuffer_data_t;

$cconsts

$cenums

$cflags

$cids

typedef struct PulseRenderPassEncoder PulseRenderPassEncoder;
typedef struct PulseUploadPassEncoder PulseUploadPassEncoder;

$cfuncptrs

$cstructs

$ccomponents

// ---- inline helpers for asset handle types ----

PULSE_DEFINE_ASSET_CONVERSIONS(shader,          PULSE_TYPE_SHADER,          PulseShaderHandle,          PulseShaderRequest)
PULSE_DEFINE_ASSET_CONVERSIONS(shader_library,  PULSE_TYPE_SHADER_LIBRARY,  PulseShaderLibraryHandle,   PulseShaderLibraryRequest)
PULSE_DEFINE_ASSET_CONVERSIONS(compute_shader,  PULSE_TYPE_COMPUTE_SHADER,  PulseComputeShaderHandle,   PulseComputeShaderRequest)
PULSE_DEFINE_ASSET_CONVERSIONS(graphics_buffer, PULSE_TYPE_GRAPHICS_BUFFER, PulseGraphicsBufferHandle,  PulseGraphicsBufferRequest)
PULSE_DEFINE_ASSET_CONVERSIONS(sampler,         PULSE_TYPE_SAMPLER,         PulseSamplerHandle,         PulseSamplerRequest)
PULSE_DEFINE_ASSET_CONVERSIONS(texture,         PULSE_TYPE_TEXTURE,         PulseTextureHandle,         PulseTextureRequest)
PULSE_DEFINE_ASSET_CONVERSIONS(mesh,            PULSE_TYPE_MESH,            PulseMeshHandle,            PulseMeshRequest)
PULSE_DEFINE_ASSET_CONVERSIONS(material,        PULSE_TYPE_MATERIAL,        PulseMaterialHandle,        PulseMaterialRequest)

$c99decl

$cswitches

#ifdef __cplusplus
}
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
#endif // PULSE_GRAPHICS_API_HEADER_GUARD
