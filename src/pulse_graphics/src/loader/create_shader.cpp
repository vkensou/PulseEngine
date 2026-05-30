#include "../graphics_internal.h"

namespace pulse_graphics_internal {

struct ShaderCreateSettings {
    CGPUBlendStateDescriptor blend_desc;
    CGPUDepthStateDescriptor depth_desc;
    CGPURasterizerStateDescriptor rasterizer_state;
};

struct ShaderLoaderState {
    bool vsPrepared = false;
    bool psPrepared = false;
};

static pulse_asset_loader_status_t step_shader_from_deps(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<ShaderLoaderState*>(state);

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader loader: no device"; return PULSE_ASSET_LOADER_FAILED; }

    auto vsHandle = ctx->dependencies[0].handle;
    pulse_shader_library_t vs = { vsHandle.index, vsHandle.generation };
    if (!s->vsPrepared) {
        auto vsState = pulse_asset_get_state(ctx->app, vsHandle);
        if (vsState == PULSE_ASSET_STATE_LOADED)
            s->vsPrepared = true;
        else if (vsState == PULSE_ASSET_STATE_FAILED || vsState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "shader create loader: failed to wait vertex shader";
            return PULSE_ASSET_LOADER_FAILED;
        }
        else
            s->vsPrepared = false;
    }

    auto psHandle = ctx->dependencies[1].handle;
    pulse_shader_library_t ps = { psHandle.index, psHandle.generation };
    if (!s->psPrepared) {
        auto psState = pulse_asset_get_state(ctx->app, psHandle);
        if (psState == PULSE_ASSET_STATE_LOADED)
            s->psPrepared = true;
        else if (psState == PULSE_ASSET_STATE_FAILED || psState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "shader create loader: failed to wait fragment shader";
            return PULSE_ASSET_LOADER_FAILED;
        }
        else
            s->psPrepared = false;
    }

    if (!s->vsPrepared || !s->psPrepared)
        return PULSE_ASSET_LOADER_WAIT_DEPENDENCIES;

    pulse_graphics_shader_library_ref vs_ref{};
    if (!pulse_graphics_shader_library_acquire(ctx->app, vs, &vs_ref)) {
        *out_error = "shader create loader: failed to acquire vertex shader library";
        return PULSE_ASSET_LOADER_FAILED;
    }

    pulse_graphics_shader_library_ref ps_ref{};
    if (!pulse_graphics_shader_library_acquire(ctx->app, ps, &ps_ref)) {
        pulse_graphics_shader_library_release(ctx->app, &vs_ref);
        *out_error = "shader create loader: failed to acquire fragment shader library";
        return PULSE_ASSET_LOADER_FAILED;
    }

    CGPUShaderLibraryId vs_lib = vs_ref.ptr->library;
    CGPUShaderLibraryId fs_lib = ps_ref.ptr->library;

    auto* desc = static_cast<const ShaderCreateSettings*>(ctx->settings);

    auto* data = static_cast<pulse_shader_data_t*>(ctx->out_asset);

    CGPUShaderEntryDescriptor ppl_shaders[2];
    ppl_shaders[0].stage = CGPU_SHADER_STAGE_VERTEX;
    ppl_shaders[0].entry = "main";
    ppl_shaders[0].library = vs_lib;
    ppl_shaders[1].stage = CGPU_SHADER_STAGE_FRAGMENT;
    ppl_shaders[1].entry = "main";
    ppl_shaders[1].library = fs_lib;
    CGPURootSignatureDescriptor rs_desc = {
        .shader_count = 2,
        .p_shaders = ppl_shaders,
    };
    auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);

    data->root_sig = root_sig;
    data->vs = ppl_shaders[0];
    data->ps = ppl_shaders[1];
    data->blend_desc = desc->blend_desc;
    data->blend_attachment_states_count = desc->blend_desc.attachment_count;
    data->p_blend_attachment_states = new CGPUBlendAttachmentState[data->blend_attachment_states_count];
    std::copy(desc->blend_desc.p_attachments, desc->blend_desc.p_attachments + desc->blend_desc.attachment_count, data->p_blend_attachment_states);
    data->blend_desc.p_attachments = data->p_blend_attachment_states;

