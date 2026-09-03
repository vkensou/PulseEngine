#include "../graphics_internal.h"

#include <cstdlib>
#include <cstring>

namespace pulse_graphics_internal {

void register_graphics_asset_types_and_loaders(PulseAssetSystemId asset_system, CGPUDeviceId device) {
    register_shader_type(asset_system, device);
    register_compute_shader_type(asset_system, device);
    register_shader_library_type(asset_system, device);
    register_mesh_type(asset_system, device);
    register_texture_type(asset_system, device);
    register_buffer_type(asset_system, device);
    register_material_type(asset_system, device);
    register_sampler_type(asset_system, device);

    register_shader_library_load_loader(asset_system, device);
    register_shader_library_create_loader(asset_system, device);
    register_shader_create_loaders(asset_system, device);
    register_shader_load_loader(asset_system, device);
    register_compute_shader_create_loaders(asset_system, device);
    register_compute_shader_load_loader(asset_system, device);
    register_texture_create_loader(asset_system, device);
    register_texture_load_loader(asset_system, device);
    register_buffer_create_loader(asset_system, device);
    register_material_create_loader(asset_system, device);
    register_material_load_loader(asset_system, device);
    register_mesh_create_loader(asset_system, device);
    register_mesh_load_loader(asset_system, device);
    register_sampler_create_loader(asset_system, device);
}

} // namespace pulse_graphics_internal
