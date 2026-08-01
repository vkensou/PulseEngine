#include "../graphics_internal.h"
#include <algorithm>

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

static bool validate_shader_properties_declare(const ShaderCreateSettings* desc, CGPURootSignatureId root_sig, pulse_shader_property_t** out_sorted_props, const char** out_validation_error)
{
    pulse_shader_property_t* sorted_props = new pulse_shader_property_t[desc->property_count];
    for (uint32_t i = 0; i < desc->property_count; ++i) {
        sorted_props[i].name = desc->p_properties[i].name;
        sorted_props[i].type = (int)desc->p_properties[i].type;
        sorted_props[i].role = (int)desc->p_properties[i].role;
        sorted_props[i].set = desc->p_properties[i].set;
        sorted_props[i].binding = desc->p_properties[i].binding;
        sorted_props[i].offset = desc->p_properties[i].offset;
        sorted_props[i].size = desc->p_properties[i].size;
    }
    std::sort(sorted_props, sorted_props + desc->property_count,
        [](const pulse_shader_property_t& a, const pulse_shader_property_t& b) {
            if (a.set != b.set) return a.set < b.set;
            if (a.binding != b.binding) return a.binding < b.binding;
            return a.offset < b.offset;
        });

    const char* validation_error = nullptr;
    uint32_t prop_idx = 0;
    for (uint32_t t = 0; t < root_sig->table_count && !validation_error; ++t) {
        uint32_t set = root_sig->p_tables[t].set_index;
        for (uint32_t r = 0; r < root_sig->p_tables[t].resources_count && !validation_error; ++r) {
            auto& res = root_sig->p_tables[t].p_resources[r];
            if (res.type != CGPU_RESOURCE_TYPE_UNIFORM_BUFFER && res.type != CGPU_RESOURCE_TYPE_RW_BUFFER)
                continue;

            while (prop_idx < desc->property_count &&
                (sorted_props[prop_idx].set < set ||
                    (sorted_props[prop_idx].set == set && sorted_props[prop_idx].binding < res.binding))) {
                ++prop_idx;
            }

            uint32_t prev_end = 0;
            uint32_t j = prop_idx;
            while (j < desc->property_count &&
                sorted_props[j].set == set &&
                sorted_props[j].binding == res.binding) {
                const auto& p = sorted_props[j];
                if (!ShaderPropertyIsUniform((EPulseShaderPropertyType)p.type)) {
                    validation_error = "shader create loader: property on ubo but not uniform";
                    break;
                }
                if (p.offset > res.size || p.size > res.size - p.offset) {
                    validation_error = "shader create loader: property range out of UBO bounds";
                    break;
                }
                if (p.offset < prev_end) {
                    validation_error = "shader create loader: overlapping property ranges";
                    break;
                }
                prev_end = p.offset + p.size;
                ++j;
            }
            prop_idx = j;
        }
    }

    if (out_validation_error)
        *out_validation_error = validation_error;

    if (!validation_error)
    {
        if (out_sorted_props)
            *out_sorted_props = sorted_props;
        return true;
    }
    else
    {
        delete[] sorted_props;
        if (out_sorted_props)
            *out_sorted_props = nullptr;
        return false;
    }
}

