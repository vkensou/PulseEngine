#include "graphic_internal.h"

namespace pulse_graphic_internal {

static void destroy_shader_library(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_shader_library_data_t* data = static_cast<pulse_shader_library_data_t*>(ptr);
    if (data->library) cgpu_device_free_shader_library(device, data->library);
}

void register_shader_library_type(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_type_desc type_desc{};
    type_desc.struct_size = sizeof(pulse_asset_type_desc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_SHADER_LIBRARY;
    type_desc.size = sizeof(pulse_shader_library_data_t);
    type_desc.align = alignof(pulse_shader_library_data_t);
    type_desc.destroy = destroy_shader_library;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_type(app, &type_desc);
}

} // namespace pulse_graphic_internal
