#include "renderer_internal.h"

#include <string.h>

namespace pulse_renderer_internal {

struct renderable_feature_userdata {
    ecs_query_t* query;
};

static void renderable_feature_extract(PulseAppId app, ecs_world_t* world, FeatureExtractContext& ctx, void* userdata) {
    auto* ud = static_cast<renderable_feature_userdata*>(userdata);
    if (!ud || !ud->query) return;

    ecs_iter_t it = ecs_query_iter(world, ud->query);
    while (ecs_query_next(&it)) {
        PulseRenderable* renderables = ecs_field(&it, PulseRenderable, 0);
        PulseWorldTransform* world_transforms = ecs_field(&it, PulseWorldTransform, 1);
        for (int i = 0; i < it.count; ++i) {
            StagingItem item = {};
            item.entity = it.entities[i];
            item.mesh = renderables[i].mesh;
            item.material = renderables[i].material;
            item.shader = pulse_material_get_shader(app, renderables[i].material);
            item.world_matrix = world_transforms[i].value;
            ctx.submit(item);
        }
    }
    if (it.flags & EcsIterIsValid) {
        ecs_iter_fini(&it);
    }
}

static int32_t renderable_feature_cull(FeatureCullContext& ctx, const StagingItem& item, void* userdata) {
    (void)ctx;
    (void)item;
    (void)userdata;
    return 0;
}

static void renderable_feature_prepare(FeaturePrepareContext& ctx, void* userdata) {
    (void)userdata;
    for (DrawItem& item : ctx.items()) {
        if (item.feature_id != ctx.feature_id) continue;
        const StagingItem& staging = ctx.staging(item);
        if (staging.shader.index == 0) continue;

        for (uint32_t u = 0; u < pulse_shader_get_ubo_info_count(ctx.state->app, staging.shader); ++u) {
            const auto& info = pulse_shader_get_ubo_info(ctx.state->app, staging.shader, u);
            if (info.set != PULSE_SHADER_SET_FEATURE) continue;

            const uint32_t column = alloc_feature_ubo_column(ctx, item, info.set, info.binding, info.size);
            if (info.binding == 0 && info.size >= sizeof(HMM_Mat4)) {
                memcpy(ctx.view.ubo_columns[column].block_ref.ptr, &staging.world_matrix, sizeof(HMM_Mat4));
            }
        }
    }
}

static void renderable_feature_draw(PulseAppId app, PulseRenderPassEncoder* encoder, FeatureDrawContext& ctx, void* userdata) {
    (void)app;
    (void)userdata;
    const StagingItem& staging = ctx.staging();
    bind_item_ubo_columns(encoder, *ctx.view, *ctx.item);
    pulse_render_pass_encoder_draw(encoder, staging.material, staging.mesh);
}

void install_renderable_feature(pulse_renderer_state* state, ecs_world_t* world) {
    ecs_query_desc_t query_desc{};
    query_desc.terms[0].id = ecs_id(PulseRenderable);
    query_desc.terms[0].inout = EcsIn;
    query_desc.terms[1].id = ecs_id(PulseWorldTransform);
    query_desc.terms[1].inout = EcsIn;
    query_desc.cache_kind = EcsQueryCacheAuto;

    auto* ud = new renderable_feature_userdata{};
    ud->query = ecs_query_init(world, &query_desc);

    state->register_feature("Renderable", renderable_feature_extract, renderable_feature_cull, renderable_feature_prepare, renderable_feature_draw, 0, ud);
}

void shutdown_renderable_feature(void* userdata) {
    auto* ud = static_cast<renderable_feature_userdata*>(userdata);
    if (!ud) return;
    if (ud->query) {
        ecs_query_fini(ud->query);
    }
    delete ud;
}

} // namespace pulse_renderer_internal