static void fill_property_data(pulse_shader_data_t* data, const ShaderCreateSettings* desc, CGPURootSignatureId root_sig, pulse_shader_property_t* sorted_props)
{
    if (sorted_props != nullptr) {
        data->property_count = desc->property_count;
        data->p_properties = sorted_props;
        for (uint32_t i = 0; i < data->property_count; ++i) {
            const char* src_name = data->p_properties[i].name;
            size_t name_len = src_name ? strlen(src_name) : 0;
            char* name_copy = new char[name_len + 1];
            if (src_name) memcpy(name_copy, src_name, name_len);
            name_copy[name_len] = '\0';
            data->p_properties[i].name = name_copy;
        }
    }
    else {
        data->property_count = 0;
        data->p_properties = nullptr;
    }

    {
        uint32_t ubo_count = 0;
        for (uint32_t t = 0; t < root_sig->table_count; ++t)
            for (uint32_t r = 0; r < root_sig->p_tables[t].resources_count; ++r)
                if (root_sig->p_tables[t].p_resources[r].type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER ||
                    root_sig->p_tables[t].p_resources[r].type == CGPU_RESOURCE_TYPE_RW_BUFFER)
                    ++ubo_count;

        data->ubo_info_count = ubo_count;
        data->p_ubo_infos = ubo_count > 0 ? new pulse_shader_ubo_info_t[ubo_count] : nullptr;

        uint32_t ubo_idx = 0;
        uint32_t prop_idx = 0;
        for (uint32_t t = 0; t < root_sig->table_count; ++t) {
            uint32_t set = root_sig->p_tables[t].set_index;
            for (uint32_t r = 0; r < root_sig->p_tables[t].resources_count; ++r) {
                auto& res = root_sig->p_tables[t].p_resources[r];
                if (res.type != CGPU_RESOURCE_TYPE_UNIFORM_BUFFER && res.type != CGPU_RESOURCE_TYPE_RW_BUFFER)
                    continue;

                while (prop_idx < data->property_count &&
                    (data->p_properties[prop_idx].set < set ||
                        (data->p_properties[prop_idx].set == set && data->p_properties[prop_idx].binding < res.binding))) {
                    ++prop_idx;
                }

                pulse_shader_ubo_info_t& entry = data->p_ubo_infos[ubo_idx];
                entry = {};
                entry.set = set;
                entry.binding = res.binding;
                entry.ubo_size = res.size;
                entry.material_managed = false;
                entry.renderer_managed = false;
                entry.layout_hash = 0;

                uint32_t j = prop_idx;
                while (j < data->property_count &&
                    data->p_properties[j].set == set &&
                    data->p_properties[j].binding == res.binding) {
                    const auto& p = data->p_properties[j];
                    if (p.role == PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL)
                        entry.renderer_managed = true;
                    if (p.role == PULSE_SHADER_PROPERTY_ROLE_MATERIAL)
                        entry.material_managed = true;

                    HGEGraphics::hash_combine(entry.layout_hash, (const char*)p.name);
                    HGEGraphics::hash_combine(entry.layout_hash, (int)p.type);
                    HGEGraphics::hash_combine(entry.layout_hash, (int)p.role);
                    HGEGraphics::hash_combine(entry.layout_hash, p.set);
                    HGEGraphics::hash_combine(entry.layout_hash, p.binding);
                    HGEGraphics::hash_combine(entry.layout_hash, p.offset);
                    HGEGraphics::hash_combine(entry.layout_hash, p.size);
                    ++j;
                }
                prop_idx = j;
                ++ubo_idx;
            }
        }
    }

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
                    for (uint32_t u = 0; u < data->ubo_info_count; ++u) {
                        if (data->p_ubo_infos[u].set == set_idx && data->p_ubo_infos[u].binding == res.binding) {
                            if (data->p_ubo_infos[u].renderer_managed)
                                data->p_set_infos[t].renderer_managed = true;
                            HGEGraphics::hash_combine(set_hash, data->p_ubo_infos[u].layout_hash);
                            break;
                        }
                    }
                }
                else {
                    HGEGraphics::hash_combine(set_hash, res.name_hash);
                    HGEGraphics::hash_combine(set_hash, (int)res.type);
                    HGEGraphics::hash_combine(set_hash, res.set);
                    HGEGraphics::hash_combine(set_hash, res.binding);
                    HGEGraphics::hash_combine(set_hash, (int)res.dim);
                }
            }
            data->p_set_infos[t].layout_hash = set_hash;
        }
    }

    for (uint32_t i = 0; i < data->property_count; ++i)
    {
        const auto& prop = data->p_properties[i];
        if (prop.role == PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL) {
            for (uint32_t j = 0; j < data->set_info_count; ++j)
            {
                auto& set_info = data->p_set_infos[j];
                if (set_info.set_index == prop.set)
                {
                    set_info.renderer_managed = true;
                    break;
                }
            }
        }
    }
}

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
    if (!root_sig) {
        internal_release_shader_library(ctx->asset_system, &vs_ref);
        internal_release_shader_library(ctx->asset_system, &ps_ref);
        *out_error = "shader create loader: failed to create root signature";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    // === Pre-validation: sort properties and validate against root_sig before any allocation ===
    pulse_shader_property_t* sorted_props = nullptr;
    const char* validation_error = nullptr;
    if (desc->property_count > 0 && desc->p_properties
        && !validate_shader_properties_declare(desc, root_sig, &sorted_props, &validation_error)) {
        delete[] sorted_props;
        cgpu_device_free_root_signature(device, root_sig);
        internal_release_shader_library(ctx->asset_system, &vs_ref);
        internal_release_shader_library(ctx->asset_system, &ps_ref);
        *out_error = validation_error;
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    // === Build phase: all validation passed, no error paths below ===
    data->root_sig = root_sig;
    data->vs = ppl_shaders[0];
    data->ps = ppl_shaders[1];
    data->blend_desc = desc->blend_desc;
    if (desc->blend_desc.attachment_count > 0 && desc->blend_desc.p_attachments != nullptr)
    {
        data->blend_attachment_states_count = desc->blend_desc.attachment_count;
        data->p_blend_attachment_states = new CGPUBlendAttachmentState[data->blend_attachment_states_count];
        std::copy(desc->blend_desc.p_attachments, desc->blend_desc.p_attachments + desc->blend_desc.attachment_count, data->p_blend_attachment_states);
    }
    else
    {
        data->blend_attachment_states_count = 0;
        data->p_blend_attachment_states = nullptr;
    }
    data->blend_desc.p_attachments = data->p_blend_attachment_states;

    data->depth_desc = desc->depth_desc;
    data->rasterizer_state = desc->rasterizer_state;

    fill_property_data(data, desc, root_sig, sorted_props);

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
    if (!pulse_shader_library_is_alive(app, fs)) {
        pulse_unload_shader_library(app, vs);
        return {};
    }
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
    if (!pulse_shader_library_is_alive(app, fs)) {
        pulse_unload_shader_library(app, vs);
        return {};
    }
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
