#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

static constexpr int kMaxUploadsPerFrame = 10;

static void upload_record_callback(PulseAppId app, PulseRenderGraphId graph, void* user_data) {
    (void)user_data;
    pulse_graphics_state* st = state_from_app(app);
    if (!st || !graph) return;

    // Deferred release of previous frame's processed staging allocations
    for (auto& df : st->pending_release)
        st->staging_pool.deallocate(df.ptr, df.size);
    st->pending_release.clear();

    int processed = 0;
    while (!st->pending_uploads.empty() && processed < kMaxUploadsPerFrame) {
        auto& entry = st->pending_uploads.front();
        bool done = false;

        switch (entry.content) {
        case UPLOAD_TEXTURE:
        case UPLOAD_TEXTURE_DATA: {
            PulseTextureData* tex = entry.texture_data;
            if (!tex)
                tex = internal_borrow_texture(st->asset_system, entry.texture);
            if (!tex) break;

            auto tex_rh = pulse_render_graph_import_texture(graph, entry.texture);
            if (!pulse_rgtexture_handle_is_valid(tex_rh)) break;
            if (entry.data && entry.data_size > 0) {
                auto* info = tex->handle->info;
                auto mipedSize = [](uint64_t s, uint64_t m) { return std::max<uint64_t>(s >> m, 1ull); };
                uint32_t tex_comp = FormatUtil_BitSizeOfBlock(info->format) / 8;
                uint32_t blockW = FormatUtil_WidthOfBlock(info->format);
                uint32_t blockH = FormatUtil_HeightOfBlock(info->format);
                const uint8_t* src = static_cast<const uint8_t*>(entry.data);

                for (uint8_t mip = 0; mip < entry.source_mip_levels; ++mip) {
                    uint64_t mipW = mipedSize(info->width, mip);
                    uint64_t mipH = mipedSize(info->height, mip);
                    uint64_t blocksW = std::max<uint64_t>((mipW + blockW - 1) / blockW, 1ull);
                    uint64_t blocksH = std::max<uint64_t>((mipH + blockH - 1) / blockH, 1ull);
                    uint64_t mipSize = blocksW * blocksH * tex_comp;

                    for (uint32_t slice = 0; slice < info->array_size_minus_one + 1; ++slice) {
                        pulse_render_graph_add_uploadtexturepass_ex(
                            graph, "tex_up", tex_rh,
                            mip, slice, mipSize, 0,
                            const_cast<uint8_t*>(src), nullptr, 0, nullptr);
                        src += mipSize;
                    }
                }

                if (entry.generate_mipmap)
                    pulse_render_graph_add_generate_mipmap(graph, tex_rh, entry.source_mip_levels);
            }

            done = true;
            break;
        }
        case UPLOAD_BUFFER:
        case UPLOAD_BUFFER_DATA: {
            PulseGraphicsBufferData* buf = entry.buffer_data;
            if (!buf)
                buf = internal_borrow_buffer(st->asset_system, entry.buffer);
            if (!buf) break;

            PulseRGBufferHandle buf_rh = {};
            if (pulse_asset_handle_is_valid(pulse_graphics_buffer_to_handle(entry.buffer)))
                buf_rh = pulse_render_graph_import_buffer(graph, entry.buffer);
            else
                buf_rh = pulse_render_graph_import_dynamic_buffer(graph, buf);
            if (!pulse_rgbuffer_handle_is_valid(buf_rh)) break;
            if (entry.data && entry.data_size > 0) {
                pulse_render_graph_add_uploadbufferpass_ex(
                    graph, "buf_up", buf_rh,
                    entry.data_size, 0,
                    const_cast<void*>(entry.data), nullptr, 0, nullptr);
            }

            done = true;
            break;
        }
        }

        if (done && entry.completed) {
            *entry.completed = true;
        }

        // Release staging memory next frame (GPU fence will have passed by then)
        if (entry.data && entry.data_size > 0)
            st->pending_release.push_back({const_cast<void*>(entry.data), entry.data_size});

        st->pending_uploads.pop_front();
        ++processed;
    }

    for (auto& entry : st->dynamic_updates) {
        if (entry.content == UPLOAD_BUFFER || entry.content == UPLOAD_BUFFER_DATA) {
            PulseGraphicsBufferData* buf = entry.buffer_data;
            if (!buf)
                buf = internal_borrow_buffer(st->asset_system, entry.buffer);
            if (!buf) continue;
            if (pulse_asset_handle_is_valid(pulse_graphics_buffer_to_handle(entry.buffer)))
                (void)pulse_render_graph_import_buffer(graph, entry.buffer);
            else
                (void)pulse_render_graph_import_dynamic_buffer(graph, buf);
        }
    }
    st->dynamic_updates.clear();
}

