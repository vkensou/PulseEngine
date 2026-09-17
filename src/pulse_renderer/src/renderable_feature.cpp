#include "renderer_internal.h"

#include <string.h>
#include <optional>
#include <utility>

namespace pulse_renderer_internal {

struct renderable_feature_userdata {
    ecs_query_t* query;
};

static bool renderable_accepts_list(const DrawItem& item, uint32_t list_id) {
    (void)item;
    (void)list_id;
    return true;
}

static const char* get_mapped_name(const pulse_renderer_state* state, EPulseRendererPropertyType type) {
    if ((int)type >= 0 && (int)type < PULSE_RENDERER_PROPERTY_TYPE_COUNT && state->property_names[(int)type])
        return state->property_names[(int)type];
    return nullptr;
}

static uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

static GpuBlockRef alloc_gpu_block(RendererView& view, UboBlockCache& ubo_cache, uint32_t frame_index, uint32_t size, uint32_t ubo_alignment) {
    size = align_up(size, ubo_alignment);
    for (size_t i = 0; i < view.blocks.size(); ++i) {
        size_t index = view.blocks.size() - i - 1;
        auto& block = view.blocks[index];
        if (block.size >= block.used + size) {
            auto offset = block.used;
            auto ptr = block.cpu_data + offset;
            block.used += size;
            return { index, offset, size, ptr };
        }
    }

    view.blocks.emplace_back();
    auto& block = view.blocks.back();
    block.cpu_data = ubo_cache.acquire(size, frame_index, block.size);
    block.used = 0;
    auto offset = block.used;
    auto ptr = block.cpu_data + offset;
    block.used += size;
    return { view.blocks.size() - 1, offset, size, ptr };
}

static std::optional<size_t> find_cached_ubo_column(RendererView& view, const DrawItem& item, const PulseUboInfo& info) {
    for (size_t i = 0; i < view.ubo_columns.size(); ++i) {
        auto& col = view.ubo_columns[i];
        if (col.layout_hash == info.layout_hash && !info.per_draw
            && ((!info.material_managed) || (col.material.index == item.material.index && col.material.generation == item.material.generation))) {
            return i;
        }
    }
    return {};
}

static size_t build_ubo_column(FeaturePrepareContext& ctx, DrawItem& item, uint32_t ubo_info_index, const PulseUboInfo& info) {
    pulse_renderer_state* state = ctx.state;
    RendererView& view = ctx.view();
    PulseShaderHandle shader = item.shader;

    RendererUboColumn col = {};
    col.material = item.material;
    col.shader = shader;
    col.ubo_info_index = ubo_info_index;
    col.layout_hash = info.layout_hash;
    col.set = info.set;
    col.binding = info.binding;

    if (!info.per_draw) {
        auto cached = find_cached_ubo_column(view, item, info);
        if (cached.has_value()) {
            col.block_ref = view.ubo_columns[cached.value()].block_ref;
            view.ubo_columns.push_back(std::move(col));
            return view.ubo_columns.size() - 1;
        }
    }

    col.block_ref = alloc_gpu_block(view, state->write_packet().ubo_cache, state->frame_index, info.size, state->ubo_alignment);

    if (info.material_managed) {
        auto mat_ubo_data = pulse_material_get_ubo_column(ctx.app, item.material, ubo_info_index);
        memcpy(col.block_ref.ptr, mat_ubo_data, info.size);
    }

    for (uint32_t p = 0; p < pulse_shader_get_shader_property_count(ctx.app, shader); ++p) {
        const auto& prop = pulse_shader_get_shader_property(ctx.app, shader, p);
        if (!prop.name || prop.set != info.set || prop.binding != info.binding) continue;
        if (prop.role != PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL) continue;

        if (prop.type == PULSE_SHADER_PROPERTY_TYPE_MAT4 && strcmp(prop.name, get_mapped_name(state, PULSE_RENDERER_PROPERTY_TYPE_VP_MATRIX)) == 0) {
            memcpy(col.block_ref.ptr + prop.offset, &ctx.vp, prop.size);
        } else if (prop.type == PULSE_SHADER_PROPERTY_TYPE_MAT4 && strcmp(prop.name, get_mapped_name(state, PULSE_RENDERER_PROPERTY_TYPE_MODEL_MATRIX)) == 0) {
            memcpy(col.block_ref.ptr + prop.offset, &item.world_matrix, prop.size);
        }
    }

    view.ubo_columns.push_back(std::move(col));
    return view.ubo_columns.size() - 1;
}

static void renderable_feature_extract(PulseAppId app, ecs_world_t* world, FeatureExtractContext& ctx, void* userdata) {
    auto* ud = static_cast<renderable_feature_userdata*>(userdata);
    if (!ud || !ud->query) return;

    ecs_iter_t it = ecs_query_iter(world, ud->query);
    while (ecs_query_next(&it)) {
        PulseRenderable* renderables = ecs_field(&it, PulseRenderable, 0);
        PulseWorldTransform* world_transforms = ecs_field(&it, PulseWorldTransform, 1);
        for (int i = 0; i < it.count; ++i) {
            DrawItem item = {};
            item.entity = it.entities[i];
            item.mesh = renderables[i].mesh;
            item.material = renderables[i].material;
            item.shader = pulse_material_get_shader(app, renderables[i].material);
            item.world_matrix = world_transforms[i].value;
            if (renderable_accepts_list(item, ctx.list_id)) {
                ctx.submit(item);
            }
        }
    }
    if (it.flags & EcsIterIsValid) {
        ecs_iter_fini(&it);
    }
}

static void renderable_feature_prepare(FeaturePrepareContext& ctx, void* userdata) {
    for (auto& item : ctx.items()) {
        if (item.feature_id != ctx.feature_id) continue;
        if (item.shader.index == 0) continue;
        size_t ubo_start = 0, ubo_count = 0;
        for (uint32_t u = 0; u < pulse_shader_get_ubo_info_count(ctx.app, item.shader); ++u) {
            const auto& info = pulse_shader_get_ubo_info(ctx.app, item.shader, u);
            if (!info.renderer_managed) continue;
            size_t buffer_alloc = build_ubo_column(ctx, item, u, info);
            if (ubo_count == 0) ubo_start = buffer_alloc;
            ++ubo_count;
        }
        item.ubo_start = ubo_start;
        item.ubo_end = ubo_start + ubo_count;
    }
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

    state->register_feature("Renderable", renderable_feature_extract, renderable_feature_prepare, nullptr, 0, ud);
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
