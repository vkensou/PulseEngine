#include "renderer_internal.h"

namespace pulse_renderer_internal {

static uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

GpuBlockRef alloc_ubo_block(pulse_renderer_state& state, ViewFrameData& view, uint32_t size) {
    const uint32_t aligned_size = align_up(size, state.ubo_alignment);
    for (size_t i = 0; i < view.blocks.size(); ++i) {
        size_t index = view.blocks.size() - i - 1;
        auto& block = view.blocks[index];
        if (block.size >= block.used + aligned_size) {
            auto offset = block.used;
            block.used += aligned_size;
            return { index, offset, aligned_size, block.cpu_data + offset };
        }
    }

    view.blocks.emplace_back();
    auto& block = view.blocks.back();
    block.cpu_data = state.ubo_cache.acquire(aligned_size, state.frame_index, block.size);
    block.used = aligned_size;
    return { view.blocks.size() - 1, 0, aligned_size, block.cpu_data };
}

uint32_t alloc_feature_ubo_column(FeaturePrepareContext& ctx, DrawItem& item, uint32_t set, uint32_t binding, uint32_t size) {
    RendererUboColumn col = {};
    col.set = set;
    col.binding = binding;
    col.block_ref = alloc_ubo_block(*ctx.state, ctx.view, size);
    ctx.view.ubo_columns.push_back(col);

    const uint32_t index = (uint32_t)ctx.view.ubo_columns.size() - 1;
    if (item.feature_column_count == 0) item.feature_column_start = index;
    ++item.feature_column_count;
    return index;
}

static void bind_ubo_column(PulseRenderPassEncoder* encoder, const ViewFrameData& view, const RendererUboColumn& col) {
    const auto& block = view.blocks[col.block_ref.index];
    pulse_render_pass_encoder_set_global_buffer_offset(encoder, block.gpu_handle, col.set, col.binding, col.block_ref.offset, col.block_ref.size);
}

void bind_item_ubo_columns(PulseRenderPassEncoder* encoder, const ViewFrameData& view, const DrawItem& item) {
    for (uint32_t i = item.global_column_start; i < item.global_column_start + item.global_column_count; ++i) {
        bind_ubo_column(encoder, view, view.ubo_columns[i]);
    }
    for (uint32_t i = item.feature_column_start; i < item.feature_column_start + item.feature_column_count; ++i) {
        bind_ubo_column(encoder, view, view.ubo_columns[i]);
    }
}

} // namespace pulse_renderer_internal
