#include "text_render_internal.h"

#include <cstring>
#include <vector>

namespace pulse_text_render_internal {

uint8_t font_glyph_vert_spv[] = {
    #include "font_glyph.vs.spv.h"
};
uint8_t font_glyph_frag_spv[] = {
    #include "font_glyph.ps.spv.h"
};

ECS_COMPONENT_DECLARE(TextLayout);

struct TextLayout {
    PulseTextLayout* layout;
    uint64_t atlas_generation;
};

struct TextGlyph {
    float rect[4];
    float uv[4];
    float color[4];
};

namespace {

constexpr uint32_t kMaxGlyphsPerDraw = 512;
constexpr uint32_t kMaxTextureViewsPerPass = 64;
constexpr const char* kPropertyNameVPMatrix = "vpMatrix";
constexpr const char* kPropertyNameModelMatrix = "wMatrix";

text_feature_userdata* feature_of(void* userdata) {
    return static_cast<text_feature_userdata*>(userdata);
}

void relayout(PulseAppId app, ecs_world_t* world, ecs_entity_t entity, const PulseText& text) {
    TextLayout* layout = ecs_get_mut(world, entity, TextLayout);
    if (!layout) {
        return;
    }
    if (layout->layout) {
        pulse_text_layout_free(app, layout->layout);
    }
    layout->layout = nullptr;
    layout->atlas_generation = pulse_font_atlas_generation(app);
    if (strlen(text.text) > 0) {
        layout->layout = pulse_text_layout(app, &text.block, text.text, text.box_width, text.box_height);
    }
}

void on_text_set(ecs_iter_t* it)
{
    PulseAppId app = pulse_get_app_from_world(it->world);
    PulseText* texts = ecs_field(it, PulseText, 0);
    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_has_id(it->world, entity, ecs_id(TextLayout))) {
            relayout(app, it->world, entity, texts[i]);
        }
    }
}

void on_text_remove(ecs_iter_t* it)
{
    PulseAppId app = pulse_get_app_from_world(it->world);
    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];

        if (ecs_has_id(it->world, entity, ecs_id(TextLayout))) {
            TextLayout* layout = ecs_get_mut(it->world, entity, TextLayout);
            if (layout->layout) {
                pulse_text_layout_free(app, layout->layout);
            }
            layout->layout = nullptr;
            ecs_remove_id(it->world, entity, ecs_id(TextLayout));
        }
    }
}

bool create_assets(text_feature_userdata& ud) {
    CGPUBlendAttachmentState blend_attachment = {
        .enable = true,
        .src_factor = CGPU_BLEND_FACTOR_SRC_ALPHA,
        .dst_factor = CGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .src_alpha_factor = CGPU_BLEND_FACTOR_SRC_ALPHA,
        .dst_alpha_factor = CGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .blend_op = CGPU_BLEND_OP_ADD,
        .blend_alpha_op = CGPU_BLEND_OP_ADD,
        .color_mask = CGPU_COLOR_MASK_RGBA,
    };
    CGPUBlendStateDescriptor blend_desc = {
        .attachment_count = 1,
        .p_attachments = &blend_attachment,
        .alpha_to_coverage = false,
        .independent_blend = false,
    };
    CGPUDepthStateDescriptor depth_desc = {
        .depth_test = false,
        .depth_write = false,
        .stencil_test = false,
    };
    CGPURasterizerStateDescriptor rasterizer_state = {
        .cull_mode = CGPU_CULL_MODE_NONE,
    };

    PulseShaderProperty properties[2] = {};
    properties[0].name = kPropertyNameVPMatrix;
    properties[0].type = PULSE_SHADER_PROPERTY_TYPE_MAT4;
    properties[0].set = PULSE_SHADER_SET_GLOBAL;
    properties[0].binding = 0;
    properties[0].offset = 0;
    properties[0].size = sizeof(HMM_Mat4);
    properties[1].name = kPropertyNameModelMatrix;
    properties[1].type = PULSE_SHADER_PROPERTY_TYPE_MAT4;
    properties[1].set = PULSE_SHADER_SET_FEATURE;
    properties[1].binding = 0;
    properties[1].offset = 0;
    properties[1].size = sizeof(HMM_Mat4);

    PulseShaderCreateFromBinaryDesc shader_desc = {};
    shader_desc.p_vs_data = font_glyph_vert_spv;
    shader_desc.vs_data_size = sizeof(font_glyph_vert_spv);
    shader_desc.p_fs_data = font_glyph_frag_spv;
    shader_desc.fs_data_size = sizeof(font_glyph_frag_spv);
    shader_desc.blend_desc = blend_desc;
    shader_desc.depth_desc = depth_desc;
    shader_desc.rasterizer_state = rasterizer_state;
    shader_desc.p_properties = properties;
    shader_desc.properties_count = 2;

    ud.shader = pulse_create_shader_from_binary(ud.app, &shader_desc);

    PulseMaterialCreateDesc material_desc = {};
    material_desc.shader = ud.shader;
    ud.material = pulse_create_material(ud.app, &material_desc);

    CGPUVertexLayout vertex_layout = { .attribute_count = 0, .p_attributes = nullptr };
    PulseMeshCreateDynamicDesc mesh_desc = {};
    mesh_desc.topology = CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    mesh_desc.index_stride = 0;
    mesh_desc.layout = vertex_layout;
    ud.mesh = pulse_create_mesh_dynamic(ud.app, &mesh_desc);

    PulseSamplerCreateDesc sampler_desc = {};
    sampler_desc.desc = {
        .min_filter = CGPU_FILTER_TYPE_LINEAR,
        .mag_filter = CGPU_FILTER_TYPE_LINEAR,
        .mipmap_mode = CGPU_MIP_MAP_MODE_LINEAR,
        .address_u = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_v = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_w = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
    };
    ud.sampler = pulse_create_sampler(ud.app, &sampler_desc);
    if (!pulse_asset_handle_is_valid(pulse_sampler_to_handle(ud.sampler))) return false;

    return true;
}

