#include "../graphics_internal.h"

namespace pulse_graphics_internal {

static void destroy_shader_library(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_shader_library_data_t* data = static_cast<pulse_shader_library_data_t*>(ptr);
    if (data->library) cgpu_device_free_shader_library(device, data->library);
}

void register_shader_library_type(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_SHADER_LIBRARY;
    type_desc.size = sizeof(pulse_shader_library_data_t);
    type_desc.align = alignof(pulse_shader_library_data_t);
    type_desc.destroy = destroy_shader_library;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    PulseAssetSystemId as = pulse_get_asset_system(app);
    pulse_asset_system_register_type(pulse_get_asset_system(app), &type_desc);
}

} // namespace pulse_graphics_internal
