#include "../graphics_internal.h"

namespace pulse_graphics_internal {

static EPulseAssetLoaderStatus step_shader_library_load(
    void*, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader library: no device"; return PULSE_ASSET_LOADER_STATUS_FAILED; }
    if (ctx->byte_size == 0 || !ctx->bytes) {
        *out_error = "shader library: no data";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    CGPUShaderLibraryDescriptor desc = {};
    desc.name = ctx->path;
    desc.code_size = ctx->byte_size;
    desc.p_codes = ctx->bytes;
    auto* lib = cgpu_device_create_shader_library(device, &desc);
    if (!lib) { *out_error = "shader library: create failed"; return PULSE_ASSET_LOADER_STATUS_FAILED; }

    auto* data = static_cast<PulseShaderLibraryData*>(ctx->out_asset);
    data->library = lib;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_shader_library_load_loader(PulseAppId app, CGPUDeviceId device)
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
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseShaderLibraryHandle pulse_load_shader_library(
    PulseAppId app,
    const PulseShaderLibraryLoadDesc* desc)
{
    PulseShaderLibraryHandle result{};
    if (!desc || !desc->path) return result;

    PulseAssetHandle h = asset_load_path(app, PULSE_TYPE_SHADER_LIBRARY, desc->path);
    if (!pulse_asset_handle_is_valid(h)) return result;
    result.index = h.index;
    result.generation = h.generation;
    return result;
}

bool pulse_acquire_shader_library(PulseAppId app, PulseShaderLibraryHandle handle, PulseShaderLibrary* sl_ref)
{
    PulseAssetRef aref{};
    if (pulse_asset_system_acquire(pulse_get_asset_system(app), pulse_shader_library_to_handle(handle), &aref)) {
        sl_ref->handle = handle;
        sl_ref->ptr = static_cast<PulseShaderLibraryData*>(aref.ptr);
        return true;
    }
    sl_ref->handle = {};
    sl_ref->ptr = nullptr;
    return false;
}

void pulse_release_shader_library(PulseAppId app, PulseShaderLibrary* sl_ref)
{
    PulseAssetRef aref{ pulse_shader_library_to_handle(sl_ref->handle), nullptr };
    pulse_asset_system_release(pulse_get_asset_system(app), &aref);
    sl_ref->handle = {};
    sl_ref->ptr = nullptr;
}

} // extern "C"
