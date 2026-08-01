#pragma once

#ifndef PULSE_GRAPHICS_API_HEADER_GUARD
#define PULSE_GRAPHICS_API_HEADER_GUARD

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t

#include "cgpu/api.h"
#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PULSE_API
#define PULSE_API
#endif

typedef struct pulse_backbuffer_data_t pulse_backbuffer_data_t;
typedef struct pulse_texture_data_t pulse_texture_data_t;
typedef struct pulse_buffer_data_t pulse_buffer_data_t;

$cconsts

$cenums

$cflags

$cids

typedef struct PulseRenderPassEncoder PulseRenderPassEncoder;
typedef struct PulseUploadPassEncoder PulseUploadPassEncoder;

$cfuncptrs

$cstructs

// ECS component declarations
extern ECS_COMPONENT_DECLARE(PulseRenderer);
extern ECS_COMPONENT_DECLARE(PulseSurface);
extern ECS_COMPONENT_DECLARE(PulseSwapchain);

// ---- inline helpers for asset handle types ----

static inline PulseAssetSystemId pulse_get_graphics_asset_system(PulseAppId app) {
    return pulse_get_asset_system(app);
}

// Shader
static inline PulseAssetHandle pulse_shader_to_handle(PulseShaderHandle shader) {
    PulseAssetHandle h = { PULSE_TYPE_SHADER, shader.index, shader.generation };
    return h;
}
static inline bool pulse_shader_is_alive(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_shader_to_handle(shader));
}
static inline bool pulse_shader_is_ready(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_shader_to_handle(shader));
}
static inline void pulse_unload_shader(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_shader_to_handle(shader));
}

// ShaderLibrary
static inline PulseAssetHandle pulse_shader_library_to_handle(PulseShaderLibraryHandle lib) {
    PulseAssetHandle h = { PULSE_TYPE_SHADER_LIBRARY, lib.index, lib.generation };
    return h;
}
static inline bool pulse_shader_library_is_alive(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_shader_library_to_handle(lib));
}
static inline bool pulse_shader_library_is_ready(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_shader_library_to_handle(lib));
}
static inline void pulse_unload_shader_library(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_shader_library_to_handle(lib));
}

// ComputeShader
static inline PulseAssetHandle pulse_compute_shader_to_handle(PulseComputeShaderHandle cs) {
    PulseAssetHandle h = { PULSE_TYPE_COMPUTE_SHADER, cs.index, cs.generation };
    return h;
}
static inline bool pulse_compute_shader_is_alive(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_compute_shader_to_handle(cs));
}
static inline bool pulse_compute_shader_is_ready(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_compute_shader_to_handle(cs));
}
static inline void pulse_unload_compute_shader(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_compute_shader_to_handle(cs));
}

// Buffer
static inline PulseAssetHandle pulse_graphics_buffer_to_handle(PulseGraphicsBufferHandle buffer) {
    PulseAssetHandle h = { PULSE_TYPE_GRAPHICS_BUFFER, buffer.index, buffer.generation };
    return h;
}
static inline bool pulse_graphics_buffer_is_alive(PulseAppId app, PulseGraphicsBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_buffer_to_handle(buffer));
}
static inline bool pulse_graphics_buffer_is_ready(PulseAppId app, PulseGraphicsBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_buffer_to_handle(buffer));
}
static inline void pulse_unload_graphics_buffer(PulseAppId app, PulseGraphicsBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_buffer_to_handle(buffer));
}

// Sampler
static inline PulseAssetHandle pulse_sampler_to_handle(PulseSamplerHandle sampler) {
    PulseAssetHandle h = { PULSE_TYPE_SAMPLER, sampler.index, sampler.generation };
    return h;
}
static inline bool pulse_sampler_is_alive(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_sampler_to_handle(sampler));
}
static inline bool pulse_sampler_is_ready(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_sampler_to_handle(sampler));
}
static inline void pulse_unload_sampler(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_sampler_to_handle(sampler));
}

// Texture
static inline PulseAssetHandle pulse_texture_to_handle(PulseTextureHandle texture) {
    PulseAssetHandle h = { PULSE_TYPE_TEXTURE, texture.index, texture.generation };
    return h;
}
static inline bool pulse_texture_is_alive(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_texture_to_handle(texture));
}
static inline bool pulse_texture_is_ready(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_texture_to_handle(texture));
}
static inline void pulse_unload_texture(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_texture_to_handle(texture));
}

// Mesh
static inline PulseAssetHandle pulse_mesh_to_handle(PulseMeshHandle mesh) {
    PulseAssetHandle h = { PULSE_TYPE_MESH, mesh.index, mesh.generation };
    return h;
}
static inline bool pulse_mesh_is_alive(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_mesh_to_handle(mesh));
}
static inline bool pulse_mesh_is_ready(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_mesh_to_handle(mesh));
}
static inline void pulse_unload_mesh(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_mesh_to_handle(mesh));
}

// Material
static inline PulseAssetHandle pulse_material_to_handle(PulseMaterialHandle material) {
    PulseAssetHandle h = { PULSE_TYPE_MATERIAL, material.index, material.generation };
    return h;
}
static inline bool pulse_material_is_alive(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_material_to_handle(material));
}
static inline bool pulse_material_is_ready(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_material_to_handle(material));
}
static inline void pulse_unload_material(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_material_to_handle(material));
}

$c99decl

$cswitches

#ifdef __cplusplus
}
#endif

#endif // PULSE_GRAPHICS_API_HEADER_GUARD
