#include "../graphics_internal.h"

namespace pulse_graphics_internal {

struct ShaderCreateSettings {
    CGPUBlendStateDescriptor blend_desc;
    CGPUDepthStateDescriptor depth_desc;
    CGPURasterizerStateDescriptor rasterizer_state;
    uint32_t property_count;
    const PulseShaderPropertyDesc* p_properties;
};

struct ShaderLoaderState {
    bool vsPrepared = false;
    bool psPrepared = false;
};

static EPulseAssetLoaderStatus step_shader_from_deps(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<ShaderLoaderState*>(state);

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader loader: no device"; return PULSE_ASSET_LOADER_STATUS_FAILED; }

    auto vsHandle = ctx->dependencies[0].handle;
    PulseShaderLibraryHandle vs = { vsHandle.index, vsHandle.generation };
    if (!s->vsPrepared) {
        auto vsState = pulse_asset_system_get_state(ctx->asset_system, vsHandle);
        if (vsState == PULSE_ASSET_STATE_LOADED)
            s->vsPrepared = true;
        else if (vsState == PULSE_ASSET_STATE_FAILED || vsState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "shader create loader: failed to wait vertex shader";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        else
            s->vsPrepared = false;
    }

    auto psHandle = ctx->dependencies[1].handle;
    PulseShaderLibraryHandle ps = { psHandle.index, psHandle.generation };
    if (!s->psPrepared) {
        auto psState = pulse_asset_system_get_state(ctx->asset_system, psHandle);
        if (psState == PULSE_ASSET_STATE_LOADED)
            s->psPrepared = true;
        else if (psState == PULSE_ASSET_STATE_FAILED || psState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "shader create loader: failed to wait fragment shader";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        else
            s->psPrepared = false;
    }

    if (!s->vsPrepared || !s->psPrepared)
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;

    PulseShaderLibrary vs_ref{};
    if (!internal_acquire_shader_library(ctx->asset_system, vs, &vs_ref)) {
        *out_error = "shader create loader: failed to acquire vertex shader library";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    PulseShaderLibrary ps_ref{};
    if (!internal_acquire_shader_library(ctx->asset_system, ps, &ps_ref)) {
        internal_release_shader_library(ctx->asset_system, &vs_ref);
        *out_error = "shader create loader: failed to acquire fragment shader library";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    CGPUShaderLibraryId vs_lib = static_cast<pulse_shader_library_data_t*>(vs_ref.ptr)->library;
    CGPUShaderLibraryId fs_lib = static_cast<pulse_shader_library_data_t*>(ps_ref.ptr)->library;

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

    // Copy shader properties
    if (desc->property_count > 0 && desc->p_properties) {
        data->property_count = desc->property_count;
        data->p_properties = new pulse_shader_property_t[desc->property_count];
        for (uint32_t i = 0; i < desc->property_count; ++i) {
            data->p_properties[i].name = desc->p_properties[i].name;
            data->p_properties[i].role = (int)desc->p_properties[i].role;
            data->p_properties[i].set = desc->p_properties[i].set;
            data->p_properties[i].binding = desc->p_properties[i].binding;
            data->p_properties[i].offset = desc->p_properties[i].offset;
        }
    } else {
        data->property_count = 0;
        data->p_properties = nullptr;
    }

    // Build UBO info: group properties by (set,binding), compute layout hash
    {
        // Collect unique (set,binding) pairs
        struct UboKey { uint32_t set; uint32_t binding; bool operator<(const UboKey& o) const { return set != o.set ? set < o.set : binding < o.binding; } };
        struct UboEntry { uint32_t set; uint32_t binding; bool renderer_managed; uint64_t hash; };
        std::vector<UboEntry> ubo_list;
        std::vector<UboKey> seen_keys;

        for (uint32_t i = 0; i < data->property_count; ++i) {
            auto& prop = data->p_properties[i];
            UboKey key = { prop.set, prop.binding };
            bool found = false;
            for (size_t k = 0; k < seen_keys.size(); ++k) {
                if (seen_keys[k].set == key.set && seen_keys[k].binding == key.binding) { found = true; break; }
            }
            if (found) continue;
            seen_keys.push_back(key);

            UboEntry entry = {};
            entry.set = prop.set;
            entry.binding = prop.binding;
            entry.renderer_managed = false;
            entry.hash = 0;

            // Accumulate all properties at this (set,binding) for hash
            for (uint32_t j = 0; j < data->property_count; ++j) {
                auto& p = data->p_properties[j];
                if (p.set != prop.set || p.binding != prop.binding) continue;
                if (p.role == PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL)
                    entry.renderer_managed = true;
                HGEGraphics::hash_combine(entry.hash, (const char*)p.name);
                HGEGraphics::hash_combine(entry.hash, (int)p.role);
                HGEGraphics::hash_combine(entry.hash, p.set);
                HGEGraphics::hash_combine(entry.hash, p.binding);
                HGEGraphics::hash_combine(entry.hash, p.offset);
            }
            ubo_list.push_back(entry);
        }

        data->ubo_info_count = (uint32_t)ubo_list.size();
        data->p_ubo_infos = new pulse_shader_ubo_info_t[ubo_list.size()];
        for (size_t i = 0; i < ubo_list.size(); ++i) {
            data->p_ubo_infos[i].set = ubo_list[i].set;
            data->p_ubo_infos[i].binding = ubo_list[i].binding;
            data->p_ubo_infos[i].renderer_managed = ubo_list[i].renderer_managed;
            data->p_ubo_infos[i].layout_hash = ubo_list[i].hash;
        }
    }

    // Build descriptor set info: for each set in root sig, compute combined hash
    {
        uint32_t table_count = root_sig->table_count;
        data->set_info_count = table_count;
        data->p_set_infos = new pulse_shader_set_info_t[table_count];
        for (uint32_t t = 0; t < table_count; ++t) {
            uint32_t set_idx = root_sig->p_tables[t].set_index;
            data->p_set_infos[t].set_index = set_idx;
            data->p_set_infos[t].renderer_managed = false;
            uint64_t set_hash = 0;

            for (uint32_t j = 0; j < root_sig->p_tables[t].resources_count; ++j) {
                auto& res = root_sig->p_tables[t].p_resources[j];
                if (res.type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || res.type == CGPU_RESOURCE_TYPE_RW_BUFFER) {
                    // Find matching ubo_info
                    for (uint32_t u = 0; u < data->ubo_info_count; ++u) {
                        if (data->p_ubo_infos[u].set == set_idx && data->p_ubo_infos[u].binding == res.binding) {
                            if (data->p_ubo_infos[u].renderer_managed)
                                data->p_set_infos[t].renderer_managed = true;
                            HGEGraphics::hash_combine(set_hash, data->p_ubo_infos[u].layout_hash);
                            break;
                        }
                    }
                } else {
                    HGEGraphics::hash_combine(set_hash, (const char*)res.name);
                    HGEGraphics::hash_combine(set_hash, (int)res.type);
                }
            }
            data->p_set_infos[t].layout_hash = set_hash;
        }
    }

    internal_release_shader_library(ctx->asset_system, &vs_ref);
    internal_release_shader_library(ctx->asset_system, &ps_ref);

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_shader_create_loaders(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
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
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseShaderHandle pulse_create_shader_from_binary(
    PulseAppId app,
    const PulseShaderCreateFromBinaryDesc* desc)
{
    if (!desc || !desc->vs_data || !desc->vs_size || !desc->fs_data || !desc->fs_size) return {};

    PulseShaderLibraryCreateDesc vs_desc = {
        desc->vs_data,
        desc->vs_size
    };
    PulseShaderLibraryCreateDesc fs_desc = {
        desc->fs_data,
        desc->fs_size
    };

    auto vs = pulse_create_shader_library(app, &vs_desc);
    if (!pulse_shader_library_is_alive(app, vs)) return {};
    auto fs = pulse_create_shader_library(app, &fs_desc);
    if (!pulse_shader_library_is_alive(app, fs)) return {};
    PulseAssetDependency deps[] = {
        { pulse_shader_library_to_handle(vs), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
        { pulse_shader_library_to_handle(fs), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
    };

	ShaderCreateSettings settings = {
        .blend_desc = desc->blend_desc,
        .depth_desc = desc->depth_desc,
        .rasterizer_state = desc->rasterizer_state,
        .property_count = desc->property_count,
        .p_properties = desc->p_properties,
    };

    PulseAssetHandle h = asset_build(app, PULSE_TYPE_SHADER, nullptr, deps, 2, &settings);
    if (!pulse_asset_handle_is_valid(h)) {
		pulse_unload_shader_library(app, vs);
		pulse_unload_shader_library(app, fs);
        return {};
    }
    return {h.index, h.generation};
}

PulseShaderHandle pulse_create_shader_from_file(
    PulseAppId app,
    const PulseShaderCreateFromFileDesc* desc)
{
    if (!desc || !desc->vert_path || !desc->frag_path) return {};

    PulseShaderLibraryLoadDesc vs_desc = {
        desc->vert_path
    };
    PulseShaderLibraryLoadDesc fs_desc = {
        desc->frag_path
    };

    auto vs = pulse_load_shader_library(app, &vs_desc);
    if (!pulse_shader_library_is_alive(app, vs)) return {};
    auto fs = pulse_load_shader_library(app, &fs_desc);
    if (!pulse_shader_library_is_alive(app, fs)) return {};
    PulseAssetDependency deps[] = {
        { pulse_shader_library_to_handle(vs), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
        { pulse_shader_library_to_handle(fs), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
    };

    ShaderCreateSettings settings = {
        .blend_desc = desc->blend_desc,
        .depth_desc = desc->depth_desc,
        .rasterizer_state = desc->rasterizer_state,
        .property_count = desc->property_count,
        .p_properties = desc->p_properties,
    };

    PulseAssetHandle h = asset_build(app, PULSE_TYPE_SHADER, nullptr, deps, 2, &settings);
    if (!pulse_asset_handle_is_valid(h)) {
        pulse_unload_shader_library(app, vs);
        pulse_unload_shader_library(app, fs);
        return {};
    }
    return {h.index, h.generation};
}

} // extern "C"
