#include "../graphics_internal.h"
#include "pulse_datalist.h"

namespace pulse_graphics_internal {

struct ComputeShaderFileLoadState {
    bool lib_requested = false;
};

static EPulseAssetLoaderStatus step_compute_shader_from_file(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<ComputeShaderFileLoadState*>(state);

    if (!s->lib_requested) {
        if (ctx->bytes_size == 0 || !ctx->p_bytes) {
            *out_error = "compute shader file loader: no data";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        PulseDatalist* dl = pulse_datalist_create_from_text(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
        if (!dl) {
            *out_error = pulse_datalist_last_error();
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        const char* cs = pulse_datalist_get_string(dl, "cs", nullptr);
        if (!cs) {
            pulse_datalist_release(dl);
            *out_error = "compute shader file loader: missing 'cs' path";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        PulseShaderLibraryLoadDesc cs_desc = { cs };
        PulseShaderLibraryRequest cs_request = pulse_load_shader_library(ctx->app, &cs_desc);
        pulse_datalist_release(dl);

        auto asset_request = pulse_shader_library_request_to_asset_request(cs_request);
        if (!pulse_asset_request_is_valid(asset_request)) {
            *out_error = "compute shader file loader: failed to request shader library";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, asset_request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED);

        s->lib_requested = true;
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
    }

    return build_compute_shader_pipeline(ctx, out_error);
}

void register_compute_shader_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_COMPUTE_SHADER;
    ld.extensions = "cshader";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_compute_shader_from_file;
    ld.loader_size = sizeof(ComputeShaderFileLoadState);
    ld.loader_align = alignof(ComputeShaderFileLoadState);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseComputeShaderRequest pulse_load_compute_shader(PulseAppId app, const char* filepath)
{
    PulseComputeShaderRequest result{};
    if (!app || !filepath || !filepath[0]) return result;

    PulseAssetSystemId as = asset_system_from_app(app);
    PulseAssetRequest request = asset_load_path(as, PULSE_TYPE_COMPUTE_SHADER, filepath);
    if (!pulse_asset_request_is_valid(request)) return result;
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

} // extern "C"
