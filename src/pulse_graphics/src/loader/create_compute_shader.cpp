#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

EPulseAssetLoaderStatus build_compute_shader_pipeline(const PulseAssetLoadTask* ctx, const char** out_error)
{
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader loader: no device"; return PULSE_ASSET_LOADER_STATUS_FAILED; }

    auto csRequest = pulse_asset_system_to_asset_request_from_dep_ref(ctx->asset_system, ctx->p_dependencies[0].dep_ref);
    PulseShaderLibraryHandle cs = pulse_shader_library_get_handle(ctx->app, { csRequest.index, csRequest.generation });
    PulseShaderLibraryData* cs_data = internal_borrow_shader_library(ctx->asset_system, cs);
    if (!cs_data) {
        *out_error = "shader create loader: failed to acquire compute shader library";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    CGPUShaderLibraryId cs_lib = cs_data->library;

    CGPUShaderEntryDescriptor ppl_shaders[1];
    ppl_shaders[0].stage = CGPU_SHADER_STAGE_COMPUTE;
    ppl_shaders[0].entry = "main";
    ppl_shaders[0].library = cs_lib;
    CGPURootSignatureDescriptor rs_desc = {
        .shader_count = 1,
        .p_shaders = ppl_shaders,
        .dynamic_buffers = true,
    };
    auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

    auto* data = static_cast<PulseComputeShaderData*>(ctx->out_asset);
    data->root_sig = root_sig;
    data->cs = ppl_shaders[0];

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseAssetLoaderStatus step_compute_shader_from_deps(
    void*, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    return build_compute_shader_pipeline(ctx, out_error);
}

void register_compute_shader_create_loaders(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_COMPUTE_SHADER;
    ld.extensions = nullptr;
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_compute_shader_from_deps;
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

PulseComputeShaderHandle pulse_create_compute_shader_from_binary(
    PulseAppId app,
    const PulseComputeShaderCreateFromBinaryDesc* desc)
{
    if (!desc || !desc->p_cs_data || !desc->cs_data_size) return {};

    PulseShaderLibraryCreateDesc cs_desc = {
        desc->p_cs_data,
        desc->cs_data_size
    };

    PulseAssetSystemId as = asset_system_from_app(app);
    auto cs = pulse_create_shader_library(app, &cs_desc);
    if (!pulse_asset_handle_is_valid(pulse_shader_library_to_handle(cs))) return {};
    PulseAssetDependency deps[] = {
        { pulse_asset_system_to_asset_dep_ref_from_handle(as, pulse_shader_library_to_handle(cs)), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
    };
    PulseAssetHandle h = asset_build_sync(as, PULSE_TYPE_COMPUTE_SHADER, nullptr, nullptr, deps, 1, desc);
    if (!pulse_asset_handle_is_valid(h)) {
        pulse_asset_system_release(as, pulse_shader_library_to_handle(cs), nullptr);
        return {};
    }
    return { h.index, h.generation };
}

} // extern "C"
