#include "../graphics_internal.h"

namespace pulse_graphics_internal {

void destroy_sampler(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_sampler_data_t* data = static_cast<pulse_sampler_data_t*>(ptr);
    if (data->handle) cgpu_device_free_sampler(device, data->handle);
}

void register_sampler_type(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_SAMPLER;
    type_desc.size = sizeof(pulse_sampler_data_t);
    type_desc.align = alignof(pulse_sampler_data_t);
    type_desc.destroy = destroy_sampler;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    PulseAssetSystemId as = pulse_get_asset_system(app);
    pulse_asset_system_register_type(pulse_get_asset_system(app), &type_desc);
}

}

extern "C" {

bool pulse_acquire_sampler(PulseAppId app, PulseSamplerHandle handle, PulseSampler* sampler_ref) {
    PulseAssetRef ref{};
    if (pulse_asset_system_acquire(pulse_get_asset_system(app), pulse_sampler_to_handle(handle), &ref)) {
        sampler_ref->handle = handle;
        sampler_ref->ptr = static_cast<pulse_sampler_data_t*>(ref.ptr);
        return true;
    }

    sampler_ref->handle = {};
    sampler_ref->ptr = nullptr;
    return false;
}

void pulse_release_sampler(PulseAppId app, PulseSampler* sampler_ref) {
    PulseAssetRef ref{ pulse_sampler_to_handle(sampler_ref->handle), nullptr };
    pulse_asset_system_release(pulse_get_asset_system(app), &ref);
    sampler_ref->handle = {};
    sampler_ref->ptr = nullptr;
}

} // extern "C"
