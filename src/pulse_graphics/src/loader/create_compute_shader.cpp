#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

struct ShaderLoaderState {
    bool csPrepared = false;
};

static pulse_asset_loader_status_t step_compute_shader_from_deps(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<ShaderLoaderState*>(state);

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader loader: no device"; return PULSE_ASSET_LOADER_FAILED; }

    auto csHandle = ctx->dependencies[0].handle;
    pulse_shader_library_t cs = { csHandle.index, csHandle.generation };
    if (!s->csPrepared) {
        auto csState = pulse_asset_get_state(ctx->app, csHandle);
        if (csState == PULSE_ASSET_STATE_LOADED)
            s->csPrepared = true;
        else if (csState == PULSE_ASSET_STATE_FAILED || csState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "shader create loader: failed to wait compute shader";
            return PULSE_ASSET_LOADER_FAILED;
        }
        else
            s->csPrepared = false;
    }

    if (!s->csPrepared)
        return PULSE_ASSET_LOADER_WAIT_DEPENDENCIES;

    pulse_graphics_shader_library_ref cs_ref{};
    if (!pulse_graphics_shader_library_acquire(ctx->app, cs, &cs_ref)) {
        *out_error = "shader create loader: failed to acquire compute shader library";
        return PULSE_ASSET_LOADER_FAILED;
    }

    CGPUShaderLibraryId cs_lib = cs_ref.ptr->library;

    CGPUShaderEntryDescriptor ppl_shaders[1];
    ppl_shaders[0].stage = CGPU_SHADER_STAGE_COMPUTE;
    ppl_shaders[0].entry = "main";
    ppl_shaders[0].library = cs_lib;
    CGPURootSignatureDescriptor rs_desc = {
        .shader_count = 1,
        .p_shaders = ppl_shaders,
    };
    auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

    auto* data = static_cast<pulse_compute_shader_data_t*>(ctx->out_asset);
    data->root_sig = root_sig;
    data->cs = ppl_shaders[0];

    pulse_graphics_shader_library_release(ctx->app, &cs_ref);

    return PULSE_ASSET_LOADER_DONE;
}

void register_compute_shader_create_loaders(PulseAppId app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld{};
    ld.struct_size = sizeof(pulse_asset_loader_desc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_COMPUTE_SHADER;
    ld.extensions = nullptr;
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_compute_shader_from_deps;
    ld.loader_size = sizeof(ShaderLoaderState);
    ld.loader_align = alignof(ShaderLoaderState);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

pulse_compute_shader_t pulse_graphics_compute_shader_create_from_binary(
    PulseAppId app,
    const pulse_graphics_compute_shader_create_from_binary_desc* desc)
{
    if (!desc || !desc->cs_data || !desc->cs_size) return {};

    pulse_graphics_shader_library_create_desc cs_desc = {
        desc->cs_data,
        desc->cs_size
    };

    auto cs = pulse_graphics_shader_library_create(app, &cs_desc);
    if (!pulse_graphics_shader_library_is_alive(app, cs)) return {};
    pulse_asset_dependency deps[] = {
        { pulse_graphics_shader_library_to_handle(cs), PULSE_DEP_REQUIRED },
    };
    pulse_asset_handle h = asset_build(app, PULSE_TYPE_COMPUTE_SHADER, nullptr, deps, 1, desc);
    if (!pulse_asset_handle_is_valid(h)) {
        pulse_graphics_shader_library_unload(app, cs);
        return {};
    }
    return { h.index, h.generation };
}

pulse_compute_shader_t pulse_graphics_compute_shader_create_from_file(
    PulseAppId app,
    const pulse_graphics_compute_shader_create_from_file_desc* desc)
{
    if (!desc || !desc->cs_path) return {};

    pulse_graphics_shader_library_load_desc cs_desc = {
        desc->cs_path
    };

    auto cs = pulse_graphics_shader_library_load(app, &cs_desc);
    if (!pulse_graphics_shader_library_is_alive(app, cs)) return {};
    pulse_asset_dependency deps[] = {
        { pulse_graphics_shader_library_to_handle(cs), PULSE_DEP_REQUIRED },
    };
    pulse_asset_handle h = asset_build(app, PULSE_TYPE_COMPUTE_SHADER, nullptr, deps, 1, desc);
    if (!pulse_asset_handle_is_valid(h)) {
        pulse_graphics_shader_library_unload(app, cs);
        return {};
    }
    return { h.index, h.generation };
}

} // extern "C"
