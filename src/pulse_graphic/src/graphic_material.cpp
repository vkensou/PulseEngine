#include "graphic_internal.h"
#include "renderer.h"
#include <vector>
#include <unordered_map>

struct MaterialBinding {
    int type;
    uint32_t set;
    uint32_t binding;
    pulse_asset_handle resource;
};

struct MaterialInternal {
    pulse_material_data_t* cpp_material;
    std::vector<MaterialBinding> bindings;
    uint64_t map_key;
};

static std::unordered_map<uint64_t, MaterialInternal*> s_material_map;

static MaterialInternal* get_material_internal(pulse_asset_handle handle) {
    uint64_t key = (uint64_t)handle.type_id << 32 | handle.index;
    auto it = s_material_map.find(key);
    return it != s_material_map.end() ? it->second : nullptr;
}

namespace pulse_graphic_internal {

void material_internal_destroy(void* data) {
    pulse_material_data_t* mat_data = static_cast<pulse_material_data_t*>(data);
    //uint64_t key = (uint64_t)mat_data->self.type_id << 32 | mat_data->self.index;
    //auto it = s_material_map.find(key);
    //if (it != s_material_map.end()) {
    //    delete it->second->cpp_material;
    //    delete it->second;
    //    s_material_map.erase(it);
    //}
}

} // namespace pulse_graphic_internal

extern "C" {

pulse_material_t pulse_graphic_material_create(
    pulse_app_t app,
    pulse_shader_t shader)
{
    pulse_material_t result{};
    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device) return result;

    pulse_shader_data_t* shader_data = pulse_graphic_shader_acquire(app, &shader);
    if (!shader_data) return result;

    pulse_shader_data_t cpp_shader;
    cpp_shader.root_sig = shader_data->root_sig;
    cpp_shader.vs = shader_data->vs;
    cpp_shader.ps = shader_data->ps;
    cpp_shader.blend_desc = shader_data->blend_desc;
    //cpp_shader.blend_attachment_states.assign(
    //    shader_data->blend_attachments,
    //    shader_data->blend_attachments + 8);
    cpp_shader.depth_desc = shader_data->depth_desc;
    cpp_shader.rasterizer_state = shader_data->rasterizer_state;

    pulse_material_data_t* mat = new pulse_material_data_t(device, &cpp_shader);
    pulse_graphic_shader_release(app, &shader);
    if (!mat) return result;

    pulse_asset_handle asset_handle = pulse_graphic_internal::asset_load_memory_path(
        app, PULSE_TYPE_MATERIAL, "", nullptr, 0);
    if (!pulse_asset_handle_is_valid(asset_handle)) {
        delete mat;
        return result;
    }

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_material_data_t* data = static_cast<pulse_material_data_t*>(ref.ptr);
        //data->shader = shader.asset;
        //data->self = asset_handle;

        MaterialInternal* internal = new MaterialInternal();
        internal->cpp_material = mat;
        internal->map_key = (uint64_t)asset_handle.type_id << 32 | asset_handle.index;
        s_material_map[internal->map_key] = internal;

        pulse_asset_release(app, &ref);
    }

    result.asset = asset_handle;
    return result;
}

void pulse_graphic_material_bind_buffer(
    pulse_app_t app, pulse_material_t* material,
    uint32_t set, uint32_t binding,
    pulse_buffer_t buffer)
{
    pulse_graphic_buffer_ref ref{};
    if (!pulse_graphic_buffer_acquire(app, buffer, &ref)) return;

    pulse_asset_ref mat_ref{};
    if (!pulse_asset_acquire(app, material->asset, &mat_ref)) {
        pulse_graphic_buffer_release(app, &ref);
        return;
    }

    MaterialInternal* internal = get_material_internal(material->asset);
    if (internal) {
        internal->bindings.push_back({0, set, binding, pulse_graphic_buffer_to_handle(buffer)});
        //internal->cpp_material->bindBuffer((int)set, (int)binding, buf_data->handle->info->size, nullptr);
    }

    pulse_asset_release(app, &mat_ref);
    pulse_graphic_buffer_release(app, &ref);
}

void pulse_graphic_material_bind_texture(
    pulse_app_t app, pulse_material_t* material,
    uint32_t set, uint32_t binding,
    pulse_texture_t texture)
{
    pulse_graphic_texture_ref ref{};
    if (!pulse_graphic_texture_acquire(app, texture, &ref)) return;

    pulse_asset_ref mat_ref{};
    if (!pulse_asset_acquire(app, material->asset, &mat_ref)) {
        pulse_graphic_texture_release(app, &ref);
        return;
    }

    MaterialInternal* internal = get_material_internal(material->asset);
    if (internal) {
        internal->bindings.push_back({1, set, binding, pulse_graphic_texture_to_handle(texture)});
        //internal->cpp_material->bindTexture((int)set, (int)binding, nullptr);
    }

    pulse_asset_release(app, &mat_ref);
    pulse_graphic_texture_release(app, &ref);
}

void pulse_graphic_material_bind_sampler(
    pulse_app_t app, pulse_material_t* material,
    uint32_t set, uint32_t binding,
    pulse_sampler_t sampler)
{
    pulse_sampler_data_t* smp_data = pulse_graphic_sampler_acquire(app, &sampler);
    if (!smp_data) return;

    pulse_asset_ref mat_ref{};
    if (!pulse_asset_acquire(app, material->asset, &mat_ref)) {
        pulse_graphic_sampler_release(app, &sampler);
        return;
    }

    MaterialInternal* internal = get_material_internal(material->asset);
    if (internal) {
        internal->bindings.push_back({2, set, binding, sampler.asset});
        //internal->cpp_material->bindSampler((int)set, (int)binding, smp_data->handle);
    }

    pulse_asset_release(app, &mat_ref);
    pulse_graphic_sampler_release(app, &sampler);
}

pulse_material_data_t* pulse_graphic_material_acquire(pulse_app_t app, pulse_material_t* handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, handle->asset, &ref)) {
        return static_cast<pulse_material_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphic_material_release(pulse_app_t app, pulse_material_t* handle) {
    pulse_asset_ref ref{handle->asset, nullptr};
    pulse_asset_release(app, &ref);
}

} // extern "C"
