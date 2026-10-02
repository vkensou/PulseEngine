#include "../graphics_internal.h"
#include <algorithm>
#include <cstring>

namespace pulse_graphics_internal {

// Settings deep-copy: the asset system allocates one block of the returned size and
// copies the struct bytes to its head; this callback lays out the nested data right
// after the struct (blend attachments, then property array, then property name
// strings) and fixes the pointers into the block. size/copy share the same layout.
static uint64_t shader_settings_size_fn(const void* settings, void* user_data) {
    const auto* s = static_cast<const ShaderCreateSettings*>(settings);
    uint64_t total = sizeof(ShaderCreateSettings);

    if (s->blend_desc.p_attachments && s->blend_desc.attachment_count > 0) {
        total += (uint64_t)s->blend_desc.attachment_count * sizeof(CGPUBlendAttachmentState);
    }

    if (s->p_properties && s->property_count > 0) {
        // Mirror the align_up_pointer in the copy callback so both compute the same layout.
        total = align_up_value(total, alignof(PulseShaderProperty));
        total += (uint64_t)s->property_count * sizeof(PulseShaderProperty);
        for (uint32_t i = 0; i < s->property_count; ++i) {
            if (s->p_properties[i].name) {
                total += strlen(s->p_properties[i].name) + 1;
            }
        }
    }
    return total;
}

static bool shader_settings_copy_fn(void* dst, const void* src, uint64_t byte_size, void* user_data) {
    auto* d = static_cast<ShaderCreateSettings*>(dst);
    const auto* s = static_cast<const ShaderCreateSettings*>(src);
    uint8_t* end = reinterpret_cast<uint8_t*>(dst) + byte_size;
    uint8_t* cursor = reinterpret_cast<uint8_t*>(dst) + sizeof(ShaderCreateSettings);

    if (s->blend_desc.p_attachments && s->blend_desc.attachment_count > 0) {
        uint64_t n = (uint64_t)s->blend_desc.attachment_count * sizeof(CGPUBlendAttachmentState);
        if (cursor + n > end) {
            return false;
        }
        memcpy(cursor, s->blend_desc.p_attachments, n);
        d->blend_desc.p_attachments = reinterpret_cast<const CGPUBlendAttachmentState*>(cursor);
        cursor += n;
    } else {
        d->blend_desc.p_attachments = nullptr;
    }

    if (s->p_properties && s->property_count > 0) {
        cursor = align_up_pointer(cursor, alignof(PulseShaderProperty));
        uint64_t n = (uint64_t)s->property_count * sizeof(PulseShaderProperty);
        if (cursor + n > end) {
            return false;
        }
        auto* props = reinterpret_cast<PulseShaderProperty*>(cursor);
        memcpy(props, s->p_properties, n);
        cursor += n;

        for (uint32_t i = 0; i < s->property_count; ++i) {
            const char* name = s->p_properties[i].name;
            if (!name) {
                continue;
            }
            size_t len = strlen(name) + 1;
            if (cursor + len > end) {
                return false;
            }
            memcpy(cursor, name, len);
            props[i].name = reinterpret_cast<const char*>(cursor);
            cursor += len;
        }
        d->p_properties = props;
    } else {
        d->p_properties = nullptr;
    }
    return true;
}

static const CGPUShaderResource* find_shader_resource(CGPURootSignatureId root_sig, uint32_t set, uint32_t binding)
{
    for (uint32_t t = 0; t < root_sig->table_count; ++t) {
        const auto& table = root_sig->p_tables[t];
        if (table.set_index != set) continue;
        for (uint32_t r = 0; r < table.resources_count; ++r) {
            if (table.p_resources[r].binding == binding) return &table.p_resources[r];
        }
    }
    return nullptr;
}

static bool property_type_matches_resource(EPulseShaderPropertyType type, ECGPUResourceTypeFlags res_type)
{
    if (type == PULSE_SHADER_PROPERTY_TYPE_TEXTURE) return res_type == CGPU_RESOURCE_TYPE_TEXTURE;
    if (type == PULSE_SHADER_PROPERTY_TYPE_SAMPLER) return res_type == CGPU_RESOURCE_TYPE_SAMPLER;
    return ShaderPropertyIsUniform(type) && (res_type == CGPU_RESOURCE_TYPE_UNIFORM_BUFFER || res_type == CGPU_RESOURCE_TYPE_RW_BUFFER);
}

static bool validate_shader_properties_declare(const ShaderCreateSettings* desc, CGPURootSignatureId root_sig, PulseShaderProperty** out_sorted_props, const char** out_validation_error)
{
    PulseShaderProperty* sorted_props = new PulseShaderProperty[desc->property_count];
    for (uint32_t i = 0; i < desc->property_count; ++i) {
        sorted_props[i] = desc->p_properties[i];
    }
    std::sort(sorted_props, sorted_props + desc->property_count,
        [](const PulseShaderProperty& a, const PulseShaderProperty& b) {
            if (a.set != b.set) return a.set < b.set;
            if (a.binding != b.binding) return a.binding < b.binding;
            return a.offset < b.offset;
        });

    const char* validation_error = nullptr;

    for (uint32_t t = 0; t < root_sig->table_count && !validation_error; ++t) {
        if (root_sig->p_tables[t].set_index >= PULSE_SHADER_SET_COUNT) {
            validation_error = "shader create loader: root signature set index out of range";
        }
    }

    uint32_t prev_set = UINT32_MAX;
    uint32_t prev_binding = UINT32_MAX;
    uint32_t prev_end = 0;
    for (uint32_t i = 0; i < desc->property_count && !validation_error; ++i) {
        const auto& p = sorted_props[i];
        if (p.set >= PULSE_SHADER_SET_COUNT) {
            validation_error = "shader create loader: property set index out of range";
            break;
        }

        const CGPUShaderResource* res = find_shader_resource(root_sig, p.set, p.binding);
        if (!res) {
            validation_error = "shader create loader: property binding not found in root signature";
            break;
        }
        if (!property_type_matches_resource((EPulseShaderPropertyType)p.type, res->type)) {
            validation_error = "shader create loader: property type does not match root signature resource";
            break;
        }

        if (p.set != prev_set || p.binding != prev_binding) {
            prev_set = p.set;
            prev_binding = p.binding;
            prev_end = 0;
        }
        if (!ShaderPropertyIsUniform((EPulseShaderPropertyType)p.type)) continue;

        if (p.offset > res->size || p.size > res->size - p.offset) {
            validation_error = "shader create loader: property range out of UBO bounds";
            break;
        }
        if (p.offset < prev_end) {
            validation_error = "shader create loader: overlapping property ranges";
            break;
        }
        prev_end = p.offset + p.size;
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

static void fill_property_data(PulseShaderData* data, const ShaderCreateSettings* desc, CGPURootSignatureId root_sig, PulseShaderProperty* sorted_props)
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
        data->p_ubo_infos = ubo_count > 0 ? new PulseUboInfo[ubo_count] : nullptr;

        uint32_t ubo_idx = 0;
        for (uint32_t t = 0; t < root_sig->table_count; ++t) {
            uint32_t set = root_sig->p_tables[t].set_index;
            for (uint32_t r = 0; r < root_sig->p_tables[t].resources_count; ++r) {
                auto& res = root_sig->p_tables[t].p_resources[r];
                if (res.type != CGPU_RESOURCE_TYPE_UNIFORM_BUFFER && res.type != CGPU_RESOURCE_TYPE_RW_BUFFER)
                    continue;

                PulseUboInfo& entry = data->p_ubo_infos[ubo_idx];
                entry = {};
                entry.set = set;
                entry.binding = res.binding;
                entry.size = res.size;

                for (uint32_t i = 0; i < data->property_count; ++i) {
                    const auto& p = data->p_properties[i];
                    if (p.set != set || p.binding != res.binding) continue;

                    HGEGraphics::hash_combine(entry.layout_hash, (const char*)p.name);
                    HGEGraphics::hash_combine(entry.layout_hash, (int)p.type);
                    HGEGraphics::hash_combine(entry.layout_hash, p.set);
                    HGEGraphics::hash_combine(entry.layout_hash, p.binding);
                    HGEGraphics::hash_combine(entry.layout_hash, p.offset);
                    HGEGraphics::hash_combine(entry.layout_hash, p.size);
                }
                ++ubo_idx;
            }
        }
    }
}

EPulseAssetLoaderStatus build_shader_pipeline(const PulseAssetLoadTask* ctx, const ShaderCreateSettings* desc, const char** out_error)
{
    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    if (!device) { *out_error = "shader loader: no device"; return PULSE_ASSET_LOADER_STATUS_FAILED; }

    auto vsRequest = pulse_asset_system_to_asset_request_from_dep_ref(ctx->asset_system, ctx->p_dependencies[0].dep_ref);
    PulseShaderLibraryHandle vs = pulse_shader_library_get_handle(ctx->app, { vsRequest.index, vsRequest.generation });
    PulseShaderLibraryData* vs_data = internal_borrow_shader_library(ctx->asset_system, vs);
    if (!vs_data) {
        *out_error = "shader create loader: failed to acquire vertex shader library";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    auto psRequest = pulse_asset_system_to_asset_request_from_dep_ref(ctx->asset_system, ctx->p_dependencies[1].dep_ref);
    PulseShaderLibraryHandle ps = pulse_shader_library_get_handle(ctx->app, { psRequest.index, psRequest.generation });
    PulseShaderLibraryData* ps_data = internal_borrow_shader_library(ctx->asset_system, ps);
    if (!ps_data) {
        *out_error = "shader create loader: failed to acquire fragment shader library";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    CGPUShaderLibraryId vs_lib = vs_data->library;
    CGPUShaderLibraryId fs_lib = ps_data->library;

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
        .dynamic_buffers = true,
    };
    auto root_sig = cgpu_device_create_root_signature(device, &rs_desc);
    if (!root_sig) {
        *out_error = "shader create loader: failed to create root signature";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    // === Pre-validation: sort properties and validate against root_sig before any allocation ===
    PulseShaderProperty* sorted_props = nullptr;
    const char* validation_error = nullptr;
    if (desc->property_count > 0 && desc->p_properties
        && !validate_shader_properties_declare(desc, root_sig, &sorted_props, &validation_error)) {
        delete[] sorted_props;
        cgpu_device_free_root_signature(device, root_sig);
        *out_error = validation_error;
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    // === Build phase: all validation passed, no error paths below ===
    auto* data = static_cast<PulseShaderData*>(ctx->out_asset);
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

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseAssetLoaderStatus step_shader_from_deps(
    void*, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    return build_shader_pipeline(ctx, static_cast<const ShaderCreateSettings*>(ctx->settings), out_error);
}

void register_shader_create_loaders(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SHADER;
    ld.extensions = nullptr;
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_shader_from_deps;
    ld.loader_size = 0;
    ld.loader_align = 0;
    ld.settings_size = sizeof(ShaderCreateSettings);
    ld.settings_align = alignof(ShaderCreateSettings);
    ld.settings_size_fn = shader_settings_size_fn;
    ld.settings_copy_fn = shader_settings_copy_fn;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseShaderHandle pulse_create_shader_from_binary(
    PulseAppId app,
    const PulseShaderCreateFromBinaryDesc* desc)
{
    if (!desc || !desc->p_vs_data || !desc->vs_data_size || !desc->p_fs_data || !desc->fs_data_size) return {};

    PulseShaderLibraryCreateDesc vs_desc = {
        desc->p_vs_data,
        desc->vs_data_size
    };
    PulseShaderLibraryCreateDesc fs_desc = {
        desc->p_fs_data,
        desc->fs_data_size
    };

    PulseAssetSystemId as = asset_system_from_app(app);
    auto vs = pulse_create_shader_library(app, &vs_desc);
    if (!pulse_asset_handle_is_valid(pulse_shader_library_to_handle(vs))) return {};
    auto fs = pulse_create_shader_library(app, &fs_desc);
    if (!pulse_asset_handle_is_valid(pulse_shader_library_to_handle(fs))) {
        pulse_asset_system_release(as, pulse_shader_library_to_handle(vs), nullptr);
        return {};
    }
    PulseAssetDependency deps[] = {
        { pulse_asset_system_to_asset_dep_ref_from_handle(as, pulse_shader_library_to_handle(vs)), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
        { pulse_asset_system_to_asset_dep_ref_from_handle(as, pulse_shader_library_to_handle(fs)), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
    };

    ShaderCreateSettings settings = {
        .blend_desc = desc->blend_desc,
        .depth_desc = desc->depth_desc,
        .rasterizer_state = desc->rasterizer_state,
        .property_count = (uint32_t)desc->properties_count,
        .p_properties = desc->p_properties,
    };

    PulseAssetHandle h = asset_build_sync(as, PULSE_TYPE_SHADER, nullptr, nullptr, deps, 2, &settings);
    if (!pulse_asset_handle_is_valid(h)) {
        pulse_asset_system_release(as, pulse_shader_library_to_handle(vs), nullptr);
        pulse_asset_system_release(as, pulse_shader_library_to_handle(fs), nullptr);
        return {};
    }
    return {h.index, h.generation};
}

} // extern "C"
