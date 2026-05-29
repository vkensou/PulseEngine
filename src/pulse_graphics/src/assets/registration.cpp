#include "../graphics_internal.h"

#include <cstdlib>
#include <cstring>

namespace pulse_graphics_internal {

void register_graphics_asset_types_and_loaders(pulse_app_t app, CGPUDeviceId device) {
    register_shader_type(app, device);
    register_compute_shader_type(app, device);
    register_shader_library_type(app, device);
    register_mesh_type(app, device);
    register_texture_type(app, device);
    register_buffer_type(app, device);
    register_material_type(app, device);
    register_sampler_type(app, device);

    register_shader_library_load_loader(app, device);
    register_shader_library_create_loader(app, device);
    register_shader_create_loaders(app, device);
    register_compute_shader_create_loaders(app, device);
    register_texture_create_loader(app, device);
    register_texture_load_loader(app, device);
    register_buffer_create_loader(app, device);
    register_material_create_loader(app, device);
    register_mesh_create_loader(app, device);
    register_mesh_load_loader(app, device);
    register_sampler_create_loader(app, device);
}

} // namespace pulse_graphics_internal
