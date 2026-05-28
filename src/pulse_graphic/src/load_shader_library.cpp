#include "graphic_internal.h"

namespace pulse_graphic_internal {

static pulse_asset_loader_status_t step_shader_library_load(
    void*, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader library: no device"; return PULSE_ASSET_LOADER_FAILED; }
    if (ctx->byte_size == 0 || !ctx->bytes) {
        *out_error = "shader library: no data";
        return PULSE_ASSET_LOADER_FAILED;
    }

    CGPUShaderLibraryDescriptor desc = {};
    desc.name = ctx->path;
    desc.code_size = ctx->byte_size;
    desc.p_codes = ctx->bytes;
    auto* lib = cgpu_device_create_shader_library(device, &desc);
    if (!lib) { *out_error = "shader library: create failed"; return PULSE_ASSET_LOADER_FAILED; }

    auto* data = static_cast<pulse_shader_library_data_t*>(ctx->out_asset);
    data->library = lib;
    return PULSE_ASSET_LOADER_DONE;
}

void register_shader_library_load_loader(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld{};
    ld.struct_size = sizeof(pulse_asset_loader_desc);
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
    pulse_asset_register_loader(app, &ld);
}

} // namespace pulse_graphic_internal

using namespace pulse_graphic_internal;

extern "C" {

pulse_shader_library_t pulse_graphic_shader_library_load(
    pulse_app_t app,
    const pulse_graphics_shader_library_load_desc* desc)
{
    pulse_shader_library_t result{};
    if (!desc || !desc->path) return result;

    pulse_asset_handle h = asset_load_path(app, PULSE_TYPE_SHADER_LIBRARY, desc->path);
    if (!pulse_asset_handle_is_valid(h)) return result;
    result.index = h.index;
    result.generation = h.generation;
    return result;
}

} // extern "C"
