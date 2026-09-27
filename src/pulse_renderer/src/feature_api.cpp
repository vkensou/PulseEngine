#include "renderer_internal.h"

namespace pulse_renderer_internal {

PulseFeatureItem to_public_feature_item(const StagingItem& staging) {
    PulseFeatureItem item = {};
    item.entity = staging.entity;
    item.mesh = staging.mesh;
    item.material = staging.material;
    item.shader = staging.shader;
    item.world_matrix = staging.world_matrix;
    item.instance_count = staging.instance_count;
    return item;
}

const void* feature_item_data(const pulse_renderer_state* state, const FrameRenderPacket* snapshot, uint32_t feature_id, uint32_t data_slot) {
    if (!state || !snapshot || feature_id >= state->features.size()) return nullptr;
    const size_t size = state->features[feature_id].data_size;
    if (size == 0) return nullptr;
    const FeatureStaging& staging = snapshot->staging[feature_id];
    if ((size_t)(data_slot + 1) * size > staging.data_arena.size()) return nullptr;
    return staging.data_arena.data() + (size_t)data_slot * size;
}

} // namespace pulse_renderer_internal

using namespace pulse_renderer_internal;

extern "C" {

EPulseResult pulse_add_render_feature(PulseAppId app, const PulseRenderFeatureDesc* desc) {
    if (!app || !desc) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    if (desc->struct_size != sizeof(PulseRenderFeatureDesc) || desc->version != PULSE_RENDER_FEATURE_DESC_VERSION) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    if (!desc->name || !desc->extract || !desc->draw) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    pulse_renderer_state* state = state_from_app(app);
    if (!state) return PULSE_RESULT_ERROR_INVALID_STATE;
    for (const PulseRenderFeatureDesc& existing : state->features) {
        if (existing.name && strcmp(existing.name, desc->name) == 0) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (state->features.size() >= 0xffff) return PULSE_RESULT_ERROR_INTERNAL;
    state->features.push_back(*desc);
    return PULSE_RESULT_OK;
}

EPulseResult pulse_remove_render_feature(PulseAppId app, const char* name) {
    if (!app || !name) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    pulse_renderer_state* state = state_from_app(app);
    if (!state) return PULSE_RESULT_ERROR_INVALID_STATE;
    for (size_t i = 0; i < state->features.size(); ++i) {
        if (!state->features[i].name || strcmp(state->features[i].name, name) != 0) continue;
        PulseRenderFeatureDesc desc = state->features[i];
        state->features.erase(state->features.begin() + i);
        if (desc.destroy) desc.destroy(desc.userdata);
        return PULSE_RESULT_OK;
    }
    return PULSE_RESULT_ERROR_NOT_FOUND;
}

void pulse_feature_extract_submit(PulseFeatureExtractContext* ctx, const PulseFeatureItem* item, const void* data) {
    if (!ctx || !ctx->state || !item) return;
    pulse_renderer_state* state = ctx->state;
    const uint32_t feature_id = ctx->feature_id;
    if (feature_id >= state->features.size()) return;
    const size_t data_size = state->features[feature_id].data_size;
    StagingItem copy = {};
    copy.entity = item->entity;
    copy.mesh = item->mesh;
    copy.material = item->material;
    copy.shader = item->shader;
    copy.world_matrix = item->world_matrix;
    copy.instance_count = item->instance_count;
    FeatureStaging& staging = state->write_packet().staging[feature_id];
    if (data_size) {
        copy.data_slot = (uint32_t)(staging.data_arena.size() / data_size);
        staging.data_arena.resize(staging.data_arena.size() + data_size);
        if (data) memcpy(staging.data_arena.data() + copy.data_slot * data_size, data, data_size);
    }
    staging.items.push_back(copy);
}

bool pulse_feature_cull_get_camera(PulseFeatureCullContext* ctx, PulseFeatureCullCamera* camera) {
    if (!ctx || !ctx->camera || !camera) return false;
    const CameraSnapshot& snap = *ctx->camera;
    camera->camera_entity = snap.camera_entity;
    camera->view_index = ctx->view_index;
    camera->view_matrix = snap.view_matrix;
    camera->proj_matrix = snap.proj_matrix;
    camera->orthographic = snap.orthographic;
    camera->fov = snap.fov;
    camera->orthographic_size = snap.orthographic_size;
    camera->near_plane = snap.near_plane;
    camera->far_plane = snap.far_plane;
    camera->width = snap.width;
    camera->height = snap.height;
    return true;
}

uint32_t pulse_feature_prepare_item_count(PulseFeaturePrepareContext* ctx) {
    if (!ctx || !ctx->list) return 0;
    return (uint32_t)ctx->list->items.size();
}

bool pulse_feature_prepare_get_item(PulseFeaturePrepareContext* ctx, uint32_t index, PulseFeatureItem* item) {
    if (!ctx || !ctx->list || !ctx->snapshot || !item) return false;
    if (index >= ctx->list->items.size()) return false;
    const DrawItem& draw = ctx->list->items[index];
    if (draw.feature_id != ctx->feature_id) return false;
    *item = to_public_feature_item(ctx->snapshot->staging[draw.feature_id].items[draw.staging_index]);
    return true;
}

const void* pulse_feature_prepare_item_data(PulseFeaturePrepareContext* ctx, uint32_t index) {
    if (!ctx || !ctx->list || index >= ctx->list->items.size()) return nullptr;
    const DrawItem& draw = ctx->list->items[index];
    if (draw.feature_id != ctx->feature_id) return nullptr;
    return feature_item_data(ctx->state, ctx->snapshot, ctx->feature_id, draw.data_slot);
}

void* pulse_feature_prepare_alloc_ubo(PulseFeaturePrepareContext* ctx, uint32_t index, uint32_t set, uint32_t binding, uint32_t size) {
    if (!ctx || !ctx->state || !ctx->view || !ctx->list) return nullptr;
    if (index >= ctx->list->items.size()) return nullptr;
    DrawItem& draw = ctx->list->items[index];
    if (draw.feature_id != ctx->feature_id) return nullptr;
    const uint32_t column = alloc_feature_ubo_column(*ctx, draw, set, binding, size);
    return ctx->view->ubo_columns[column].block_ref.ptr;
}

const PulseFeatureItem* pulse_feature_draw_get_item(PulseFeatureDrawContext* ctx) {
    if (!ctx) return nullptr;
    return &ctx->feature_item;
}

const void* pulse_feature_draw_item_data(PulseFeatureDrawContext* ctx) {
    if (!ctx || !ctx->item) return nullptr;
    return feature_item_data(ctx->state, ctx->snapshot, ctx->item->feature_id, ctx->item->data_slot);
}

void pulse_feature_draw_bind_ubo_columns(PulseRenderPassEncoder* encoder, PulseFeatureDrawContext* ctx) {
    if (!encoder || !ctx || !ctx->view || !ctx->item) return;
    bind_item_ubo_columns(encoder, *ctx->view, *ctx->item);
}

uint32_t pulse_feature_record_item_count(PulseFeatureRecordContext* ctx) {
    if (!ctx) return 0;
    return (uint32_t)ctx->items.size();
}

const void* pulse_feature_record_item_data(PulseFeatureRecordContext* ctx, uint32_t index) {
    if (!ctx || index >= ctx->items.size()) return nullptr;
    const DrawItem& draw = *ctx->items[index];
    return feature_item_data(ctx->state, ctx->snapshot, draw.feature_id, draw.data_slot);
}

} // extern "C"
