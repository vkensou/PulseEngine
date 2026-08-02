#include "../graphics_internal.h"

namespace pulse_graphics_internal {

struct ShaderLibraryCreateState {
    bool initialized = false;
};

static EPulseAssetLoaderStatus step_shader_library_create(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<ShaderLibraryCreateState*>(state);

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        if (!device) { *out_error = "shader library create: no device"; return PULSE_ASSET_LOADER_STATUS_FAILED; }

        auto* desc = static_cast<const PulseShaderLibraryCreateDesc*>(ctx->settings);
        if (!desc->code || desc->code_size == 0) {
            *out_error = "shader library create: no data";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        CGPUShaderLibraryDescriptor lib_desc = {};
        lib_desc.name = "shader_library";
        lib_desc.code_size = desc->code_size;
        lib_desc.p_codes = static_cast<const uint8_t*>(desc->code);
        auto* lib = cgpu_device_create_shader_library(device, &lib_desc);
        if (!lib) { *out_error = "shader library create: create failed"; return PULSE_ASSET_LOADER_STATUS_FAILED; }

        auto* data = static_cast<PulseShaderLibraryData*>(ctx->out_asset);
        data->library = lib;

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_shader_library_create_loader(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SHADER_LIBRARY;
    ld.extensions = nullptr;
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_shader_library_create;
    ld.loader_size = sizeof(ShaderLibraryCreateState);
    ld.loader_align = alignof(ShaderLibraryCreateState);
    ld.settings_size = sizeof(PulseShaderLibraryCreateDesc);
    ld.settings_align = alignof(PulseShaderLibraryCreateDesc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseShaderLibraryHandle pulse_create_shader_library(
    PulseAppId app,
    const PulseShaderLibraryCreateDesc* desc)
{
    PulseShaderLibraryHandle result{};
    if (!desc) return result;

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device) return result;

    PulseAssetHandle h = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_SHADER_LIBRARY, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(h)) return result;
    result.index = h.index;
    result.generation = h.generation;
    return result;
}

} // extern "C"
