#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

struct ShaderLoaderState {
    bool csPrepared = false;
};

static EPulseAssetLoaderStatus step_compute_shader_from_deps(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<ShaderLoaderState*>(state);

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader loader: no device"; return PULSE_ASSET_LOADER_STATUS_FAILED; }

    auto csHandle = ctx->dependencies[0].handle;
    PulseShaderLibraryHandle cs = { csHandle.index, csHandle.generation };
    if (!s->csPrepared) {
        auto csState = pulse_asset_system_get_state(ctx->asset_system, csHandle);
        if (csState == PULSE_ASSET_STATE_LOADED)
            s->csPrepared = true;
        else if (csState == PULSE_ASSET_STATE_FAILED || csState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "shader create loader: failed to wait compute shader";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        else
            s->csPrepared = false;
    }

    if (!s->csPrepared)
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;

    PulseShaderLibrary cs_ref{};
    if (!internal_acquire_shader_library(ctx->asset_system, cs, &cs_ref)) {
        *out_error = "shader create loader: failed to acquire compute shader library";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    CGPUShaderLibraryId cs_lib = static_cast<PulseShaderLibraryData*>(cs_ref.ptr)->library;

    CGPUShaderEntryDescriptor ppl_shaders[1];
    ppl_shaders[0].stage = CGPU_SHADER_STAGE_COMPUTE;
    ppl_shaders[0].entry = "main";
    ppl_shaders[0].library = cs_lib;
    CGPURootSignatureDescriptor rs_desc = {
        .shader_count = 1,
        .p_shaders = ppl_shaders,
    };
    auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

    auto* data = static_cast<PulseComputeShaderData*>(ctx->out_asset);
    data->root_sig = root_sig;
    data->cs = ppl_shaders[0];

    internal_release_shader_library(ctx->asset_system, &cs_ref);

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_compute_shader_create_loaders(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
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
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseComputeShaderHandle pulse_create_compute_shader_from_binary(
    PulseAppId app,
    const PulseComputeShaderCreateFromBinaryDesc* desc)
{
    if (!desc || !desc->cs_data || !desc->cs_size) return {};

    PulseShaderLibraryCreateDesc cs_desc = {
        desc->cs_data,
        desc->cs_size
    };

    auto cs = pulse_create_shader_library(app, &cs_desc);
    if (!pulse_shader_library_is_alive(app, cs)) return {};
    PulseAssetDependency deps[] = {
        { pulse_shader_library_to_handle(cs), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
    };
    PulseAssetHandle h = asset_build(app, PULSE_TYPE_COMPUTE_SHADER, nullptr, deps, 1, desc);
    if (!pulse_asset_handle_is_valid(h)) {
        pulse_unload_shader_library(app, cs);
        return {};
    }
    return { h.index, h.generation };
}

PulseComputeShaderHandle pulse_create_compute_shader_from_file(
    PulseAppId app,
    const PulseComputeShaderCreateFromFileDesc* desc)
{
    if (!desc || !desc->cs_path) return {};

    PulseShaderLibraryLoadDesc cs_desc = {
        desc->cs_path
    };

    auto cs = pulse_load_shader_library(app, &cs_desc);
    if (!pulse_shader_library_is_alive(app, cs)) return {};
    PulseAssetDependency deps[] = {
        { pulse_shader_library_to_handle(cs), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
    };
    PulseAssetHandle h = asset_build(app, PULSE_TYPE_COMPUTE_SHADER, nullptr, deps, 1, desc);
    if (!pulse_asset_handle_is_valid(h)) {
        pulse_unload_shader_library(app, cs);
        return {};
    }
    return { h.index, h.generation };
}

} // extern "C"
