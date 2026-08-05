#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

static void destroy_texture(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    PulseTextureData* data = static_cast<PulseTextureData*>(ptr);
    if (data->view) cgpu_device_free_texture_view(device, data->view);
    if (data->handle) cgpu_device_free_texture(device, data->handle);
}

void register_texture_type(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_TEXTURE;
    type_desc.size = sizeof(PulseTextureData);
    type_desc.align = alignof(PulseTextureData);
    type_desc.destroy = destroy_texture;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(asset_system, &type_desc);
}

}

extern "C" {

PulseTextureHandle pulse_texture_get_handle(PulseAppId app, PulseTextureRequest request) {
    PulseAssetHandle h = pulse_asset_system_get_handle(
        pulse_graphics_internal::asset_system_from_app(app), pulse_texture_request_to_asset_request(request));
    return !pulse_asset_handle_is_valid(h) ? PulseTextureHandle{} : PulseTextureHandle{h.index, h.generation};
}

bool pulse_texture_is_ready(PulseAppId app, PulseTextureRequest request) {
    return pulse_asset_system_is_ready(
        pulse_graphics_internal::asset_system_from_app(app), pulse_texture_request_to_asset_request(request));
}

bool pulse_texture_is_alive(PulseAppId app, PulseTextureRequest request) {
    return pulse_asset_system_is_alive(
        pulse_graphics_internal::asset_system_from_app(app), pulse_texture_request_to_asset_request(request));
}

} // extern "C"
