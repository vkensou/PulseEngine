#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

static void destroy_mesh(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    PulseMeshData* data = static_cast<PulseMeshData*>(ptr);
    if (data->p_vertex_attributes) {
        delete[] data->p_vertex_attributes;
        data->p_vertex_attributes = nullptr;
    }
    data->vertex_buffer = nullptr;
    data->index_buffer = nullptr;
}

void register_mesh_type(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_MESH;
    type_desc.size = sizeof(PulseMeshData);
    type_desc.align = alignof(PulseMeshData);
    type_desc.destroy = destroy_mesh;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(pulse_get_asset_system(app), &type_desc);
}

}

extern "C" {

void pulse_update_mesh_vertices(PulseAppId app, PulseMeshHandle* mesh, const void* data, uint32_t count) {
    pulse_graphics_internal::pulse_graphics_state* st = pulse_graphics_internal::state_from_app(app);
    if (st && mesh) {
        PulseMesh ref{};
        if (pulse_acquire_mesh(app, *mesh, &ref)) {
            auto* m = ref.ptr;
            pulse_graphics_internal::UploadEntry entry{};
            entry.content = pulse_graphics_internal::UPLOAD_BUFFER_DATA;
            entry.buffer_data = m->vertex_buffer;
            entry.data = data;
            entry.data_size = count;
            st->dynamic_updates.push_back(entry);
            pulse_release_mesh(app, &ref);
        }
    }
}

void pulse_update_mesh_indices(PulseAppId app, PulseMeshHandle* mesh, const void* data, uint32_t count) {
    pulse_graphics_internal::pulse_graphics_state* st = pulse_graphics_internal::state_from_app(app);
    if (st && mesh) {
        PulseMesh ref{};
        if (pulse_acquire_mesh(app, *mesh, &ref)) {
            auto* m = ref.ptr;
            pulse_graphics_internal::UploadEntry entry{};
            entry.content = pulse_graphics_internal::UPLOAD_BUFFER_DATA;
            entry.buffer_data = m->index_buffer;
            entry.data = data;
            entry.data_size = count;
            st->dynamic_updates.push_back(entry);
            pulse_release_mesh(app, &ref);
        }
    }
}

bool pulse_acquire_mesh(PulseAppId app, PulseMeshHandle handle, PulseMesh* mesh_ref) {
    PulseAssetRef ref{};
    if (pulse_asset_system_acquire(pulse_get_asset_system(app), pulse_mesh_to_handle(handle), &ref)) {
        mesh_ref->handle = handle;
        mesh_ref->ptr = static_cast<PulseMeshData*>(ref.ptr);
        return true;
    }

    mesh_ref->handle = {};
    mesh_ref->ptr = nullptr;
    return false;
}

void pulse_release_mesh(PulseAppId app, PulseMesh* mesh_ref) {
    PulseAssetRef ref{ pulse_mesh_to_handle(mesh_ref->handle), nullptr };
    pulse_asset_system_release(pulse_get_asset_system(app), &ref);
    mesh_ref->handle = {};
    mesh_ref->ptr = nullptr;
}

} // extern "C"
