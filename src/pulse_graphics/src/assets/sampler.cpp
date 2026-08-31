#include "../graphics_internal.h"

namespace pulse_graphics_internal {

void destroy_sampler(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    PulseSamplerData* data = static_cast<PulseSamplerData*>(ptr);
    if (data->handle) cgpu_device_free_sampler(device, data->handle);
}

void register_sampler_type(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_SAMPLER;
    type_desc.size = sizeof(PulseSamplerData);
    type_desc.align = alignof(PulseSamplerData);
    type_desc.destroy = destroy_sampler;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(asset_system, &type_desc);
}

}

extern "C" {

PulseSamplerHandle pulse_sampler_get_handle(PulseAppId app, PulseSamplerRequest request) {
    PulseAssetHandle h = pulse_asset_system_get_handle(
        pulse_graphics_internal::asset_system_from_app(app), pulse_sampler_request_to_asset_request(request));
    return !pulse_asset_handle_is_valid(h) ? PulseSamplerHandle{} : PulseSamplerHandle{h.index, h.generation};
}

bool pulse_sampler_is_ready(PulseAppId app, PulseSamplerRequest request) {
    return pulse_asset_system_is_ready(
        pulse_graphics_internal::asset_system_from_app(app), pulse_sampler_request_to_asset_request(request));
}

bool pulse_sampler_is_alive(PulseAppId app, PulseSamplerRequest request) {
    return pulse_asset_system_is_alive(
        pulse_graphics_internal::asset_system_from_app(app), pulse_sampler_request_to_asset_request(request));
}

} // extern "C"