void text_extract(PulseAppId app, PulseFeatureExtractContext* ctx, void* userdata) {
    text_feature_userdata* ud = feature_of(userdata);
    if (!ud || !ud->query) return;
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return;

    const uint64_t atlas_generation = pulse_font_atlas_generation(app);
    ecs_iter_t it = ecs_query_iter(world, ud->query);
    while (ecs_query_next(&it)) {
        PulseText* texts = ecs_field(&it, PulseText, 0);
        PulseWorldTransform* transforms = ecs_field(&it, PulseWorldTransform, 1);
        TextLayout* layouts = ecs_field(&it, TextLayout, 2);
        for (int i = 0; i < it.count; ++i) {
            if (texts[i].text[0] == '\0') continue;
            if (layouts[i].atlas_generation != atlas_generation) {
                relayout(app, world, it.entities[i], texts[i]);
            }
            if (!layouts[i].layout) continue;

            const PulseTextLayout& layout = *layouts[i].layout;
            const uint32_t instances_count = (uint32_t)layout.instances_count;
            const HMM_Mat4 world_matrix = transforms[i].value;

            uint32_t first = 0;
            while (first < instances_count) {
                const uint32_t page = layout.p_instances[first].page;
                uint32_t count = 0;
                while (first + count < instances_count && count < kMaxGlyphsPerDraw && layout.p_instances[first + count].page == page) ++count;

                PulseFeatureItem item = {};
                item.entity = it.entities[i];
                item.mesh = ud->mesh;
                item.material = ud->material;
                item.shader = ud->shader;
                item.world_matrix = world_matrix;
                item.instance_count = count;

                text_draw_data draw_data = {};
                draw_data.page = page;
                draw_data.payload_offset = first;
                draw_data.payload_count = count;
                pulse_feature_extract_submit(ctx, &item, &draw_data);

                first += count;
            }
        }
    }
    if (it.flags & EcsIterIsValid) {
        ecs_iter_fini(&it);
    }
}

void write_world_matrix(PulseAppId app, PulseShaderHandle shader, void* block, const HMM_Mat4& matrix) {
    for (uint32_t p = 0; p < pulse_shader_get_shader_property_count(app, shader); ++p) {
        const PulseShaderProperty prop = pulse_shader_get_shader_property(app, shader, p);
        if (!prop.name || strcmp(prop.name, kPropertyNameModelMatrix) != 0) continue;
        if (prop.set != PULSE_SHADER_SET_FEATURE || prop.binding != 0) continue;
        if (prop.type != PULSE_SHADER_PROPERTY_TYPE_MAT4 || prop.size != sizeof(HMM_Mat4)) continue;
        memcpy(static_cast<uint8_t*>(block) + prop.offset, &matrix, sizeof(HMM_Mat4));
    }
}

