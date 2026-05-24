#include "graphic_internal.h"

extern "C" {

pulse_sampler_t pulse_graphic_sampler_create(
    pulse_app_t app,
    const CGPUSamplerDescriptor* desc)
{
    pulse_sampler_t result{};
    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device || !desc) return result;

    CGPUSamplerId sampler = cgpu_device_create_sampler(device, desc);
    if (!sampler) return result;

    pulse_asset_handle asset_handle = pulse_asset_load_from_memory(
        app, PULSE_TYPE_SAMPLER, "", nullptr, 0, NULL);
    if (asset_handle.index == PULSE_ASSET_INVALID_INDEX) return result;

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_sampler_data_t* smp = static_cast<pulse_sampler_data_t*>(ref.ptr);
        smp->handle = sampler;
        pulse_asset_release(app, &ref);
    }

    result.asset = asset_handle;
    return result;
}

pulse_sampler_data_t* pulse_graphic_sampler_acquire(pulse_app_t app, pulse_sampler_t* handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, handle->asset, &ref)) {
        return static_cast<pulse_sampler_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphic_sampler_release(pulse_app_t app, pulse_sampler_t* handle) {
    pulse_asset_ref ref{handle->asset, nullptr};
    pulse_asset_release(app, &ref);
}

} // extern "C"
