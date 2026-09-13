#include "../graphics_internal.h"
#include "pulse_datalist.h"

#include <vector>

namespace pulse_graphics_internal {

struct MaterialFileLoadState {
    bool deps_requested = false;
};

struct MaterialPropEntry {
    const char* name;
    EPulseShaderPropertyType type;
    const char* texture_path;
    bool texture_mipmaps;
};

static size_t material_prop_component_count(EPulseShaderPropertyType type) {
    switch (type) {
    case PULSE_SHADER_PROPERTY_TYPE_FLOAT4: return 4;
    case PULSE_SHADER_PROPERTY_TYPE_MAT4: return 16;
    default: return 0;
    }
}

static bool parse_material_properties(PulseDatalist* dl, std::vector<MaterialPropEntry>& props, const char** out_error) {
    PulseDatalist* plist = pulse_datalist_get_obj(dl, "properties");
    if (!plist)
        return true;

    size_t n = pulse_datalist_count(plist);
    for (size_t i = 0; i < n; ++i) {
        PulseDatalist* p = pulse_datalist_get(plist, i);
        MaterialPropEntry entry = {};
        entry.name = pulse_datalist_get_string(p, "name", nullptr);
        if (!entry.name) {
            *out_error = "material file loader: property missing 'name'";
            return false;
        }
        entry.type = prop_type_from_string(p);
        if (material_prop_component_count(entry.type) > 0) {
            PulseDatalist* v = pulse_datalist_get_obj(p, "value");
            if (!v || pulse_datalist_count(v) != material_prop_component_count(entry.type)) {
                *out_error = "material file loader: numeric property needs a matching component-count 'value' list";
                return false;
            }
        } else if (entry.type == PULSE_SHADER_PROPERTY_TYPE_TEXTURE) {
            entry.texture_path = pulse_datalist_get_string(p, "value", nullptr);
            if (!entry.texture_path) {
                *out_error = "material file loader: texture property needs a 'value' path";
                return false;
            }
            entry.texture_mipmaps = pulse_datalist_get_bool(p, "mipmaps", false);
        } else {
            *out_error = "material file loader: unsupported property type (float4, mat4, texture only)";
            return false;
        }
        props.push_back(entry);
    }
    return true;
}

static EPulseAssetLoaderStatus step_material_from_file(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<MaterialFileLoadState*>(state);

    if (!s->deps_requested) {
        PulseDatalist* dl = parse_datalist_bytes(ctx, out_error);
        if (!dl)
            return PULSE_ASSET_LOADER_STATUS_FAILED;

        const char* shader_path = pulse_datalist_get_string(dl, "shader", nullptr);
        if (!shader_path) {
            pulse_datalist_release(dl);
            *out_error = "material file loader: missing 'shader' path";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        PulseShaderRequest shader_req = pulse_load_shader(ctx->app, shader_path);
        auto shader_asset_request = pulse_shader_request_to_asset_request(shader_req);
        if (!pulse_asset_request_is_valid(shader_asset_request)) {
            pulse_datalist_release(dl);
            *out_error = "material file loader: failed to request shader";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, shader_asset_request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED);

        std::vector<MaterialPropEntry> props;
        if (!parse_material_properties(dl, props, out_error)) {
            pulse_datalist_release(dl);
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        for (const MaterialPropEntry& entry : props) {
            if (entry.type != PULSE_SHADER_PROPERTY_TYPE_TEXTURE)
                continue;
            PulseTextureLoadDesc tex_desc = { entry.texture_path, entry.texture_mipmaps };
            PulseTextureRequest tex_req = pulse_load_texture(ctx->app, &tex_desc);
            auto tex_asset_request = pulse_texture_request_to_asset_request(tex_req);
            if (!pulse_asset_request_is_valid(tex_asset_request)) {
                pulse_datalist_release(dl);
                *out_error = "material file loader: failed to request texture";
                return PULSE_ASSET_LOADER_STATUS_FAILED;
            }
            pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, tex_asset_request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED);
        }

        pulse_datalist_release(dl);
        s->deps_requested = true;
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
    }

    PulseDatalist* dl = parse_datalist_bytes(ctx, out_error);
    if (!dl)
        return PULSE_ASSET_LOADER_STATUS_FAILED;

    const char* shader_path = pulse_datalist_get_string(dl, "shader", nullptr);
    if (!shader_path) {
        pulse_datalist_release(dl);
        *out_error = "material file loader: missing 'shader' path";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    std::vector<MaterialPropEntry> props;
    if (!parse_material_properties(dl, props, out_error)) {
        pulse_datalist_release(dl);
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    PulseShaderRequest shader_req = pulse_load_shader(ctx->app, shader_path);
    PulseShaderHandle shader_handle = pulse_shader_get_handle(ctx->app, shader_req);
    PulseShaderData* shader_data = internal_borrow_shader(ctx->asset_system, shader_handle);
    if (!shader_data) {
        pulse_datalist_release(dl);
        *out_error = "material file loader: shader not available";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    auto* mat = static_cast<PulseMaterialData*>(ctx->out_asset);
    PulseShader shader_ref = { shader_handle, shader_data };
    HGEGraphics::init_material(mat, device, shader_ref);

    PulseDatalist* plist = pulse_datalist_get_obj(dl, "properties");
    for (size_t i = 0; i < props.size(); ++i) {
        const MaterialPropEntry& entry = props[i];
        PulseDatalist* p = pulse_datalist_get(plist, i);
        if (entry.type == PULSE_SHADER_PROPERTY_TYPE_FLOAT4) {
            PulseDatalist* v = pulse_datalist_get_obj(p, "value");
            float c[4];
            for (int k = 0; k < 4; ++k)
                c[k] = (float)pulse_datalist_get_double(pulse_datalist_get(v, (size_t)k), nullptr, 0.0);
            pulse_material_set_float4(mat, entry.name, HMM_V4(c[0], c[1], c[2], c[3]));
        } else if (entry.type == PULSE_SHADER_PROPERTY_TYPE_MAT4) {
            PulseDatalist* v = pulse_datalist_get_obj(p, "value");
            HMM_Mat4 m = {};
            for (int k = 0; k < 16; ++k)
                m.Elements[k / 4][k % 4] = (float)pulse_datalist_get_double(pulse_datalist_get(v, (size_t)k), nullptr, 0.0);
            pulse_material_set_mat4(mat, entry.name, m);
        } else if (entry.type == PULSE_SHADER_PROPERTY_TYPE_TEXTURE) {
            PulseTextureLoadDesc tex_desc = { entry.texture_path, entry.texture_mipmaps };
            PulseTextureRequest tex_req = pulse_load_texture(ctx->app, &tex_desc);
            PulseTextureHandle tex_handle = pulse_texture_get_handle(ctx->app, tex_req);
            PulseTextureData* tex_data = internal_borrow_texture(ctx->asset_system, tex_handle);
            if (!tex_data) {
                pulse_datalist_release(dl);
                *out_error = "material file loader: texture not available";
                return PULSE_ASSET_LOADER_STATUS_FAILED;
            }
            pulse_material_set_texture(mat, entry.name, tex_data);
        }
    }

    pulse_datalist_release(dl);
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_material_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_MATERIAL;
    ld.extensions = "material";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_material_from_file;
    ld.loader_size = sizeof(MaterialFileLoadState);
    ld.loader_align = alignof(MaterialFileLoadState);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseMaterialRequest pulse_load_material(PulseAppId app, const char* filepath)
{
    PulseMaterialRequest result{};
    if (!app || !filepath || !filepath[0]) return result;

    PulseAssetSystemId as = asset_system_from_app(app);
    PulseAssetRequest request = asset_load_path(as, PULSE_TYPE_MATERIAL, filepath);
    if (!pulse_asset_request_is_valid(request)) return result;
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

} // extern "C"
