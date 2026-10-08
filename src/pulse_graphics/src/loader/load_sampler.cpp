#include "../graphics_internal.h"

namespace pulse_graphics_internal {

static void parse_sampler_descriptor(const PulseDatalist* dl, CGPUSamplerDescriptor* out) {
    static const NamedValue filters[] = {
        { "nearest", CGPU_FILTER_TYPE_NEAREST },
        { "linear",  CGPU_FILTER_TYPE_LINEAR },
    };
    static const NamedValue mipmap_modes[] = {
        { "nearest", CGPU_MIP_MAP_MODE_NEAREST },
        { "linear",  CGPU_MIP_MAP_MODE_LINEAR },
    };
    static const NamedValue address_modes[] = {
        { "mirror",          CGPU_ADDRESS_MODE_MIRROR },
        { "repeat",          CGPU_ADDRESS_MODE_REPEAT },
        { "clamp_to_edge",   CGPU_ADDRESS_MODE_CLAMP_TO_EDGE },
        { "clamp_to_border", CGPU_ADDRESS_MODE_CLAMP_TO_BORDER },
    };
    static const NamedValue compare_ops[] = {
        { "never",         CGPU_COMPARE_OP_NEVER },
        { "less",          CGPU_COMPARE_OP_LESS },
        { "equal",         CGPU_COMPARE_OP_EQUAL },
        { "less_equal",    CGPU_COMPARE_OP_LESS_EQUAL },
        { "greater",       CGPU_COMPARE_OP_GREATER },
        { "not_equal",     CGPU_COMPARE_OP_NOT_EQUAL },
        { "greater_equal", CGPU_COMPARE_OP_GREATER_EQUAL },
        { "always",        CGPU_COMPARE_OP_ALWAYS },
    };

    CGPUSamplerDescriptor desc = {};
    desc.min_filter = (ECGPUFilterType)enum_from_string(dl, "min_filter", filters, sizeof(filters) / sizeof(filters[0]), CGPU_FILTER_TYPE_LINEAR);
    desc.mag_filter = (ECGPUFilterType)enum_from_string(dl, "mag_filter", filters, sizeof(filters) / sizeof(filters[0]), CGPU_FILTER_TYPE_LINEAR);
    desc.mipmap_mode = (ECGPUMipMapMode)enum_from_string(dl, "mipmap_mode", mipmap_modes, sizeof(mipmap_modes) / sizeof(mipmap_modes[0]), CGPU_MIP_MAP_MODE_LINEAR);
    desc.address_u = (ECGPUAddressMode)enum_from_string(dl, "address_u", address_modes, sizeof(address_modes) / sizeof(address_modes[0]), CGPU_ADDRESS_MODE_CLAMP_TO_EDGE);
    desc.address_v = (ECGPUAddressMode)enum_from_string(dl, "address_v", address_modes, sizeof(address_modes) / sizeof(address_modes[0]), CGPU_ADDRESS_MODE_CLAMP_TO_EDGE);
    desc.address_w = (ECGPUAddressMode)enum_from_string(dl, "address_w", address_modes, sizeof(address_modes) / sizeof(address_modes[0]), CGPU_ADDRESS_MODE_CLAMP_TO_EDGE);
    desc.mip_lod_bias = (float)pulse_datalist_get_double(dl, "mip_lod_bias", 0.0);
    desc.max_anisotropy = (float)pulse_datalist_get_double(dl, "max_anisotropy", 1.0);
    desc.compare_func = (ECGPUCompareOp)enum_from_string(dl, "compare_func", compare_ops, sizeof(compare_ops) / sizeof(compare_ops[0]), CGPU_COMPARE_OP_NEVER);
    *out = desc;
}

EPulseAssetLoaderStatus step_sampler_from_file(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    PulseDatalist* dl = parse_datalist_bytes(ctx, out_error);
    if (!dl)
        return PULSE_ASSET_LOADER_STATUS_FAILED;

    CGPUSamplerDescriptor desc = {};
    parse_sampler_descriptor(dl, &desc);
    pulse_datalist_release(dl);

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    CGPUSamplerId sampler = cgpu_device_create_sampler(device, &desc);
    if (!sampler) {
        *out_error = "sampler file loader: sampler creation failed";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    auto* sam = static_cast<PulseSamplerData*>(ctx->out_asset);
    sam->handle = sampler;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_sampler_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SAMPLER;
    ld.extensions = "sampler";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_sampler_from_file;
    ld.loader_size = 0;
    ld.loader_align = 0;
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

}

using namespace pulse_graphics_internal;

extern "C" {

PulseSamplerRequest pulse_load_sampler(PulseAppId app, const char* filepath)
{
    PulseSamplerRequest result{};
    if (!app || !filepath || !filepath[0]) return result;

    PulseAssetSystemId as = asset_system_from_app(app);
    PulseAssetRequest request = asset_load_path(as, PULSE_TYPE_SAMPLER, filepath);
    if (!pulse_asset_request_is_valid(request)) return result;
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

} // extern "C"
