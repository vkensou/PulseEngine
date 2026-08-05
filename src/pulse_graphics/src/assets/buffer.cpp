#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

static void destroy_buffer(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    PulseGraphicsBufferData* data = static_cast<PulseGraphicsBufferData*>(ptr);
    if (data->handle) cgpu_device_free_buffer(device, data->handle);
}

void register_buffer_type(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_GRAPHICS_BUFFER;
    type_desc.size = sizeof(PulseGraphicsBufferData);
    type_desc.align = alignof(PulseGraphicsBufferData);
    type_desc.destroy = destroy_buffer;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(asset_system, &type_desc);
}

}

extern "C" {

PulseGraphicsBufferHandle pulse_graphics_buffer_get_handle(PulseAppId app, PulseGraphicsBufferRequest request) {
    PulseAssetHandle h = pulse_asset_system_get_handle(
        pulse_graphics_internal::asset_system_from_app(app), pulse_graphics_buffer_request_to_asset_request(request));
    return !pulse_asset_handle_is_valid(h) ? PulseGraphicsBufferHandle{} : PulseGraphicsBufferHandle{h.index, h.generation};
}

bool pulse_graphics_buffer_is_ready(PulseAppId app, PulseGraphicsBufferRequest request) {
    return pulse_asset_system_is_ready(
        pulse_graphics_internal::asset_system_from_app(app), pulse_graphics_buffer_request_to_asset_request(request));
}

bool pulse_graphics_buffer_is_alive(PulseAppId app, PulseGraphicsBufferRequest request) {
    return pulse_asset_system_is_alive(
        pulse_graphics_internal::asset_system_from_app(app), pulse_graphics_buffer_request_to_asset_request(request));
}

} // extern "C"
