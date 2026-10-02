#include "../graphics_internal.h"

namespace pulse_graphics_internal {

static void destroy_compute_shader(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    PulseComputeShaderData* data = static_cast<PulseComputeShaderData*>(ptr);
    if (data->root_sig) cgpu_device_free_root_signature(device, data->root_sig);
}

void register_compute_shader_type(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_COMPUTE_SHADER;
    type_desc.size = sizeof(PulseComputeShaderData);
    type_desc.align = alignof(PulseComputeShaderData);
    type_desc.destroy = destroy_compute_shader;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(asset_system, &type_desc);
}

} // namespace pulse_graphics_internal
