#include "graphics_internal.h"

namespace pulse_graphics_internal {

namespace {

int serialize_asset_path(const flecs::serializer* ser, PulseAssetHandle asset) {
    const char* path = "";
    pulse_graphics_state* state = state_from_world(const_cast<ecs_world_t*>(ser->world));
    if (state && state->asset_system) {
        const char* asset_path = pulse_asset_system_get_path(state->asset_system, asset);
        if (asset_path) {
            path = asset_path;
        }
    }
    return ser->value(ecs_id(ecs_string_t), &path);
}

int serialize_shader_handle(const flecs::serializer* ser, const PulseShaderHandle* mesh) {
    return serialize_asset_path(ser, pulse_shader_to_handle(*mesh));
}

int serialize_shader_library_handle(const flecs::serializer* ser, const PulseShaderLibraryHandle* mesh) {
    return serialize_asset_path(ser, pulse_shader_library_to_handle(*mesh));
}

int serialize_compute_shader_handle(const flecs::serializer* ser, const PulseComputeShaderHandle* mesh) {
    return serialize_asset_path(ser, pulse_compute_shader_to_handle(*mesh));
}

int serialize_graphics_buffer_handle(const flecs::serializer* ser, const PulseGraphicsBufferHandle* mesh) {
    return serialize_asset_path(ser, pulse_graphics_buffer_to_handle(*mesh));
}

int serialize_sampler_handle(const flecs::serializer* ser, const PulseSamplerHandle* mesh) {
    return serialize_asset_path(ser, pulse_sampler_to_handle(*mesh));
}

int serialize_texture_handle(const flecs::serializer* ser, const PulseTextureHandle* mesh) {
    return serialize_asset_path(ser, pulse_texture_to_handle(*mesh));
}

int serialize_mesh_handle(const flecs::serializer* ser, const PulseMeshHandle* mesh) {
    return serialize_asset_path(ser, pulse_mesh_to_handle(*mesh));
}

int serialize_material_handle(const flecs::serializer* ser, const PulseMaterialHandle* material) {
    return serialize_asset_path(ser, pulse_material_to_handle(*material));
}

} // namespace

void register_asset_reflection(ecs_world_t* world) {
    flecs::opaque<PulseShaderHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_shader_handle);
    flecs::opaque<PulseShaderLibraryHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_shader_library_handle);
    flecs::opaque<PulseComputeShaderHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_compute_shader_handle);
    flecs::opaque<PulseGraphicsBufferHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_graphics_buffer_handle);
    flecs::opaque<PulseSamplerHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_sampler_handle);
    flecs::opaque<PulseTextureHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_texture_handle);
    flecs::opaque<PulseMeshHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_mesh_handle);
    flecs::opaque<PulseMaterialHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_material_handle);
}

} // namespace pulse_graphics_internal
