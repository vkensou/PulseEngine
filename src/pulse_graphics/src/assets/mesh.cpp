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
    if (data->vertex_buffer.ptr && !data->vertex_buffer.ptr->handle) {
        HGEGraphics::free_buffer(data->vertex_buffer.ptr);
    }
    if (data->index_buffer.ptr && !data->index_buffer.ptr->handle) {
        HGEGraphics::free_buffer(data->index_buffer.ptr);
    }
    data->vertex_buffer = {};
    data->index_buffer = {};
}

void register_mesh_type(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_MESH;
    type_desc.size = sizeof(PulseMeshData);
    type_desc.align = alignof(PulseMeshData);
    type_desc.destroy = destroy_mesh;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(asset_system, &type_desc);
}

}

extern "C" {

void pulse_update_mesh_vertices(PulseAppId app, PulseMeshHandle mesh, const void* data, uint32_t count) {
    pulse_graphics_internal::pulse_graphics_state* st = pulse_graphics_internal::state_from_app(app);
    if (st) {
        PulseMeshData* m = pulse_graphics_internal::internal_borrow_mesh(pulse_graphics_internal::asset_system_from_app(app), mesh);
        if (m) {
            pulse_graphics_internal::UploadEntry entry{};
            entry.content = pulse_graphics_internal::UPLOAD_BUFFER_DATA;
            entry.buffer = m->vertex_buffer.handle;
            entry.buffer_data = m->vertex_buffer.ptr;
            entry.data = data;
            entry.data_size = count * m->vertex_stride;
            st->dynamic_updates.push_back(entry);
        }
    }
}

void pulse_update_mesh_indices(PulseAppId app, PulseMeshHandle mesh, const void* data, uint32_t count) {
    pulse_graphics_internal::pulse_graphics_state* st = pulse_graphics_internal::state_from_app(app);
    if (st) {
        PulseMeshData* m = pulse_graphics_internal::internal_borrow_mesh(pulse_graphics_internal::asset_system_from_app(app), mesh);
        if (m) {
            pulse_graphics_internal::UploadEntry entry{};
            entry.content = pulse_graphics_internal::UPLOAD_BUFFER_DATA;
            entry.buffer = m->index_buffer.handle;
            entry.buffer_data = m->index_buffer.ptr;
            entry.data = data;
            entry.data_size = count * m->index_stride;
            st->dynamic_updates.push_back(entry);
        }
    }
}

} // extern "C"
