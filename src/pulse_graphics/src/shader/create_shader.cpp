#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

// ── Shared state for dependency-based shader loaders ───────────
struct ShaderLoaderState {
    bool from_file;
};

// ── Shared ctor for dependency-based shader loaders ────────────
pulse_result_t ctor_shader_from_deps(void* state, const pulse_asset_load_task* ctx) {
    auto* s = static_cast<ShaderLoaderState*>(state);
    s->from_file = ctx->dependency_count > 0;
    return PULSE_OK;
}

// ── Loader: shader from shader_library dependencies (create_from_file) ─
static pulse_asset_loader_status_t step_shader_from_deps(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* sls = static_cast<ShaderLoaderState*>(state);
    if (!sls->from_file) {
        return PULSE_ASSET_LOADER_DONE;
    }
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader loader: no device"; return PULSE_ASSET_LOADER_FAILED; }

    pulse_asset_ref vs_ref{}, fs_ref{};
    if (ctx->dependency_count < 2 ||
        !pulse_asset_acquire(ctx->app, ctx->dependencies[0].handle, &vs_ref) ||
        !pulse_asset_acquire(ctx->app, ctx->dependencies[1].handle, &fs_ref))
    {
        *out_error = "shader loader: missing deps";
        return PULSE_ASSET_LOADER_FAILED;
    }
    auto* vs_lib_data = static_cast<pulse_shader_library_data_t*>(vs_ref.ptr);
    auto* fs_lib_data = static_cast<pulse_shader_library_data_t*>(fs_ref.ptr);

    CGPUShaderLibraryId vs_lib = vs_lib_data->library;
    CGPUShaderLibraryId fs_lib = fs_lib_data->library;
    vs_lib_data->library = CGPU_NULLPTR;
    fs_lib_data->library = CGPU_NULLPTR;

    auto* desc = static_cast<const pulse_graphics_shader_create_from_file_desc*>(ctx->settings);

    auto cpp_shader = HGEGraphics::create_shader_from_libraries(
        device, vs_lib, fs_lib,
        desc->blend_desc, desc->depth_desc, desc->rasterizer_state);

    pulse_asset_release(ctx->app, &vs_ref);
    pulse_asset_release(ctx->app, &fs_ref);

    if (!cpp_shader) { *out_error = "shader loader: create_shader failed"; return PULSE_ASSET_LOADER_FAILED; }

    auto* data = static_cast<pulse_shader_data_t*>(ctx->out_asset);
    data->root_sig = cpp_shader->root_sig;
    data->vs = cpp_shader->vs;
    data->ps = cpp_shader->ps;
    data->blend_desc = cpp_shader->blend_desc;
    data->depth_desc = cpp_shader->depth_desc;
    data->rasterizer_state = cpp_shader->rasterizer_state;
    cpp_shader->root_sig = CGPU_NULLPTR;
    cpp_shader->vs.library = CGPU_NULLPTR;
    cpp_shader->ps.library = CGPU_NULLPTR;

    return PULSE_ASSET_LOADER_DONE;
}

// ── Loader: shader from binary (create_from_binary) ────────────
struct ShaderCreateFromBinaryState {
    bool initialized = false;
};

static pulse_asset_loader_status_t step_shader_create_from_binary(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<ShaderCreateFromBinaryState*>(state);

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        if (!device) { *out_error = "shader create: no device"; return PULSE_ASSET_LOADER_FAILED; }

        auto* desc = static_cast<const pulse_graphics_shader_create_from_binary_desc*>(ctx->settings);

        auto cpp_shader = HGEGraphics::create_shader(
            device,
            static_cast<const uint8_t*>(desc->vs_data), desc->vs_size,
            static_cast<const uint8_t*>(desc->fs_data), desc->fs_size,
            desc->blend_desc, desc->depth_desc, desc->rasterizer_state);
        if (!cpp_shader) { *out_error = "shader create: create_shader failed"; return PULSE_ASSET_LOADER_FAILED; }

        auto* data = static_cast<pulse_shader_data_t*>(ctx->out_asset);
        data->root_sig = cpp_shader->root_sig;
        data->vs = cpp_shader->vs;
        data->ps = cpp_shader->ps;
        data->blend_desc = cpp_shader->blend_desc;
        data->depth_desc = cpp_shader->depth_desc;
        data->rasterizer_state = cpp_shader->rasterizer_state;
        cpp_shader->root_sig = CGPU_NULLPTR;
        cpp_shader->vs.library = CGPU_NULLPTR;
        cpp_shader->ps.library = CGPU_NULLPTR;

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_DONE;
}

