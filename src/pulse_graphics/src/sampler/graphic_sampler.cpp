#include "../graphics_internal.h"

extern "C" {

pulse_sampler_t pulse_graphics_sampler_create(
    pulse_app_t app,
    const CGPUSamplerDescriptor* desc)
{
    pulse_sampler_t result{};
    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device || !desc) return result;

    CGPUSamplerId sampler = cgpu_device_create_sampler(device, desc);
    if (!sampler) return result;

    pulse_asset_handle asset_handle = pulse_graphics_internal::asset_load_memory_path(
        app, PULSE_TYPE_SAMPLER, "", nullptr, 0);
    if (!pulse_asset_handle_is_valid(asset_handle)) return result;

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_sampler_data_t* smp = static_cast<pulse_sampler_data_t*>(ref.ptr);
        smp->handle = sampler;
        pulse_asset_release(app, &ref);
    }

    result.asset = asset_handle;
    return result;
}

pulse_sampler_data_t* pulse_graphics_sampler_acquire(pulse_app_t app, pulse_sampler_t* handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, handle->asset, &ref)) {
        return static_cast<pulse_sampler_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphics_sampler_release(pulse_app_t app, pulse_sampler_t* handle) {
    pulse_asset_ref ref{handle->asset, nullptr};
    pulse_asset_release(app, &ref);
}

} // extern "C"