    data->depth_desc = desc->depth_desc;
    data->rasterizer_state = desc->rasterizer_state;

    pulse_graphics_shader_library_release(ctx->app, &vs_ref);
    pulse_graphics_shader_library_release(ctx->app, &ps_ref);

    return PULSE_ASSET_LOADER_DONE;
}

void register_shader_create_loaders(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld{};
    ld.struct_size = sizeof(pulse_asset_loader_desc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SHADER;
    ld.extensions = nullptr;
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_shader_from_deps;
    ld.loader_size = sizeof(ShaderLoaderState);
    ld.loader_align = alignof(ShaderLoaderState);
    ld.settings_size = sizeof(ShaderCreateSettings);
    ld.settings_align = alignof(ShaderCreateSettings);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

pulse_shader_t pulse_graphics_shader_create_from_binary(
    pulse_app_t app,
    const pulse_graphics_shader_create_from_binary_desc* desc)
{
    if (!desc || !desc->vs_data || !desc->vs_size || !desc->fs_data || !desc->fs_size) return {};

    pulse_graphics_shader_library_create_desc vs_desc = {
        desc->vs_data,
        desc->vs_size
    };
    pulse_graphics_shader_library_create_desc fs_desc = {
        desc->fs_data,
        desc->fs_size
    };

    auto vs = pulse_graphics_shader_library_create(app, &vs_desc);
    if (!pulse_graphics_shader_library_is_alive(app, vs)) return {};
    auto fs = pulse_graphics_shader_library_create(app, &fs_desc);
    if (!pulse_graphics_shader_library_is_alive(app, fs)) return {};
    pulse_asset_dependency deps[] = {
        { pulse_graphics_shader_library_to_handle(vs), PULSE_DEP_REQUIRED },
        { pulse_graphics_shader_library_to_handle(fs), PULSE_DEP_REQUIRED },
    };

	ShaderCreateSettings settings = {
        .blend_desc = desc->blend_desc,
        .depth_desc = desc->depth_desc,
        .rasterizer_state = desc->rasterizer_state,
    };

    pulse_asset_handle h = asset_build(app, PULSE_TYPE_SHADER, nullptr, deps, 2, &settings);
    if (!pulse_asset_handle_is_valid(h)) {
		pulse_graphics_shader_library_unload(app, vs);
		pulse_graphics_shader_library_unload(app, fs);
        return {};
    }
    return {h.index, h.generation};
}

pulse_shader_t pulse_graphics_shader_create_from_file(
    pulse_app_t app,
    const pulse_graphics_shader_create_from_file_desc* desc)
{
    if (!desc || !desc->vert_path || !desc->frag_path) return {};

    pulse_graphics_shader_library_load_desc vs_desc = {
        desc->vert_path
    };
    pulse_graphics_shader_library_load_desc fs_desc = {
        desc->frag_path
    };

    auto vs = pulse_graphics_shader_library_load(app, &vs_desc);
    if (!pulse_graphics_shader_library_is_alive(app, vs)) return {};
    auto fs = pulse_graphics_shader_library_load(app, &fs_desc);
    if (!pulse_graphics_shader_library_is_alive(app, fs)) return {};
    pulse_asset_dependency deps[] = {
        { pulse_graphics_shader_library_to_handle(vs), PULSE_DEP_REQUIRED },
        { pulse_graphics_shader_library_to_handle(fs), PULSE_DEP_REQUIRED },
    };

    ShaderCreateSettings settings = {
        .blend_desc = desc->blend_desc,
        .depth_desc = desc->depth_desc,
        .rasterizer_state = desc->rasterizer_state,
    };

    pulse_asset_handle h = asset_build(app, PULSE_TYPE_SHADER, nullptr, deps, 2, &settings);
    if (!pulse_asset_handle_is_valid(h)) {
        pulse_graphics_shader_library_unload(app, vs);
        pulse_graphics_shader_library_unload(app, fs);
        return {};
    }
    return {h.index, h.generation};
}

} // extern "C"
