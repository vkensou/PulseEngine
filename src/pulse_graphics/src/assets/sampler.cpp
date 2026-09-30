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

} // namespace pulse_graphics_internal
