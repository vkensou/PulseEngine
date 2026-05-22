#include "graphic_internal.h"
#include "rendergraph.h"
#include "renderer.h"

namespace pulse_graphic_internal {

static void upload_record_callback(pulse_app_t app, pulse_rendergraph_t* graph, void* user_data) {
    (void)user_data;
    pulse_graphic_state* st = state_from_app(app);
    if (!st || !graph) return;

    for (auto& entry : st->pending_uploads) {
        pulse_asset_ref ref{};
        if (!pulse_asset_acquire(app, entry.handle, &ref)) continue;

        if (entry.is_texture) {
            pulse_texture_data_t* tex = static_cast<pulse_texture_data_t*>(ref.ptr);
            pulse_texture_handle_t tex_handle = pulse_rendergraph_import_texture(graph, (void*)tex->handle);
            (void)tex_handle;
        } else {
            pulse_mesh_data_t* mesh = static_cast<pulse_mesh_data_t*>(ref.ptr);
            pulse_buffer_handle_t vbuf = pulse_rendergraph_import_buffer(graph, (void*)mesh->vertex_buffer);
            (void)vbuf;
        }
        pulse_asset_release(app, &ref);
    }
    st->pending_uploads.clear();

    for (auto& entry : st->dynamic_updates) {
        pulse_asset_ref ref{};
        if (!pulse_asset_acquire(app, entry.handle, &ref)) continue;
        pulse_mesh_data_t* mesh = static_cast<pulse_mesh_data_t*>(ref.ptr);
        pulse_buffer_handle_t vbuf = pulse_rendergraph_import_buffer(graph, (void*)mesh->vertex_buffer);
        (void)vbuf;
        pulse_asset_release(app, &ref);
    }
    st->dynamic_updates.clear();
}

void install_upload_callback(pulse_app_t app) {
    pulse_cgpu_renderer_record_callback_desc desc{};
    desc.callback = upload_record_callback;
    desc.user_data = nullptr;
    desc.priority = -1000;
    pulse_cgpu_render_add_record_callback(app, &desc);
}

} // namespace pulse_graphic_internal