void text_prepare(PulseAppId app, PulseFeaturePrepareContext* ctx, void* userdata) {
    (void)userdata;
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return;

    const uint32_t count = pulse_feature_prepare_item_count(ctx);
    for (uint32_t i = 0; i < count; ++i) {
        PulseFeatureItem item = {};
        if (!pulse_feature_prepare_get_item(ctx, i, &item)) continue;
        if (item.shader.index == 0) continue;

        const text_draw_data* data = static_cast<const text_draw_data*>(pulse_feature_prepare_item_data(ctx, i));
        if (!data) continue;

        if (!ecs_is_alive(world, item.entity)) continue;
        const TextLayout* layout_component = ecs_get(world, item.entity, TextLayout);
        if (!layout_component || !layout_component->layout) continue;
        const PulseTextLayout& layout = *layout_component->layout;
        if ((size_t)data->payload_offset + data->payload_count > layout.instances_count) continue;

        void* world_block = pulse_feature_prepare_alloc_ubo(ctx, i, PULSE_SHADER_SET_FEATURE, 0, sizeof(HMM_Mat4));
        if (world_block) write_world_matrix(app, item.shader, world_block, item.world_matrix);

        const uint32_t glyph_size = data->payload_count * (uint32_t)sizeof(TextGlyph);
        TextGlyph* glyphs = static_cast<TextGlyph*>(pulse_feature_prepare_alloc_ubo(ctx, i, PULSE_SHADER_SET_FEATURE, 1, glyph_size));
        if (!glyphs) continue;
        for (uint32_t g = 0; g < data->payload_count; ++g) {
            const PulseGlyphInstance& src = layout.p_instances[data->payload_offset + g];
            glyphs[g].rect[0] = src.x;
            glyphs[g].rect[1] = src.y;
            glyphs[g].rect[2] = src.width;
            glyphs[g].rect[3] = src.height;
            glyphs[g].uv[0] = src.u0;
            glyphs[g].uv[1] = src.v0;
            glyphs[g].uv[2] = src.u1;
            glyphs[g].uv[3] = src.v1;
            glyphs[g].color[0] = src.r;
            glyphs[g].color[1] = src.g;
            glyphs[g].color[2] = src.b;
            glyphs[g].color[3] = src.a;
        }
    }
}

void text_record(PulseAppId app, PulseRenderGraphId graph, PulseRenderPassBuilder* pass, PulseFeatureRecordContext* ctx, void* userdata) {
    text_feature_userdata* ud = feature_of(userdata);
    if (!ud || !pass) return;

    const uint32_t page_count = pulse_font_page_count(app);

    ud->used_pages.clear();
    const uint32_t count = pulse_feature_record_item_count(ctx);
    for (uint32_t i = 0; i < count; ++i) {
        const text_draw_data* data = static_cast<const text_draw_data*>(pulse_feature_record_item_data(ctx, i));
        if (!data) continue;
        if (data->page >= page_count) continue;
        bool known = false;
        for (uint32_t existing : ud->used_pages) {
            if (existing == data->page) {
                known = true;
                break;
            }
        }
        if (!known) ud->used_pages.push_back(data->page);
    }
    if (ud->used_pages.size() > kMaxTextureViewsPerPass) {
        ud->used_pages.resize(kMaxTextureViewsPerPass);
    }

    for (uint32_t page : ud->used_pages) {
        const PulseTextureHandle handle = pulse_font_page_texture(app, page);
        PulseRGTextureHandle texture = pulse_render_graph_import_texture(graph, handle);
        if (!pulse_rgtexture_handle_is_valid(texture)) continue;
        pulse_render_pass_builder_sample(pass, texture);
    }
}

