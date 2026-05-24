#include "graphic_internal.h"
#include "renderer.h"
#include <cstring>

extern "C" {

pulse_mesh_t pulse_graphic_mesh_create_from_data(
    pulse_app_t app,
    const void* vertex_data, uint32_t vertex_count, uint32_t vertex_stride,
    const void* index_data,  uint32_t index_count,  uint32_t index_stride,
    ECGPUPrimitiveTopology topology,
    const CGPUVertexLayout* layout)
{
    pulse_mesh_t result{};
    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device || !layout) return result;

    CGPUVertexLayout use_layout = *layout;
    uint32_t use_idx_stride = (index_stride == 0 && index_count > 0) ? sizeof(uint32_t) : index_stride;

    pulse_asset_handle asset_handle = pulse_asset_load_from_memory(
        app, PULSE_TYPE_MESH, "", nullptr, 0);
    if (asset_handle.index == PULSE_ASSET_INVALID_INDEX) return result;

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_mesh_data_t* mesh = static_cast<pulse_mesh_data_t*>(ref.ptr);
        HGEGraphics::init_mesh(mesh, device, vertex_count, index_count, topology, use_layout, use_idx_stride, false, false);
        pulse_asset_release(app, &ref);
    }

    if (vertex_data && vertex_count > 0) {
        auto* gstate = pulse_graphic_internal::state_from_app(app);
        if (gstate) {
            uint64_t vb_bytes = vertex_count * vertex_stride;
            pulse_asset_ref ref2{};
            if (pulse_asset_acquire(app, asset_handle, &ref2)) {
                auto* mesh = static_cast<pulse_mesh_data_t*>(ref2.ptr);
                auto* staging = pulse_graphic_internal::queue_staging_buffer_full(gstate, mesh->vertex_buffer, vb_bytes, nullptr);
                memcpy(staging, vertex_data, vb_bytes);
                pulse_asset_release(app, &ref2);
            }
        }
    }

    result.asset = asset_handle;
    return result;
}

pulse_mesh_t pulse_graphic_mesh_create_dynamic(
    pulse_app_t app,
    uint32_t max_vertex_count, uint32_t vertex_stride,
    uint32_t max_index_count,  uint32_t index_stride,
    ECGPUPrimitiveTopology topology,
    const CGPUVertexLayout* layout)
{
    pulse_mesh_t result{};
    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device || !layout) return result;

    pulse_asset_handle asset_handle = pulse_asset_load_from_memory(
        app, PULSE_TYPE_MESH, "", nullptr, 0);
    if (asset_handle.index == PULSE_ASSET_INVALID_INDEX) return result;

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_mesh_data_t* mesh = static_cast<pulse_mesh_data_t*>(ref.ptr);
        mesh->vertex_layout = *layout;
        mesh->p_vertex_attributes = new CGPUVertexAttribute[layout->attribute_count];
        std::copy(layout->p_attributes, layout->p_attributes + layout->attribute_count, mesh->p_vertex_attributes);
        mesh->vertex_layout.p_attributes = mesh->p_vertex_attributes;
        mesh->prim_topology = topology;
        mesh->vertices_count = 0;
        mesh->vertex_stride = 0;
        for (auto i = 0; i < layout->attribute_count; ++i)
        {
            mesh->vertex_stride += layout->p_attributes[i].elem_stride;
        }
        mesh->index_stride = index_stride;
        mesh->vertex_buffer = HGEGraphics::create_empty_buffer();
        mesh->index_buffer = HGEGraphics::create_empty_buffer();
        mesh->prepared = true;
        pulse_asset_release(app, &ref);
    }

    result.asset = asset_handle;
    return result;
}

void pulse_graphic_mesh_update_vertices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count) {
    pulse_graphic_internal::pulse_graphic_state* st = pulse_graphic_internal::state_from_app(app);
    if (st && mesh) {
        pulse_asset_ref ref{};
        if (pulse_asset_acquire(app, mesh->asset, &ref)) {
            auto* m = static_cast<pulse_mesh_data_t*>(ref.ptr);
            pulse_graphic_internal::UploadEntry entry{};
            entry.content = pulse_graphic_internal::UPLOAD_BUFFER_DATA;
            entry.buffer_data = m->vertex_buffer;
            entry.data = data;
            entry.data_size = count;
            st->dynamic_updates.push_back(entry);
            pulse_asset_release(app, &ref);
        }
    }
}

void pulse_graphic_mesh_update_indices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count) {
    pulse_graphic_internal::pulse_graphic_state* st = pulse_graphic_internal::state_from_app(app);
    if (st && mesh) {
        pulse_asset_ref ref{};
        if (pulse_asset_acquire(app, mesh->asset, &ref)) {
            auto* m = static_cast<pulse_mesh_data_t*>(ref.ptr);
            pulse_graphic_internal::UploadEntry entry{};
            entry.content = pulse_graphic_internal::UPLOAD_BUFFER_DATA;
            entry.buffer_data = m->index_buffer;
            entry.data = data;
            entry.data_size = count;
            st->dynamic_updates.push_back(entry);
            pulse_asset_release(app, &ref);
        }
    }
}

pulse_mesh_data_t* pulse_graphic_mesh_acquire(pulse_app_t app, pulse_mesh_t* handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, handle->asset, &ref)) {
        return static_cast<pulse_mesh_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphic_mesh_release(pulse_app_t app, pulse_mesh_t* handle) {
    pulse_asset_ref ref{handle->asset, nullptr};
    pulse_asset_release(app, &ref);
}

} // extern "C"