uint8_t* queue_staging_texture_full(
    pulse_graphics_state* gstate,
    PulseTextureHandle handle,
    PulseTextureData* texture,
    uint8_t source_mip_levels,
    bool generate_mipmap,
    uint64_t* out_size,
    bool* completed)
{
    auto* info = texture->handle->info;
    uint32_t tex_comp = FormatUtil_BitSizeOfBlock(info->format) / 8;
    uint32_t blockW = FormatUtil_WidthOfBlock(info->format);
    uint32_t blockH = FormatUtil_HeightOfBlock(info->format);
    auto mipedSize = [](uint64_t s, uint64_t m) { return std::max<uint64_t>(s >> m, 1ull); };

    uint64_t totalSize = 0;
    for (uint8_t mip = 0; mip < source_mip_levels; ++mip) {
        uint64_t mipW = mipedSize(info->width, mip);
        uint64_t mipH = mipedSize(info->height, mip);
        uint64_t blocksW = std::max<uint64_t>((mipW + blockW - 1) / blockW, 1ull);
        uint64_t blocksH = std::max<uint64_t>((mipH + blockH - 1) / blockH, 1ull);
        totalSize += blocksW * blocksH * tex_comp * (info->array_size_minus_one + 1);
    }

    auto* ptr = static_cast<uint8_t*>(gstate->staging_pool.allocate(totalSize, alignof(std::max_align_t)));

    gstate->pending_uploads.push_back({
        UPLOAD_TEXTURE_DATA, handle, {},
        texture, {},
        ptr, totalSize,
        completed,
        source_mip_levels,
        generate_mipmap
    });

    if (out_size) *out_size = totalSize;
    return ptr;
}

uint8_t* queue_staging_buffer_full(
    pulse_graphics_state* gstate,
    PulseGraphicsBufferHandle handle,
    PulseGraphicsBufferData* buffer,
    uint64_t size,
    bool* completed)
{
    auto* ptr = static_cast<uint8_t*>(gstate->staging_pool.allocate(size, alignof(std::max_align_t)));

    gstate->pending_uploads.push_back({
        UPLOAD_BUFFER_DATA, {}, handle,
        nullptr, buffer,
        ptr, size,
        completed
    });

    return ptr;
}

void install_upload_callback(PulseAppId app) {
    PulseRenderRecordCallbackDesc desc{};
    desc.callback = upload_record_callback;
    desc.user_data = nullptr;
    desc.priority = -1000;
    pulse_add_render_record_callback(app, &desc);
}

bool is_upload_pending(PulseAppId app, PulseAssetHandle handle) {
    pulse_graphics_state* state = state_from_app(app);
    if (!state) return false;
    if (!pulse_asset_handle_is_valid(handle)) return false;
    for (const auto& entry : state->pending_uploads) {
        if (entry.content == UPLOAD_TEXTURE && pulse_asset_handle_equals(pulse_texture_to_handle(entry.texture), handle)) return true;
        if (entry.content == UPLOAD_BUFFER && pulse_asset_handle_equals(pulse_graphics_buffer_to_handle(entry.buffer), handle)) return true;
    }
    return false;
}

void clear_upload_pending(PulseAppId app, PulseAssetHandle handle) {
    pulse_graphics_state* state = state_from_app(app);
    if (!state) return;
    if (!pulse_asset_handle_is_valid(handle)) return;
    auto& vec = state->pending_uploads;
    for (size_t i = 0; i < vec.size(); ) {
        bool match = false;
        if (vec[i].content == UPLOAD_TEXTURE && pulse_asset_handle_equals(pulse_texture_to_handle(vec[i].texture), handle)) match = true;
        if (vec[i].content == UPLOAD_BUFFER && pulse_asset_handle_equals(pulse_graphics_buffer_to_handle(vec[i].buffer), handle)) match = true;
        if (match) {
            vec.erase(vec.begin() + i);
        } else {
            ++i;
        }
    }
}

} // namespace pulse_graphics_internal
