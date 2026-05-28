#include "graphic_internal.h"
#include "renderer.h"

namespace pulse_graphic_internal {

// ── Shared state for dependency-based compute shader loaders ───
struct ShaderLoaderState {
    bool from_file;
};

// ── Loader: compute shader from shader_library dependency (create_from_file) ─
static pulse_asset_loader_status_t step_compute_shader_from_deps(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* sls = static_cast<ShaderLoaderState*>(state);
    if (!sls->from_file) { return PULSE_ASSET_LOADER_DONE; }
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "cs loader: no device"; return PULSE_ASSET_LOADER_FAILED; }

    pulse_asset_ref cs_ref{};
    if (ctx->dependency_count < 1 ||
        !pulse_asset_acquire(ctx->app, ctx->dependencies[0].handle, &cs_ref))
    {
        *out_error = "cs loader: missing dep";
        return PULSE_ASSET_LOADER_FAILED;
    }
    auto* cs_lib_data = static_cast<pulse_shader_library_data_t*>(cs_ref.ptr);

    CGPUShaderLibraryId cs_lib = cs_lib_data->library;
    cs_lib_data->library = CGPU_NULLPTR;

    auto cpp_cs = HGEGraphics::create_compute_shader_from_library(device, cs_lib);
    pulse_asset_release(ctx->app, &cs_ref);
    if (!cpp_cs) { *out_error = "cs loader: create_compute_shader failed"; return PULSE_ASSET_LOADER_FAILED; }

    auto* data = static_cast<pulse_compute_shader_data_t*>(ctx->out_asset);
    data->root_sig = cpp_cs->root_sig;
    data->cs = cpp_cs->cs;
    cpp_cs->root_sig = CGPU_NULLPTR;
    cpp_cs->cs.library = CGPU_NULLPTR;
    return PULSE_ASSET_LOADER_DONE;
}

// ── Loader: compute shader from binary (create_from_binary) ────
struct ComputeShaderCreateFromBinaryState {
    bool initialized = false;
};

static pulse_asset_loader_status_t step_compute_shader_create_from_binary(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<ComputeShaderCreateFromBinaryState*>(state);

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        if (!device) { *out_error = "cs create: no device"; return PULSE_ASSET_LOADER_FAILED; }

        auto* desc = static_cast<const pulse_graphics_compute_shader_create_from_binary_desc*>(ctx->settings);

        auto cpp_cs = HGEGraphics::create_compute_shader(
            device,
            static_cast<const uint8_t*>(desc->cs_data), desc->cs_size);
        if (!cpp_cs) { *out_error = "cs create: create_compute_shader failed"; return PULSE_ASSET_LOADER_FAILED; }

        auto* data = static_cast<pulse_compute_shader_data_t*>(ctx->out_asset);
        data->root_sig = cpp_cs->root_sig;
        data->cs = cpp_cs->cs;
        cpp_cs->root_sig = CGPU_NULLPTR;
        cpp_cs->cs.library = CGPU_NULLPTR;

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_DONE;
}

// ── Register all compute shader loaders ────────────────────────
void register_compute_shader_create_loaders(pulse_app_t app, CGPUDeviceId device)
{
    // Loader: compute shader from dependency (create_from_file)
    {
        pulse_asset_loader_desc ld{};
        ld.struct_size = sizeof(pulse_asset_loader_desc);
        ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
        ld.type_id = PULSE_TYPE_COMPUTE_SHADER;
        ld.extensions = nullptr;
        ld.ctor = ctor_shader_from_deps;
        ld.dtor = nullptr;
        ld.step = step_compute_shader_from_deps;
        ld.loader_size = sizeof(ShaderLoaderState);
        ld.loader_align = alignof(ShaderLoaderState);
        ld.settings_size = 0;
        ld.settings_align = 0;
        ld.user_data = const_cast<struct CGPUDevice*>(device);
        pulse_asset_register_loader(app, &ld);
    }

    // Loader: compute shader from binary
    {
        pulse_asset_loader_desc ld{};
        ld.struct_size = sizeof(pulse_asset_loader_desc);
        ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
        ld.type_id = PULSE_TYPE_COMPUTE_SHADER;
        ld.extensions = nullptr;
        ld.ctor = nullptr;
        ld.dtor = nullptr;
        ld.step = step_compute_shader_create_from_binary;
        ld.loader_size = sizeof(ComputeShaderCreateFromBinaryState);
        ld.loader_align = alignof(ComputeShaderCreateFromBinaryState);
        ld.settings_size = sizeof(pulse_graphics_compute_shader_create_from_binary_desc);
        ld.settings_align = alignof(pulse_graphics_compute_shader_create_from_binary_desc);
        ld.user_data = const_cast<struct CGPUDevice*>(device);
        pulse_asset_register_loader(app, &ld);
    }
}

} // namespace pulse_graphic_internal

using namespace pulse_graphic_internal;

extern "C" {

pulse_compute_shader_t pulse_graphic_compute_shader_create_from_binary(
    pulse_app_t app,
    const pulse_graphics_compute_shader_create_from_binary_desc* desc)
{
    pulse_compute_shader_t result{};
    if (!desc) return result;

    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device) return result;

    pulse_asset_handle h = pulse_graphic_internal::asset_build(
        app, PULSE_TYPE_COMPUTE_SHADER, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(h)) return result;
    result.index = h.index;
    result.generation = h.generation;
    return result;
}

pulse_compute_shader_t pulse_graphic_compute_shader_create_from_file(
    pulse_app_t app,
    const pulse_graphics_compute_shader_create_from_file_desc* desc)
{
    pulse_compute_shader_t result{};
    if (!desc || !desc->comp_path) return result;

    pulse_asset_handle cs = asset_load_path(app, PULSE_TYPE_SHADER_LIBRARY, desc->comp_path);
    if (!pulse_asset_handle_is_valid(cs)) return result;
    pulse_asset_dependency deps[] = {
        { cs, PULSE_DEP_REQUIRED },
    };
    pulse_asset_handle h = asset_build(app, PULSE_TYPE_COMPUTE_SHADER, nullptr, deps, 1);
    if (!pulse_asset_handle_is_valid(h)) return result;
    result.index = h.index;
    result.generation = h.generation;
    return result;
}

} // extern "C"
