#include "../graphics_internal.h"

namespace pulse_graphics_internal {

static EPulseAssetLoaderStatus step_shader_library_load(
    void*, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader library: no device"; return PULSE_ASSET_LOADER_STATUS_FAILED; }
    if (ctx->bytes_size == 0 || !ctx->p_bytes) {
        *out_error = "shader library: no data";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    CGPUShaderLibraryDescriptor desc = {};
    desc.name = ctx->path;
    desc.code_size = static_cast<uint32_t>(ctx->bytes_size);
    desc.p_codes = static_cast<const uint8_t*>(ctx->p_bytes);
    auto* lib = cgpu_device_create_shader_library(device, &desc);
    if (!lib) { *out_error = "shader library: create failed"; return PULSE_ASSET_LOADER_STATUS_FAILED; }

    auto* data = static_cast<PulseShaderLibraryData*>(ctx->out_asset);
    data->library = lib;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_shader_library_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SHADER_LIBRARY;
    ld.extensions = "spv";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_shader_library_load;
    ld.loader_size = 0;
    ld.loader_align = 0;
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseShaderLibraryRequest pulse_load_shader_library(
    PulseAppId app,
    const PulseShaderLibraryLoadDesc* desc)
{
    PulseShaderLibraryRequest result{};
    if (!desc || !desc->path) return result;

    PulseAssetSystemId as = asset_system_from_app(app);
    PulseAssetRequest request = asset_load_path(as, PULSE_TYPE_SHADER_LIBRARY, desc->path);
    if (!pulse_asset_request_is_valid(request)) return result;
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

} // extern "C"
