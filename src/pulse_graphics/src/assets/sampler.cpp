#include "../graphics_internal.h"

namespace pulse_graphics_internal {

void destroy_sampler(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_sampler_data_t* data = static_cast<pulse_sampler_data_t*>(ptr);
    if (data->handle) cgpu_device_free_sampler(device, data->handle);
}

void register_sampler_type(PulseAppId app, CGPUDeviceId device)
{
    pulse_asset_type_desc type_desc{};
    type_desc.struct_size = sizeof(pulse_asset_type_desc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_SAMPLER;
    type_desc.size = sizeof(pulse_sampler_data_t);
    type_desc.align = alignof(pulse_sampler_data_t);
    type_desc.destroy = destroy_sampler;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_type(app, &type_desc);
}

}

extern "C" {

bool pulse_graphics_sampler_acquire(PulseAppId app, pulse_sampler_t handle, pulse_graphics_sampler_ref* sampler_ref) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, pulse_graphics_sampler_to_handle(handle), &ref)) {
        sampler_ref->handle = handle;
        sampler_ref->ptr = static_cast<pulse_sampler_data_t*>(ref.ptr);
        return true;
    }

    sampler_ref->handle = {};
    sampler_ref->ptr = nullptr;
    return false;
}

void pulse_graphics_sampler_release(PulseAppId app, pulse_graphics_sampler_ref* sampler_ref) {
    pulse_asset_ref ref{ pulse_graphics_sampler_to_handle(sampler_ref->handle), nullptr };
    pulse_asset_release(app, &ref);
    sampler_ref->handle = {};
    sampler_ref->ptr = nullptr;
}

} // extern "C"
