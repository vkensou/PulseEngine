#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

static void destroy_mesh(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_mesh_data_t* data = static_cast<pulse_mesh_data_t*>(ptr);
    if (data->p_vertex_attributes) {
        delete[] data->p_vertex_attributes;
        data->p_vertex_attributes = nullptr;
    }
    data->vertex_buffer = nullptr;
    data->index_buffer = nullptr;
}

void register_mesh_type(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_type_desc type_desc{};
    type_desc.struct_size = sizeof(pulse_asset_type_desc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_MESH;
    type_desc.size = sizeof(pulse_mesh_data_t);
    type_desc.align = alignof(pulse_mesh_data_t);
    type_desc.destroy = destroy_mesh;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_type(app, &type_desc);
}

}

extern "C" {

void pulse_graphics_mesh_update_vertices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count) {
    pulse_graphics_internal::pulse_graphics_state* st = pulse_graphics_internal::state_from_app(app);
    if (st && mesh) {
        pulse_graphics_mesh_ref ref{};
        if (pulse_graphics_mesh_acquire(app, *mesh, &ref)) {
            auto* m = ref.ptr;
            pulse_graphics_internal::UploadEntry entry{};
            entry.content = pulse_graphics_internal::UPLOAD_BUFFER_DATA;
            entry.buffer_data = m->vertex_buffer;
            entry.data = data;
            entry.data_size = count;
            st->dynamic_updates.push_back(entry);
            pulse_graphics_mesh_release(app, &ref);
        }
    }
}

void pulse_graphics_mesh_update_indices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count) {
    pulse_graphics_internal::pulse_graphics_state* st = pulse_graphics_internal::state_from_app(app);
    if (st && mesh) {
        pulse_graphics_mesh_ref ref{};
        if (pulse_graphics_mesh_acquire(app, *mesh, &ref)) {
            auto* m = ref.ptr;
            pulse_graphics_internal::UploadEntry entry{};
            entry.content = pulse_graphics_internal::UPLOAD_BUFFER_DATA;
            entry.buffer_data = m->index_buffer;
            entry.data = data;
            entry.data_size = count;
            st->dynamic_updates.push_back(entry);
            pulse_graphics_mesh_release(app, &ref);
        }
    }
}

bool pulse_graphics_mesh_acquire(pulse_app_t app, pulse_mesh_t handle, pulse_graphics_mesh_ref* mesh_ref) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, pulse_graphics_mesh_to_handle(handle), &ref)) {
        mesh_ref->handle = handle;
        mesh_ref->ptr = static_cast<pulse_mesh_data_t*>(ref.ptr);
        return true;
    }

    mesh_ref->handle = {};
    mesh_ref->ptr = nullptr;
    return false;
}

void pulse_graphics_mesh_release(pulse_app_t app, pulse_graphics_mesh_ref* mesh_ref) {
    pulse_asset_ref ref{ pulse_graphics_mesh_to_handle(mesh_ref->handle), nullptr };
    pulse_asset_release(app, &ref);
    mesh_ref->handle = {};
    mesh_ref->ptr = nullptr;
}

} // extern "C"
