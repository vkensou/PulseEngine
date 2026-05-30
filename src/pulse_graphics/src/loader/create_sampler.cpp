#include "../graphics_internal.h"

#include "renderer.h"

namespace pulse_graphics_internal {

pulse_asset_loader_status_t step_sampler_create(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);

    auto create_desc = static_cast<const pulse_graphics_sampler_create_desc*>(ctx->settings);
    CGPUSamplerId sampler = cgpu_device_create_sampler(device, &create_desc->desc);
    if (!sampler) return PULSE_ASSET_LOADER_FAILED;

    auto* sam = static_cast<pulse_sampler_data_t*>(ctx->out_asset);
    sam->handle = sampler;

    return PULSE_ASSET_LOADER_DONE;;
}

void register_sampler_create_loader(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld{};
    ld.struct_size = sizeof(pulse_asset_loader_desc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SAMPLER;
    ld.extensions = "";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_sampler_create;
    ld.loader_size = 0;
    ld.loader_align = 0;
    ld.settings_size = sizeof(pulse_graphics_sampler_create_desc);
    ld.settings_align = alignof(pulse_graphics_sampler_create_desc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld);
}

}

extern "C" {

pulse_sampler_t pulse_graphics_sampler_create(
    pulse_app_t app,
    const pulse_graphics_sampler_create_desc* desc)
{
    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device || !desc) return {};


    pulse_asset_handle asset_handle = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_SAMPLER, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(asset_handle)) 
    return {};

    return { asset_handle.index, asset_handle.generation };
}

}
