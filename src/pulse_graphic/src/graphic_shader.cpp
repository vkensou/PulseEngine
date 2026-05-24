#include "graphic_internal.h"
#include "renderer.h"
#include <cstring>

namespace pulse_graphic_internal {

static pulse_shader_t create_shader_impl(
    pulse_app_t app,
    const void* vs_data, uint32_t vs_size,
    const void* fs_data, uint32_t fs_size,
    bool is_compute,
    const void* cs_data, uint32_t cs_size,
    const CGPUBlendStateDescriptor* blend_desc,
    const CGPUDepthStateDescriptor* depth_desc,
    const CGPURasterizerStateDescriptor* rasterizer_state)
{
    pulse_shader_t result{};
    CGPUDeviceId device = get_device(app);
    if (!device) return result;

    uint64_t type_id = is_compute ? PULSE_TYPE_COMPUTE_SHADER : PULSE_TYPE_SHADER;

    if (is_compute) {
        auto cpp_shader = HGEGraphics::create_compute_shader(
            device, static_cast<const uint8_t*>(cs_data), cs_size);
        if (!cpp_shader) return result;

        pulse_asset_handle asset_handle = pulse_asset_load_from_memory(
            app, type_id, "", nullptr, 0);
        if (asset_handle.index == PULSE_ASSET_INVALID_INDEX) return result;

        pulse_asset_ref ref{};
        if (pulse_asset_acquire(app, asset_handle, &ref)) {
            pulse_compute_shader_data_t* data = static_cast<pulse_compute_shader_data_t*>(ref.ptr);
            data->root_sig = cpp_shader->root_sig;
            data->cs = cpp_shader->cs;
            cpp_shader->root_sig = CGPU_NULLPTR;
            cpp_shader->cs.library = CGPU_NULLPTR;
            pulse_asset_release(app, &ref);
        }
        result.asset = asset_handle;
    } else {
        CGPUBlendStateDescriptor default_blend{};
        if (!blend_desc) blend_desc = &default_blend;
        CGPUDepthStateDescriptor default_depth{};
        if (!depth_desc) depth_desc = &default_depth;
        CGPURasterizerStateDescriptor default_rasterizer{};
        if (!rasterizer_state) rasterizer_state = &default_rasterizer;

        auto cpp_shader = HGEGraphics::create_shader(
            device,
            static_cast<const uint8_t*>(vs_data), vs_size,
            static_cast<const uint8_t*>(fs_data), fs_size,
            *blend_desc, *depth_desc, *rasterizer_state);
        if (!cpp_shader) return result;

        pulse_asset_handle asset_handle = pulse_asset_load_from_memory(
            app, type_id, "", nullptr, 0);
        if (asset_handle.index == PULSE_ASSET_INVALID_INDEX) return result;

        pulse_asset_ref ref{};
        if (pulse_asset_acquire(app, asset_handle, &ref)) {
            pulse_shader_data_t* data = static_cast<pulse_shader_data_t*>(ref.ptr);
            data->root_sig = cpp_shader->root_sig;
            data->vs = cpp_shader->vs;
            data->ps = cpp_shader->ps;
            data->blend_desc = cpp_shader->blend_desc;
            for (size_t i = 0; i < cpp_shader->blend_attachment_states.size() && i < 8; ++i) {
                data->blend_attachments[i] = cpp_shader->blend_attachment_states[i];
            }
            data->blend_desc.p_attachments = data->blend_attachments;
            data->blend_desc.attachment_count = cpp_shader->blend_attachment_states.size() < 8
                ? (uint32_t)cpp_shader->blend_attachment_states.size() : 8u;
            data->depth_desc = cpp_shader->depth_desc;
            data->rasterizer_state = cpp_shader->rasterizer_state;
            cpp_shader->root_sig = CGPU_NULLPTR;
            cpp_shader->vs.library = CGPU_NULLPTR;
            cpp_shader->ps.library = CGPU_NULLPTR;
            pulse_asset_release(app, &ref);
        }
        result.asset = asset_handle;
    }
    return result;
}

} // namespace pulse_graphic_internal

