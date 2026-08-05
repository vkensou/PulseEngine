#include "../graphics_internal.h"

namespace pulse_graphics_internal {

static void destroy_shader_library(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    PulseShaderLibraryData* data = static_cast<PulseShaderLibraryData*>(ptr);
    if (data->library) cgpu_device_free_shader_library(device, data->library);
}

void register_shader_library_type(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_SHADER_LIBRARY;
    type_desc.size = sizeof(PulseShaderLibraryData);
    type_desc.align = alignof(PulseShaderLibraryData);
    type_desc.destroy = destroy_shader_library;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(asset_system, &type_desc);
}

} // namespace pulse_graphics_internal

extern "C" {

PulseShaderLibraryHandle pulse_shader_library_get_handle(PulseAppId app, PulseShaderLibraryRequest request) {
    PulseAssetHandle h = pulse_asset_system_get_handle(
        pulse_graphics_internal::asset_system_from_app(app), pulse_shader_library_request_to_asset_request(request));
    return !pulse_asset_handle_is_valid(h) ? PulseShaderLibraryHandle{} : PulseShaderLibraryHandle{h.index, h.generation};
}

bool pulse_shader_library_is_ready(PulseAppId app, PulseShaderLibraryRequest request) {
    return pulse_asset_system_is_ready(
        pulse_graphics_internal::asset_system_from_app(app), pulse_shader_library_request_to_asset_request(request));
}

bool pulse_shader_library_is_alive(PulseAppId app, PulseShaderLibraryRequest request) {
    return pulse_asset_system_is_alive(
        pulse_graphics_internal::asset_system_from_app(app), pulse_shader_library_request_to_asset_request(request));
}

} // extern "C"
