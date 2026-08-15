#include "../graphics_internal.h"

#include "renderer.h"

namespace pulse_graphics_internal {

EPulseAssetLoaderStatus step_sampler_create(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);

    auto create_desc = static_cast<const PulseSamplerCreateDesc*>(ctx->settings);
    CGPUSamplerId sampler = cgpu_device_create_sampler(device, &create_desc->desc);
    if (!sampler) return PULSE_ASSET_LOADER_STATUS_FAILED;

    auto* sam = static_cast<PulseSamplerData*>(ctx->out_asset);
    sam->handle = sampler;

    return PULSE_ASSET_LOADER_STATUS_DONE;;
}

void register_sampler_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SAMPLER;
    ld.extensions = "";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_sampler_create;
    ld.loader_size = 0;
    ld.loader_align = 0;
    ld.settings_size = sizeof(PulseSamplerCreateDesc);
    ld.settings_align = alignof(PulseSamplerCreateDesc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

}

extern "C" {

PulseSamplerHandle pulse_create_sampler(
    PulseAppId app,
    const PulseSamplerCreateDesc* desc)
{
    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device || !desc) return {};

    PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
    PulseAssetHandle h = pulse_graphics_internal::asset_build_sync(
        as, PULSE_TYPE_SAMPLER, nullptr, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(h))
        return {};

    return { h.index, h.generation };
}

}
