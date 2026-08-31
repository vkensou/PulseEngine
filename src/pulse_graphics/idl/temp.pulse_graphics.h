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

static inline PulseAssetSystemId pulse_get_graphics_asset_system(PulseAppId app) {
    return pulse_get_asset_system(app);
}

// Shader
static inline PulseAssetHandle pulse_shader_to_handle(PulseShaderHandle shader) {
    PulseAssetHandle h = { PULSE_TYPE_SHADER, shader.index, shader.generation };
    return h;
}
static inline PulseAssetRequest pulse_shader_request_to_asset_request(PulseShaderRequest shader) {
    PulseAssetRequest r = { PULSE_TYPE_SHADER, shader.index, shader.generation };
    return r;
}

// ShaderLibrary
static inline PulseAssetHandle pulse_shader_library_to_handle(PulseShaderLibraryHandle lib) {
    PulseAssetHandle h = { PULSE_TYPE_SHADER_LIBRARY, lib.index, lib.generation };
    return h;
}
static inline PulseAssetRequest pulse_shader_library_request_to_asset_request(PulseShaderLibraryRequest lib) {
    PulseAssetRequest r = { PULSE_TYPE_SHADER_LIBRARY, lib.index, lib.generation };
    return r;
}

// ComputeShader
static inline PulseAssetHandle pulse_compute_shader_to_handle(PulseComputeShaderHandle cs) {
    PulseAssetHandle h = { PULSE_TYPE_COMPUTE_SHADER, cs.index, cs.generation };
    return h;
}
static inline PulseAssetRequest pulse_compute_shader_request_to_asset_request(PulseComputeShaderRequest cs) {
    PulseAssetRequest r = { PULSE_TYPE_COMPUTE_SHADER, cs.index, cs.generation };
    return r;
}

// Buffer
static inline PulseAssetHandle pulse_graphics_buffer_to_handle(PulseGraphicsBufferHandle buffer) {
    PulseAssetHandle h = { PULSE_TYPE_GRAPHICS_BUFFER, buffer.index, buffer.generation };
    return h;
}
static inline PulseAssetRequest pulse_graphics_buffer_request_to_asset_request(PulseGraphicsBufferRequest buffer) {
    PulseAssetRequest r = { PULSE_TYPE_GRAPHICS_BUFFER, buffer.index, buffer.generation };
    return r;
}

// Sampler
static inline PulseAssetHandle pulse_sampler_to_handle(PulseSamplerHandle sampler) {
    PulseAssetHandle h = { PULSE_TYPE_SAMPLER, sampler.index, sampler.generation };
    return h;
}
static inline PulseAssetRequest pulse_sampler_request_to_asset_request(PulseSamplerRequest sampler) {
    PulseAssetRequest r = { PULSE_TYPE_SAMPLER, sampler.index, sampler.generation };
    return r;
}

// Texture
static inline PulseAssetHandle pulse_texture_to_handle(PulseTextureHandle texture) {
    PulseAssetHandle h = { PULSE_TYPE_TEXTURE, texture.index, texture.generation };
    return h;
}
static inline PulseAssetRequest pulse_texture_request_to_asset_request(PulseTextureRequest texture) {
    PulseAssetRequest r = { PULSE_TYPE_TEXTURE, texture.index, texture.generation };
    return r;
}

// Mesh
static inline PulseAssetHandle pulse_mesh_to_handle(PulseMeshHandle mesh) {
    PulseAssetHandle h = { PULSE_TYPE_MESH, mesh.index, mesh.generation };
    return h;
}
static inline PulseAssetRequest pulse_mesh_request_to_asset_request(PulseMeshRequest mesh) {
    PulseAssetRequest r = { PULSE_TYPE_MESH, mesh.index, mesh.generation };
    return r;
}

// Material
static inline PulseAssetHandle pulse_material_to_handle(PulseMaterialHandle material) {
    PulseAssetHandle h = { PULSE_TYPE_MATERIAL, material.index, material.generation };
    return h;
}
static inline PulseAssetRequest pulse_material_request_to_asset_request(PulseMaterialRequest material) {
    PulseAssetRequest r = { PULSE_TYPE_MATERIAL, material.index, material.generation };
    return r;
}

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
