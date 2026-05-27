#include "graphic_internal.h"
#include "rendergraph.h"
#include "renderer.h"

namespace pulse_graphic_internal {

static constexpr int kMaxUploadsPerFrame = 10;

static void upload_record_callback(pulse_app_t app, pulse_rendergraph_t* graph, void* user_data) {
    (void)user_data;
    pulse_graphic_state* st = state_from_app(app);
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
            pulse_texture_data_t* tex = entry.texture_data;
            pulse_asset_ref ref{};
            if (!tex && pulse_asset_handle_is_valid(pulse_graphic_texture_to_handle(entry.texture))) {
                if (pulse_asset_acquire(app, pulse_graphic_texture_to_handle(entry.texture), &ref))
                    tex = static_cast<pulse_texture_data_t*>(ref.ptr);
            }
            if (!tex) break;

            auto tex_rh = pulse_rendergraph_import_texture(graph, tex);
            if (entry.data && entry.data_size > 0) {
                auto* info = tex->handle->info;
                auto mipedSize = [](uint64_t s, uint64_t m) { return std::max<uint64_t>(s >> m, 1ull); };
                uint32_t tex_comp = FormatUtil_BitSizeOfBlock(info->format) / 8;
                const uint8_t* src = static_cast<const uint8_t*>(entry.data);

                for (uint8_t mip = 0; mip < entry.source_mip_levels; ++mip) {
                    uint64_t mipW = mipedSize(info->width, mip);
                    uint64_t mipH = mipedSize(info->height, mip);
                    uint64_t mipSize = mipW * mipH * tex_comp;

                    for (uint32_t slice = 0; slice < info->array_size_minus_one + 1; ++slice) {
                        pulse_rendergraph_add_uploadtexturepass_ex(
                            graph, "tex_up", tex_rh,
                            mip, slice, mipSize, 0,
                            const_cast<uint8_t*>(src), nullptr, 0, nullptr);
                        src += mipSize;
                    }
                }

                if (entry.generate_mipmap)
                    pulse_rendergraph_add_generate_mipmap(graph, tex_rh, entry.source_mip_levels);
            }

            if (ref.ptr) pulse_asset_release(app, &ref);
            done = true;
            break;
        }
        case UPLOAD_BUFFER:
        case UPLOAD_BUFFER_DATA: {
            pulse_buffer_data_t* buf = entry.buffer_data;
            pulse_asset_ref ref{};
            if (!buf && pulse_asset_handle_is_valid(entry.buffer.asset)) {
                if (pulse_asset_acquire(app, entry.buffer.asset, &ref))
                    buf = static_cast<pulse_buffer_data_t*>(ref.ptr);
            }
            if (!buf) break;

            auto buf_rh = pulse_rendergraph_import_buffer(graph, buf);
            if (entry.data && entry.data_size > 0) {
                pulse_rendergraph_add_uploadbufferpass_ex(
                    graph, "buf_up", buf_rh,
                    entry.data_size, 0,
                    const_cast<void*>(entry.data), nullptr, 0, nullptr);
            }

            if (ref.ptr) pulse_asset_release(app, &ref);
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
            pulse_buffer_data_t* buf = entry.buffer_data;
            pulse_asset_ref ref{};
            if (!buf && pulse_asset_handle_is_valid(entry.buffer.asset)) {
                if (pulse_asset_acquire(app, entry.buffer.asset, &ref))
                    buf = static_cast<pulse_buffer_data_t*>(ref.ptr);
            }
            if (buf) {
                (void)pulse_rendergraph_import_buffer(graph, buf);
            }
            if (ref.ptr) pulse_asset_release(app, &ref);
        }
    }
    st->dynamic_updates.clear();
}

uint8_t* queue_staging_texture_full(
    pulse_graphic_state* gstate,
    pulse_texture_data_t* texture,
    uint8_t source_mip_levels,
    bool generate_mipmap,
    uint64_t* out_size,
    bool* completed)
{
    auto* info = texture->handle->info;
    uint32_t tex_comp = FormatUtil_BitSizeOfBlock(info->format) / 8;
    auto mipedSize = [](uint64_t s, uint64_t m) { return std::max<uint64_t>(s >> m, 1ull); };

    uint64_t totalSize = 0;
    for (uint8_t mip = 0; mip < source_mip_levels; ++mip) {
        uint64_t mipW = mipedSize(info->width, mip);
        uint64_t mipH = mipedSize(info->height, mip);
        totalSize += mipW * mipH * tex_comp * (info->array_size_minus_one + 1);
    }

    auto* ptr = static_cast<uint8_t*>(gstate->staging_pool.allocate(totalSize, alignof(std::max_align_t)));

    gstate->pending_uploads.push_back({
        UPLOAD_TEXTURE_DATA, {}, {},
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
    pulse_graphic_state* gstate,
    pulse_buffer_data_t* buffer,
    uint64_t size,
    bool* completed)
{
    auto* ptr = static_cast<uint8_t*>(gstate->staging_pool.allocate(size, alignof(std::max_align_t)));

    gstate->pending_uploads.push_back({
        UPLOAD_BUFFER_DATA, {}, {},
        nullptr, buffer,
        ptr, size,
        completed
    });

    return ptr;
}

void install_upload_callback(pulse_app_t app) {
    pulse_cgpu_renderer_record_callback_desc desc{};
    desc.callback = upload_record_callback;
    desc.user_data = nullptr;
    desc.priority = -1000;
    pulse_cgpu_render_add_record_callback(app, &desc);
}

} // namespace pulse_graphic_internal