using namespace pulse_graphic_internal;

extern "C" {

pulse_shader_t pulse_graphic_shader_create_from_binary(
    pulse_app_t app,
    const void* vs_data, uint32_t vs_size,
    const void* fs_data, uint32_t fs_size,
    const CGPUBlendStateDescriptor* blend_desc,
    const CGPUDepthStateDescriptor* depth_desc,
    const CGPURasterizerStateDescriptor* rasterizer_state)
{
    return create_shader_impl(app, vs_data, vs_size, fs_data, fs_size,
        false, nullptr, 0, blend_desc, depth_desc, rasterizer_state);
}

pulse_compute_shader_t pulse_graphic_compute_shader_create_from_binary(
    pulse_app_t app,
    const void* cs_data, uint32_t cs_size)
{
    return {};
}

pulse_shader_t pulse_graphic_shader_load(
    pulse_app_t app,
    const char* vert_path,
    const char* frag_path,
    const CGPUBlendStateDescriptor* blend_desc,
    const CGPUDepthStateDescriptor* depth_desc,
    const CGPURasterizerStateDescriptor* rasterizer_state)
{
    (void)blend_desc; (void)depth_desc; (void)rasterizer_state;
    pulse_asset_handle vs = pulse_asset_load(app, PULSE_TYPE_SHADER_LIBRARY, vert_path);
    if (vs.index == PULSE_ASSET_INVALID_INDEX) return pulse_shader_t{};
    pulse_asset_handle fs = pulse_asset_load(app, PULSE_TYPE_SHADER_LIBRARY, frag_path);
    if (fs.index == PULSE_ASSET_INVALID_INDEX) return pulse_shader_t{};
    pulse_asset_dependency deps[] = {
        { vs, PULSE_DEP_REQUIRED },
        { fs, PULSE_DEP_REQUIRED },
    };
    pulse_asset_handle h = pulse_asset_load_with_deps(app, PULSE_TYPE_SHADER, nullptr, deps, 2);
    if (h.index == PULSE_ASSET_INVALID_INDEX) return pulse_shader_t{};
    return pulse_shader_t{h};
}

pulse_compute_shader_t pulse_graphic_compute_shader_load(
    pulse_app_t app,
    const char* comp_path)
{
    pulse_asset_handle cs = pulse_asset_load(app, PULSE_TYPE_SHADER_LIBRARY, comp_path);
    if (cs.index == PULSE_ASSET_INVALID_INDEX) return pulse_compute_shader_t{};
    pulse_asset_dependency deps[] = {
        { cs, PULSE_DEP_REQUIRED },
    };
    pulse_asset_handle h = pulse_asset_load_with_deps(app, PULSE_TYPE_COMPUTE_SHADER, nullptr, deps, 1);
    if (h.index == PULSE_ASSET_INVALID_INDEX) return pulse_compute_shader_t{};
    return pulse_compute_shader_t{h};
}

pulse_shader_data_t* pulse_graphic_shader_acquire(pulse_app_t app, pulse_shader_t* handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, handle->asset, &ref)) {
        return static_cast<pulse_shader_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphic_shader_release(pulse_app_t app, pulse_shader_t* handle) {
    pulse_asset_ref ref{handle->asset, nullptr};
    pulse_asset_release(app, &ref);
}

pulse_compute_shader_data_t* pulse_graphic_compute_shader_acquire(pulse_app_t app, pulse_compute_shader_t* handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, handle->asset, &ref)) {
        return static_cast<pulse_compute_shader_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphic_compute_shader_release(pulse_app_t app, pulse_compute_shader_t* handle) {
    pulse_asset_ref ref{ handle->asset, nullptr };
    pulse_asset_release(app, &ref);
}

} // extern "C"
