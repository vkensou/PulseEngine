#include "graphic_internal.h"

namespace pulse_graphic_internal {

struct ShaderLibraryCreateState {
    bool initialized = false;
};

static pulse_asset_loader_status_t step_shader_library_create(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<ShaderLibraryCreateState*>(state);

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        if (!device) { *out_error = "shader library create: no device"; return PULSE_ASSET_LOADER_FAILED; }

        auto* desc = static_cast<const pulse_graphics_shader_library_create_desc*>(ctx->settings);
        if (!desc->code || desc->code_size == 0) {
            *out_error = "shader library create: no data";
            return PULSE_ASSET_LOADER_FAILED;
        }

        CGPUShaderLibraryDescriptor lib_desc = {};
        lib_desc.name = "shader_library";
        lib_desc.code_size = desc->code_size;
        lib_desc.p_codes = static_cast<const uint8_t*>(desc->code);
        auto* lib = cgpu_device_create_shader_library(device, &lib_desc);
        if (!lib) { *out_error = "shader library create: create failed"; return PULSE_ASSET_LOADER_FAILED; }

        auto* data = static_cast<pulse_shader_library_data_t*>(ctx->out_asset);
        data->library = lib;

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_DONE;
}

void register_shader_library_create_loader(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld{};
    ld.struct_size = sizeof(pulse_asset_loader_desc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SHADER_LIBRARY;
    ld.extensions = nullptr;
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_shader_library_create;
    ld.loader_size = sizeof(ShaderLibraryCreateState);
    ld.loader_align = alignof(ShaderLibraryCreateState);
    ld.settings_size = sizeof(pulse_graphics_shader_library_create_desc);
    ld.settings_align = alignof(pulse_graphics_shader_library_create_desc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld);
}

} // namespace pulse_graphic_internal

using namespace pulse_graphic_internal;

extern "C" {

pulse_shader_library_t pulse_graphic_shader_library_create(
    pulse_app_t app,
    const pulse_graphics_shader_library_create_desc* desc)
{
    pulse_shader_library_t result{};
    if (!desc) return result;

    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device) return result;

    pulse_asset_handle h = pulse_graphic_internal::asset_build(
        app, PULSE_TYPE_SHADER_LIBRARY, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(h)) return result;
    result.index = h.index;
    result.generation = h.generation;
    return result;
}

} // extern "C"