void text_draw(PulseAppId app, PulseRenderPassEncoder* encoder, PulseFeatureDrawContext* ctx, void* userdata) {
    text_feature_userdata* ud = feature_of(userdata);
    if (!ud) return;

    const PulseFeatureItem* item = pulse_feature_draw_get_item(ctx);
    if (!item) return;
    const text_draw_data* data = static_cast<const text_draw_data*>(pulse_feature_draw_item_data(ctx));
    if (!data) return;
    const PulseTextureHandle atlas = pulse_font_page_texture(app, data->page);

    pulse_feature_draw_bind_ubo_columns(encoder, ctx);
    pulse_render_pass_encoder_set_global_texture(encoder, atlas, PULSE_SHADER_SET_FEATURE, 2);
    pulse_render_pass_encoder_set_global_sampler(encoder, ud->sampler, PULSE_SHADER_SET_FEATURE, 3);
    pulse_render_pass_encoder_draw_submesh_instanced(encoder, item->material, item->mesh, 0, 0, 6, 0, item->instance_count, 0);
}

void release_assets(text_feature_userdata& ud) {
    PulseAssetSystemId asset_system = pulse_get_asset_system(ud.app);
    if (!asset_system) return;
    if (pulse_asset_handle_is_valid(pulse_mesh_to_handle(ud.mesh))) pulse_asset_system_release(asset_system, pulse_mesh_to_handle(ud.mesh), nullptr);
    if (pulse_asset_handle_is_valid(pulse_material_to_handle(ud.material))) pulse_asset_system_release(asset_system, pulse_material_to_handle(ud.material), nullptr);
    if (pulse_asset_handle_is_valid(pulse_shader_to_handle(ud.shader))) pulse_asset_system_release(asset_system, pulse_shader_to_handle(ud.shader), nullptr);
    if (pulse_asset_handle_is_valid(pulse_sampler_to_handle(ud.sampler))) pulse_asset_system_release(asset_system, pulse_sampler_to_handle(ud.sampler), nullptr);
}

void text_destroy(void* userdata) {
    text_feature_userdata* ud = feature_of(userdata);
    if (!ud) return;
    if (ud->query) {
        ecs_query_fini(ud->query);
        ud->query = nullptr;
    }
    release_assets(*ud);
    delete ud;
}

} // namespace

ECS_CTOR(TextLayout, ptr, {
    ptr->layout = nullptr;
    ptr->atlas_generation = 0;
    })

void install_text_feature(PulseAppId app, ecs_world_t* world) {
    if (!app || !world) return;

    auto* ud = new text_feature_userdata();
    ud->app = app;
    if (!create_assets(*ud)) {
        release_assets(*ud);
        delete ud;
        return;
    }

    ecs_type_hooks_t text_hooks = {
         .on_set = on_text_set,
        .on_remove = on_text_remove,
    };
    ecs_set_hooks_id(world, ecs_id(PulseText), &text_hooks);

    ecs_id(TextLayout) = flecs::_::type<TextLayout>::id(world);
    ecs_type_hooks_t text_layout_hooks = {
        .ctor = ecs_ctor(TextLayout),
    };
    ecs_set_hooks_id(world, ecs_id(TextLayout), &text_layout_hooks);
    ecs_add_pair(world, ecs_id(PulseText), EcsWith, ecs_id(TextLayout));

    ecs_query_desc_t query_desc{};
    query_desc.terms[0].id = ecs_id(PulseText);
    query_desc.terms[0].inout = EcsIn;
    query_desc.terms[1].id = ecs_id(PulseWorldTransform);
    query_desc.terms[1].inout = EcsIn;
    query_desc.terms[2].id = ecs_id(TextLayout);
    query_desc.terms[2].inout = EcsIn;
    query_desc.cache_kind = EcsQueryCacheAuto;
    ud->query = ecs_query_init(world, &query_desc);
    if (!ud->query) {
        release_assets(*ud);
        delete ud;
        return;
    }

    PulseRenderFeatureDesc desc = {};
    desc.struct_size = sizeof(PulseRenderFeatureDesc);
    desc.version = PULSE_RENDER_FEATURE_DESC_VERSION;
    desc.name = kTextFeatureName;
    desc.data_size = sizeof(text_draw_data);
    desc.extract = text_extract;
    desc.prepare = text_prepare;
    desc.draw = text_draw;
    desc.record = text_record;
    desc.destroy = text_destroy;
    desc.userdata = ud;
    if (pulse_add_render_feature(app, &desc) != PULSE_RESULT_OK) {
        text_destroy(ud);
    }
}

} // namespace pulse_text_render_internal
