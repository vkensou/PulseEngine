#include "graphic_internal.h"
#include "rendergraph.h"
#include "renderer.h"

namespace pulse_graphic_internal {

static void upload_record_callback(pulse_app_t app, pulse_rendergraph_t* graph, void* user_data) {
    (void)user_data;
    pulse_graphic_state* st = state_from_app(app);
    if (!st || !graph) return;

    // Free previous frame's staging data (fence waited at begin_frame of this frame)
    st->staging_pool[1 - st->staging_write].clear();

    for (auto& entry : st->pending_uploads) {
        bool done = false;

        switch (entry.content) {
        case UPLOAD_TEXTURE: {
            pulse_asset_ref ref{};
            if (pulse_asset_acquire(app, entry.texture.asset, &ref)) {
                auto* tex = static_cast<pulse_texture_data_t*>(ref.ptr);
                auto tex_rh = pulse_rendergraph_import_texture(graph, tex);
                if (entry.data && entry.data_size > 0) {
                    pulse_rendergraph_add_uploadtexturepass_ex(
                        graph, "tex_up", tex_rh,
                        0, 0, entry.data_size, 0,
                        const_cast<void*>(entry.data), nullptr, 0, nullptr);
                }
                pulse_asset_release(app, &ref);
                done = true;
            }
            break;
        }
        case UPLOAD_BUFFER: {
            pulse_asset_ref ref{};
            if (pulse_asset_acquire(app, entry.buffer.asset, &ref)) {
                auto* buf = static_cast<pulse_buffer_data_t*>(ref.ptr);
                auto buf_rh = pulse_rendergraph_import_buffer(graph, buf);
                if (entry.data && entry.data_size > 0) {
                    pulse_rendergraph_add_uploadbufferpass_ex(
                        graph, "buf_up", buf_rh,
                        entry.data_size, 0,
                        const_cast<void*>(entry.data), nullptr, 0, nullptr);
                }
                pulse_asset_release(app, &ref);
                done = true;
            }
            break;
        }
        case UPLOAD_TEXTURE_DATA: {
            if (entry.texture_data) {
                auto tex_rh = pulse_rendergraph_import_texture(graph, entry.texture_data);
                if (entry.data && entry.data_size > 0) {
                    pulse_rendergraph_add_uploadtexturepass_ex(
                        graph, "tex_up", tex_rh,
                        0, 0, entry.data_size, 0,
                        const_cast<void*>(entry.data), nullptr, 0, nullptr);
                }
                done = true;
            }
            break;
        }
        case UPLOAD_BUFFER_DATA: {
            if (entry.buffer_data) {
                auto buf_rh = pulse_rendergraph_import_buffer(graph, entry.buffer_data);
                if (entry.data && entry.data_size > 0) {
                    pulse_rendergraph_add_uploadbufferpass_ex(
                        graph, "bdata_up", buf_rh,
                        entry.data_size, 0,
                        const_cast<void*>(entry.data), nullptr, 0, nullptr);
                }
                done = true;
            }
            break;
        }
        }

        if (done && entry.completed) {
            *entry.completed = true;
        }
    }
    st->pending_uploads.clear();

    for (auto& entry : st->dynamic_updates) {
        pulse_asset_ref ref{};
        if (entry.content == UPLOAD_BUFFER && pulse_asset_acquire(app, entry.buffer.asset, &ref)) {
            auto* buf = static_cast<pulse_buffer_data_t*>(ref.ptr);
            (void)pulse_rendergraph_import_buffer(graph, buf);
            pulse_asset_release(app, &ref);
        } else if (entry.content == UPLOAD_BUFFER_DATA && entry.buffer_data) {
            (void)pulse_rendergraph_import_buffer(graph, entry.buffer_data);
        }
    }
    st->dynamic_updates.clear();

    // Swap staging pool for next frame
    st->staging_write = 1 - st->staging_write;
}

void install_upload_callback(pulse_app_t app) {
    pulse_cgpu_renderer_record_callback_desc desc{};
    desc.callback = upload_record_callback;
    desc.user_data = nullptr;
    desc.priority = -1000;
    pulse_cgpu_render_add_record_callback(app, &desc);
}

} // namespace pulse_graphic_internal
