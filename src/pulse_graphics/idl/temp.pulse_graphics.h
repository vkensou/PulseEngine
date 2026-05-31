#pragma once

#ifndef PULSE_GRAPHICS_API_HEADER_GUARD
#define PULSE_GRAPHICS_API_HEADER_GUARD

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t

#include "cgpu/api.h"
#include "pulse_app.h"
#include "pulse_asset.h"
#include "rendergraph.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PULSE_API
#define PULSE_API
#endif

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

// ECS component declarations
extern ECS_COMPONENT_DECLARE(PulseGraphicsRenderer);
extern ECS_COMPONENT_DECLARE(PulseGraphicsSurface);
extern ECS_COMPONENT_DECLARE(PulseGraphicsSwapchain);

// ---- inline helpers for asset handle types ----

static inline PulseAssetSystemId pulse_get_graphics_asset_system(PulseAppId app) {
    return pulse_get_asset_system(app);
}

// Shader
static inline PulseAssetHandle pulse_graphics_shader_to_handle(PulseShaderHandle shader) {
    PulseAssetHandle h = { PULSE_TYPE_SHADER, shader.index, shader.generation };
    return h;
}
static inline bool pulse_graphics_shader_is_alive(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_shader_to_handle(shader));
}
static inline bool pulse_graphics_shader_is_ready(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_shader_to_handle(shader));
}
static inline void pulse_graphics_shader_unload(PulseAppId app, PulseShaderHandle shader) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_shader_to_handle(shader));
}

// ShaderLibrary
static inline PulseAssetHandle pulse_graphics_shader_library_to_handle(PulseShaderLibraryHandle lib) {
    PulseAssetHandle h = { PULSE_TYPE_SHADER_LIBRARY, lib.index, lib.generation };
    return h;
}
static inline bool pulse_graphics_shader_library_is_alive(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_shader_library_to_handle(lib));
}
static inline bool pulse_graphics_shader_library_is_ready(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_shader_library_to_handle(lib));
}
static inline void pulse_graphics_shader_library_unload(PulseAppId app, PulseShaderLibraryHandle lib) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_shader_library_to_handle(lib));
}

// ComputeShader
static inline PulseAssetHandle pulse_graphics_compute_shader_to_handle(PulseComputeShaderHandle cs) {
    PulseAssetHandle h = { PULSE_TYPE_COMPUTE_SHADER, cs.index, cs.generation };
    return h;
}
static inline bool pulse_graphics_compute_shader_is_alive(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_compute_shader_to_handle(cs));
}
static inline bool pulse_graphics_compute_shader_is_ready(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_compute_shader_to_handle(cs));
}
static inline void pulse_graphics_compute_shader_unload(PulseAppId app, PulseComputeShaderHandle cs) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_compute_shader_to_handle(cs));
}

// Buffer
static inline PulseAssetHandle pulse_graphics_buffer_to_handle(PulseBufferHandle buffer) {
    PulseAssetHandle h = { PULSE_TYPE_BUFFER, buffer.index, buffer.generation };
    return h;
}
static inline bool pulse_graphics_buffer_is_alive(PulseAppId app, PulseBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_buffer_to_handle(buffer));
}
static inline bool pulse_graphics_buffer_is_ready(PulseAppId app, PulseBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_buffer_to_handle(buffer));
}
static inline void pulse_graphics_buffer_unload(PulseAppId app, PulseBufferHandle buffer) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_buffer_to_handle(buffer));
}

// Sampler
static inline PulseAssetHandle pulse_graphics_sampler_to_handle(PulseSamplerHandle sampler) {
    PulseAssetHandle h = { PULSE_TYPE_SAMPLER, sampler.index, sampler.generation };
    return h;
}
static inline bool pulse_graphics_sampler_is_alive(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_sampler_to_handle(sampler));
}
static inline bool pulse_graphics_sampler_is_ready(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_sampler_to_handle(sampler));
}
static inline void pulse_graphics_sampler_unload(PulseAppId app, PulseSamplerHandle sampler) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_sampler_to_handle(sampler));
}

// Texture
static inline PulseAssetHandle pulse_graphics_texture_to_handle(PulseTextureHandle texture) {
    PulseAssetHandle h = { PULSE_TYPE_TEXTURE, texture.index, texture.generation };
    return h;
}
static inline bool pulse_graphics_texture_is_alive(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_texture_to_handle(texture));
}
static inline bool pulse_graphics_texture_is_ready(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_texture_to_handle(texture));
}
static inline void pulse_graphics_texture_unload(PulseAppId app, PulseTextureHandle texture) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_texture_to_handle(texture));
}

// Mesh
static inline PulseAssetHandle pulse_graphics_mesh_to_handle(PulseMeshHandle mesh) {
    PulseAssetHandle h = { PULSE_TYPE_MESH, mesh.index, mesh.generation };
    return h;
}
static inline bool pulse_graphics_mesh_is_alive(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_mesh_to_handle(mesh));
}
static inline bool pulse_graphics_mesh_is_ready(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_mesh_to_handle(mesh));
}
static inline void pulse_graphics_mesh_unload(PulseAppId app, PulseMeshHandle mesh) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_mesh_to_handle(mesh));
}

// Material
static inline PulseAssetHandle pulse_graphics_material_to_handle(PulseMaterialHandle material) {
    PulseAssetHandle h = { PULSE_TYPE_MATERIAL, material.index, material.generation };
    return h;
}
static inline bool pulse_graphics_material_is_alive(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_alive(as, pulse_graphics_material_to_handle(material));
}
static inline bool pulse_graphics_material_is_ready(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    return pulse_asset_system_is_ready(as, pulse_graphics_material_to_handle(material));
}
static inline void pulse_graphics_material_unload(PulseAppId app, PulseMaterialHandle material) {
    PulseAssetSystemId as = pulse_get_graphics_asset_system(app);
    pulse_asset_system_unload(as, pulse_graphics_material_to_handle(material));
}

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_GRAPHICS_API_HEADER_GUARD
