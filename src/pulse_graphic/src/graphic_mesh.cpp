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

    auto cpp_mesh = HGEGraphics::create_mesh(device, vertex_count, index_count,
        topology, use_layout, use_idx_stride, false, false);
    if (!cpp_mesh) return result;

    pulse_asset_handle asset_handle = pulse_asset_load_from_memory(
        app, PULSE_TYPE_MESH, "", nullptr, 0);
    if (asset_handle.index == PULSE_ASSET_INVALID_INDEX) return result;

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_mesh_data_t* mesh = static_cast<pulse_mesh_data_t*>(ref.ptr);
        mesh->vertex_layout = use_layout;
        mesh->vertex_stride = vertex_stride;
        mesh->index_stride = use_idx_stride;
        mesh->vertices_count = vertex_count;
        mesh->index_count = index_count;
        mesh->prim_topology = topology;
        mesh->vertex_buffer = cpp_mesh->vertex_buffer->handle;
        mesh->index_buffer = cpp_mesh->index_buffer ? cpp_mesh->index_buffer->handle : CGPU_NULLPTR;
        mesh->has_index_buffer = (index_data && index_count > 0);
        cpp_mesh->vertex_buffer->handle = CGPU_NULLPTR;
        if (cpp_mesh->index_buffer) cpp_mesh->index_buffer->handle = CGPU_NULLPTR;
        pulse_asset_release(app, &ref);
    }

    if (vertex_data && vertex_count > 0) {
        pulse_graphic_internal::mark_upload_pending(app, asset_handle);
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

    uint32_t use_idx_stride = (index_stride == 0) ? sizeof(uint32_t) : index_stride;
    auto cpp_mesh = HGEGraphics::create_dynamic_mesh(topology, *layout, use_idx_stride);
    if (!cpp_mesh) return result;

    pulse_asset_handle asset_handle = pulse_asset_load_from_memory(
        app, PULSE_TYPE_MESH, "", nullptr, 0);
    if (asset_handle.index == PULSE_ASSET_INVALID_INDEX) return result;

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_mesh_data_t* mesh = static_cast<pulse_mesh_data_t*>(ref.ptr);
        mesh->vertex_layout = *layout;
        mesh->vertex_stride = vertex_stride;
        mesh->index_stride = use_idx_stride;
        mesh->vertices_count = max_vertex_count;
        mesh->index_count = max_index_count;
        mesh->prim_topology = topology;
        mesh->vertex_buffer = cpp_mesh->vertex_buffer->handle;
        mesh->index_buffer = cpp_mesh->index_buffer ? cpp_mesh->index_buffer->handle : CGPU_NULLPTR;
        mesh->has_index_buffer = (max_index_count > 0);
        cpp_mesh->vertex_buffer->handle = CGPU_NULLPTR;
        if (cpp_mesh->index_buffer) cpp_mesh->index_buffer->handle = CGPU_NULLPTR;
        pulse_asset_release(app, &ref);
    }

    result.asset = asset_handle;
    return result;
}

void pulse_graphic_mesh_update_vertices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count) {
    pulse_graphic_internal::pulse_graphic_state* st = pulse_graphic_internal::state_from_app(app);
    if (st && mesh) {
        pulse_graphic_internal::pulse_graphic_state::UploadEntry entry{mesh->asset, false, data, count};
        st->dynamic_updates.push_back(entry);
    }
}

void pulse_graphic_mesh_update_indices(pulse_app_t app, pulse_mesh_t* mesh, const void* data, uint32_t count) {
    pulse_graphic_internal::pulse_graphic_state* st = pulse_graphic_internal::state_from_app(app);
    if (st && mesh) {
        pulse_graphic_internal::pulse_graphic_state::UploadEntry entry{mesh->asset, false, data, count};
        st->dynamic_updates.push_back(entry);
    }
}

pulse_mesh_t pulse_graphic_mesh_load(
    pulse_app_t app,
    const char* filepath)
{
    (void)app; (void)filepath;
    return pulse_mesh_t{};
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