// ── Register all shader loaders ────────────────────────────────
void register_shader_create_loaders(pulse_app_t app, CGPUDeviceId device)
{
    // Loader: shader from dependencies (create_from_file)
    {
        pulse_asset_loader_desc ld{};
        ld.struct_size = sizeof(pulse_asset_loader_desc);
        ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
        ld.type_id = PULSE_TYPE_SHADER;
        ld.extensions = nullptr;
        ld.ctor = ctor_shader_from_deps;
        ld.dtor = nullptr;
        ld.step = step_shader_from_deps;
        ld.loader_size = sizeof(ShaderLoaderState);
        ld.loader_align = alignof(ShaderLoaderState);
        ld.settings_size = sizeof(pulse_graphics_shader_create_from_file_desc);
        ld.settings_align = alignof(pulse_graphics_shader_create_from_file_desc);
        ld.user_data = const_cast<struct CGPUDevice*>(device);
        pulse_asset_register_loader(app, &ld);
    }

    // Loader: shader from binary
    {
        pulse_asset_loader_desc ld{};
        ld.struct_size = sizeof(pulse_asset_loader_desc);
        ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
        ld.type_id = PULSE_TYPE_SHADER;
        ld.extensions = nullptr;
        ld.ctor = nullptr;
        ld.dtor = nullptr;
        ld.step = step_shader_create_from_binary;
        ld.loader_size = sizeof(ShaderCreateFromBinaryState);
        ld.loader_align = alignof(ShaderCreateFromBinaryState);
        ld.settings_size = sizeof(pulse_graphics_shader_create_from_binary_desc);
        ld.settings_align = alignof(pulse_graphics_shader_create_from_binary_desc);
        ld.user_data = const_cast<struct CGPUDevice*>(device);
        pulse_asset_register_loader(app, &ld);
    }
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

pulse_shader_t pulse_graphics_shader_create_from_binary(
    pulse_app_t app,
    const pulse_graphics_shader_create_from_binary_desc* desc)
{
    pulse_shader_t result{};
    if (!desc) return result;

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device) return result;

    pulse_asset_handle h = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_SHADER, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(h)) return result;
    result.index = h.index;
    result.generation = h.generation;
    return result;
}

pulse_shader_t pulse_graphics_shader_create_from_file(
    pulse_app_t app,
    const pulse_graphics_shader_create_from_file_desc* desc)
{
    pulse_shader_t result{};
    if (!desc || !desc->vert_path || !desc->frag_path) return result;

    pulse_asset_handle vs = asset_load_path(app, PULSE_TYPE_SHADER_LIBRARY, desc->vert_path);
    if (!pulse_asset_handle_is_valid(vs)) return result;
    pulse_asset_handle fs = asset_load_path(app, PULSE_TYPE_SHADER_LIBRARY, desc->frag_path);
    if (!pulse_asset_handle_is_valid(fs)) return result;
    pulse_asset_dependency deps[] = {
        { vs, PULSE_DEP_REQUIRED },
        { fs, PULSE_DEP_REQUIRED },
    };
    pulse_asset_handle h = asset_build(app, PULSE_TYPE_SHADER, nullptr, deps, 2, desc);
    if (!pulse_asset_handle_is_valid(h)) return result;
    result.index = h.index;
    result.generation = h.generation;
    return result;
}

} // extern "C"
