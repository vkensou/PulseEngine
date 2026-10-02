#include "renderer_internal.h"

#include <string.h>

namespace pulse_renderer_internal {

struct renderable_feature_userdata {
    ecs_query_t* query;
};

static void renderable_feature_extract(PulseAppId app, PulseFeatureExtractContext* ctx, void* userdata) {
    auto* ud = static_cast<renderable_feature_userdata*>(userdata);
    if (!ud || !ud->query) return;

    ecs_world_t* world = pulse_app_world(app);
    if (!world) return;

    ecs_iter_t it = ecs_query_iter(world, ud->query);
    while (ecs_query_next(&it)) {
        PulseRenderable* renderables = ecs_field(&it, PulseRenderable, 0);
        PulseWorldTransform* world_transforms = ecs_field(&it, PulseWorldTransform, 1);
        for (int i = 0; i < it.count; ++i) {
            PulseFeatureItem item = {};
            item.entity = it.entities[i];
            item.mesh = renderables[i].mesh;
            item.material = renderables[i].material;
            item.shader = pulse_material_get_shader(app, renderables[i].material);
            item.world_matrix = world_transforms[i].value;
            pulse_feature_extract_submit(ctx, &item, nullptr);
        }
    }
    if (it.flags & EcsIterIsValid) {
        ecs_iter_fini(&it);
    }
}

static void write_model_matrix(PulseAppId app, PulseShaderHandle shader, uint32_t set, uint32_t binding, void* block, const HMM_Mat4& matrix) {
    for (uint32_t p = 0; p < pulse_shader_get_shader_property_count(app, shader); ++p) {
        const PulseShaderProperty prop = pulse_shader_get_shader_property(app, shader, p);
        if (!prop.name || prop.set != set || prop.binding != binding) continue;
        if (strcmp(prop.name, kPropertyNameModelMatrix) != 0) continue;
        if (prop.type != PULSE_SHADER_PROPERTY_TYPE_MAT4 || prop.size != sizeof(HMM_Mat4)) continue;
        memcpy(static_cast<uint8_t*>(block) + prop.offset, &matrix, sizeof(HMM_Mat4));
    }
}

static void renderable_feature_prepare(PulseAppId app, PulseFeaturePrepareContext* ctx, void* userdata) {
    (void)userdata;
    const uint32_t count = pulse_feature_prepare_item_count(ctx);
    for (uint32_t i = 0; i < count; ++i) {
        PulseFeatureItem item = {};
        if (!pulse_feature_prepare_get_item(ctx, i, &item)) continue;
        if (item.shader.index == 0) continue;

        for (uint32_t u = 0; u < pulse_shader_get_ubo_info_count(app, item.shader); ++u) {
            const auto& info = pulse_shader_get_ubo_info(app, item.shader, u);
            if (info.set != PULSE_SHADER_SET_FEATURE) continue;
            void* block = pulse_feature_prepare_alloc_ubo(ctx, i, info.set, info.binding, info.size);
            if (!block) continue;
            write_model_matrix(app, item.shader, info.set, info.binding, block, item.world_matrix);
        }
    }
}

static void renderable_feature_draw(PulseAppId app, PulseRenderPassEncoder* encoder, PulseFeatureDrawContext* ctx, void* userdata) {
    (void)app;
    (void)userdata;
    const PulseFeatureItem* item = pulse_feature_draw_get_item(ctx);
    if (!item) return;
    pulse_feature_draw_bind_ubo_columns(encoder, ctx);
    pulse_render_pass_encoder_draw(encoder, item->material, item->mesh);
}

static void renderable_feature_destroy(void* userdata) {
    auto* ud = static_cast<renderable_feature_userdata*>(userdata);
    if (!ud) return;
    if (ud->query) {
        ecs_query_fini(ud->query);
    }
    delete ud;
}

void install_renderable_feature(PulseAppId app, ecs_world_t* world) {
    ecs_query_desc_t query_desc{};
    query_desc.terms[0].id = ecs_id(PulseRenderable);
    query_desc.terms[0].inout = EcsIn;
    query_desc.terms[1].id = ecs_id(PulseWorldTransform);
    query_desc.terms[1].inout = EcsIn;
    query_desc.cache_kind = EcsQueryCacheAuto;

    auto* ud = new renderable_feature_userdata{};
    ud->query = ecs_query_init(world, &query_desc);

    PulseRenderFeatureDesc desc = {};
    desc.struct_size = sizeof(PulseRenderFeatureDesc);
    desc.version = PULSE_RENDER_FEATURE_DESC_VERSION;
    desc.name = "Renderable";
    desc.extract = renderable_feature_extract;
    desc.prepare = renderable_feature_prepare;
    desc.draw = renderable_feature_draw;
    desc.destroy = renderable_feature_destroy;
    desc.userdata = ud;
    if (pulse_add_render_feature(app, &desc) != PULSE_RESULT_OK) {
        renderable_feature_destroy(ud);
    }
}

} // namespace pulse_renderer_internal
