#include "../graphics_internal.h"
#include "pulse_datalist.h"

#include <cstring>
#include <vector>

namespace pulse_graphics_internal {

static uint32_t color_mask_from_string(const PulseDatalist* node, const char* key) {
    static const NamedValue table[] = {
        { "none", 0 },
        { "r",    CGPU_COLOR_MASK_R },
        { "g",    CGPU_COLOR_MASK_G },
        { "b",    CGPU_COLOR_MASK_B },
        { "a",    CGPU_COLOR_MASK_A },
        { "rgb",  CGPU_COLOR_MASK_RGB },
        { "rgba", CGPU_COLOR_MASK_RGBA },
    };
    return (uint32_t)enum_from_string(node, key, table, sizeof(table) / sizeof(table[0]), CGPU_COLOR_MASK_RGBA);
}

static const char* read_prop_name(const PulseDatalist* prop) {
    return pulse_datalist_get_string(prop, "name", nullptr);
}

static EPulseShaderPropertyRole prop_role_from_string(const PulseDatalist* prop) {
    static const NamedValue table[] = {
        { "material",     PULSE_SHADER_PROPERTY_ROLE_MATERIAL },
        { "non_material", PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL },
    };
    return (EPulseShaderPropertyRole)enum_from_string(prop, "role", table, sizeof(table) / sizeof(table[0]), PULSE_SHADER_PROPERTY_ROLE_MATERIAL);
}

struct ShaderFileLoadState {
    bool libs_requested = false;
};

// Parse a .shader datalist node into a ShaderCreateSettings. The blend attachment array and the
// property array are laid out in the caller-owned vectors; property name pointers reference strings
// owned by the datalist node, which must stay alive for the duration of the pipeline build.
static void parse_shader_file(PulseDatalist* dl, ShaderCreateSettings* settings, std::vector<CGPUBlendAttachmentState>& attachments, std::vector<PulseShaderProperty>& props) {
    PulseDatalist* depth = pulse_datalist_get_obj(dl, "depth");
    settings->depth_desc = {};
    if (depth) {
        static const NamedValue cmp_ops[] = {
            { "never",         CGPU_COMPARE_OP_NEVER },
            { "less",          CGPU_COMPARE_OP_LESS },
            { "equal",         CGPU_COMPARE_OP_EQUAL },
            { "less_equal",    CGPU_COMPARE_OP_LESS_EQUAL },
            { "greater",       CGPU_COMPARE_OP_GREATER },
            { "not_equal",     CGPU_COMPARE_OP_NOT_EQUAL },
            { "greater_equal", CGPU_COMPARE_OP_GREATER_EQUAL },
            { "always",        CGPU_COMPARE_OP_ALWAYS },
        };
        settings->depth_desc.depth_test = pulse_datalist_get_bool(depth, "depth_test", false);
        settings->depth_desc.depth_write = pulse_datalist_get_bool(depth, "depth_write", false);
        settings->depth_desc.depth_op = (ECGPUCompareOp)enum_from_string(depth, "depth_op", cmp_ops, sizeof(cmp_ops) / sizeof(cmp_ops[0]), CGPU_COMPARE_OP_LESS);
        settings->depth_desc.stencil_test = pulse_datalist_get_bool(depth, "stencil_test", false);
    }

    PulseDatalist* raster = pulse_datalist_get_obj(dl, "rasterizer");
    settings->rasterizer_state = {};
    if (raster) {
        static const NamedValue cull_modes[] = {
            { "none",  CGPU_CULL_MODE_NONE },
            { "back",  CGPU_CULL_MODE_BACK },
            { "front", CGPU_CULL_MODE_FRONT },
            { "both",  CGPU_CULL_MODE_BOTH },
        };
        static const NamedValue front_faces[] = {
            { "counter_clockwise", CGPU_FRONT_FACE_COUNTER_CLOCKWISE },
            { "clock_wise",        CGPU_FRONT_FACE_CLOCK_WISE },
        };
        settings->rasterizer_state.cull_mode = (ECGPUCullModeFlags)enum_from_string(raster, "cull_mode", cull_modes, sizeof(cull_modes) / sizeof(cull_modes[0]), CGPU_CULL_MODE_NONE);
        settings->rasterizer_state.front_face = (ECGPUFrontFace)enum_from_string(raster, "front_face", front_faces, sizeof(front_faces) / sizeof(front_faces[0]), CGPU_FRONT_FACE_COUNTER_CLOCKWISE);
        settings->rasterizer_state.depth_bias = (int32_t)pulse_datalist_get_int(raster, "depth_bias", 0);
    }

    PulseDatalist* blend = pulse_datalist_get_obj(dl, "blend");
    settings->blend_desc = {};
    if (blend) {
        static const NamedValue blend_factors[] = {
            { "zero",                  CGPU_BLEND_FACTOR_ZERO },
            { "one",                   CGPU_BLEND_FACTOR_ONE },
            { "src_color",             CGPU_BLEND_FACTOR_SRC_COLOR },
            { "one_minus_src_color",   CGPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR },
            { "dst_color",             CGPU_BLEND_FACTOR_DST_COLOR },
            { "one_minus_dst_color",   CGPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR },
            { "src_alpha",             CGPU_BLEND_FACTOR_SRC_ALPHA },
            { "one_minus_src_alpha",   CGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA },
            { "dst_alpha",             CGPU_BLEND_FACTOR_DST_ALPHA },
            { "one_minus_dst_alpha",   CGPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA },
            { "src_alpha_saturate",    CGPU_BLEND_FACTOR_SRC_ALPHA_SATURATE },
        };
        static const NamedValue blend_ops[] = {
            { "add",              CGPU_BLEND_OP_ADD },
            { "subtract",         CGPU_BLEND_OP_SUBTRACT },
            { "reverse_subtract", CGPU_BLEND_OP_REVERSE_SUBTRACT },
            { "min",              CGPU_BLEND_OP_MIN },
            { "max",              CGPU_BLEND_OP_MAX },
        };
        settings->blend_desc.alpha_to_coverage = pulse_datalist_get_bool(blend, "alpha_to_coverage", false);
        settings->blend_desc.independent_blend = pulse_datalist_get_bool(blend, "independent_blend", false);
        PulseDatalist* list = pulse_datalist_get_obj(blend, "attachments");
        if (list) {
            size_t n = pulse_datalist_count(list);
            attachments.resize(n);
            for (size_t i = 0; i < n; ++i) {
                PulseDatalist* a = pulse_datalist_get(list, i);
                CGPUBlendAttachmentState& att = attachments[i];
                att = {};
                att.enable = pulse_datalist_get_bool(a, "enable", false);
                att.src_factor = (ECGPUBlendFactor)enum_from_string(a, "src_factor", blend_factors, sizeof(blend_factors) / sizeof(blend_factors[0]), CGPU_BLEND_FACTOR_ONE);
                att.dst_factor = (ECGPUBlendFactor)enum_from_string(a, "dst_factor", blend_factors, sizeof(blend_factors) / sizeof(blend_factors[0]), CGPU_BLEND_FACTOR_ZERO);
                att.src_alpha_factor = (ECGPUBlendFactor)enum_from_string(a, "src_alpha_factor", blend_factors, sizeof(blend_factors) / sizeof(blend_factors[0]), CGPU_BLEND_FACTOR_ONE);
                att.dst_alpha_factor = (ECGPUBlendFactor)enum_from_string(a, "dst_alpha_factor", blend_factors, sizeof(blend_factors) / sizeof(blend_factors[0]), CGPU_BLEND_FACTOR_ZERO);
                att.blend_op = (ECGPUBlendOp)enum_from_string(a, "blend_op", blend_ops, sizeof(blend_ops) / sizeof(blend_ops[0]), CGPU_BLEND_OP_ADD);
                att.blend_alpha_op = (ECGPUBlendOp)enum_from_string(a, "blend_alpha_op", blend_ops, sizeof(blend_ops) / sizeof(blend_ops[0]), CGPU_BLEND_OP_ADD);
                att.color_mask = (ECGPUColorMaskFlags)color_mask_from_string(a, "color_mask");
            }
            settings->blend_desc.attachment_count = (uint32_t)n;
            settings->blend_desc.p_attachments = n > 0 ? attachments.data() : nullptr;
        }
    }

    PulseDatalist* plist = pulse_datalist_get_obj(dl, "properties");
    if (plist) {
        size_t n = pulse_datalist_count(plist);
        props.resize(n);
        for (size_t i = 0; i < n; ++i) {
            PulseDatalist* p = pulse_datalist_get(plist, i);
            PulseShaderProperty& prop = props[i];
            prop = {};
            prop.name = read_prop_name(p);
            prop.type = prop_type_from_string(p);
            prop.role = prop_role_from_string(p);
            prop.set = (uint32_t)pulse_datalist_get_int(p, "set", 0);
            prop.binding = (uint32_t)pulse_datalist_get_int(p, "binding", 0);
            prop.offset = (uint32_t)pulse_datalist_get_int(p, "offset", 0);
            prop.size = (uint32_t)pulse_datalist_get_int(p, "size", 0);
        }
        settings->property_count = (uint32_t)n;
        settings->p_properties = n > 0 ? props.data() : nullptr;
    } else {
        settings->property_count = 0;
        settings->p_properties = nullptr;
    }
}

static EPulseAssetLoaderStatus step_shader_from_file(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<ShaderFileLoadState*>(state);

    if (!s->libs_requested) {
        PulseDatalist* dl = parse_datalist_bytes(ctx, out_error);
        if (!dl)
            return PULSE_ASSET_LOADER_STATUS_FAILED;

        const char* vert = pulse_datalist_get_string(dl, "vert", nullptr);
        const char* frag = pulse_datalist_get_string(dl, "frag", nullptr);
        if (!vert || !frag) {
            pulse_datalist_release(dl);
            *out_error = "shader file loader: missing 'vert' or 'frag' path";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        PulseShaderLibraryLoadDesc vs_desc = { vert };
        PulseShaderLibraryLoadDesc fs_desc = { frag };
        PulseShaderLibraryRequest vs = pulse_load_shader_library(ctx->app, &vs_desc);
        PulseShaderLibraryRequest fs = pulse_load_shader_library(ctx->app, &fs_desc);
        pulse_datalist_release(dl);

        auto vs_asset_request = pulse_shader_library_request_to_asset_request(vs);
        auto fs_asset_request = pulse_shader_library_request_to_asset_request(fs);
        if (!pulse_asset_request_is_valid(vs_asset_request) || !pulse_asset_request_is_valid(fs_asset_request)) {
            *out_error = "shader file loader: failed to request shader libraries";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, vs_asset_request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED);
        pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, fs_asset_request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED);

        s->libs_requested = true;
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
    }

    PulseDatalist* dl = parse_datalist_bytes(ctx, out_error);
    if (!dl)
        return PULSE_ASSET_LOADER_STATUS_FAILED;

    ShaderCreateSettings settings = {};
    std::vector<CGPUBlendAttachmentState> attachments;
    std::vector<PulseShaderProperty> props;
    parse_shader_file(dl, &settings, attachments, props);

    EPulseAssetLoaderStatus status = build_shader_pipeline(ctx, &settings, out_error);
    pulse_datalist_release(dl);
    return status;
}

void register_shader_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_SHADER;
    ld.extensions = "shader";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_shader_from_file;
    ld.loader_size = sizeof(ShaderFileLoadState);
    ld.loader_align = alignof(ShaderFileLoadState);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseShaderRequest pulse_load_shader(PulseAppId app, const char* filepath)
{
    PulseShaderRequest result{};
    if (!app || !filepath || !filepath[0]) return result;

    PulseAssetSystemId as = asset_system_from_app(app);
    PulseAssetRequest request = asset_load_path(as, PULSE_TYPE_SHADER, filepath);
    if (!pulse_asset_request_is_valid(request)) return result;
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

} // extern "C"
