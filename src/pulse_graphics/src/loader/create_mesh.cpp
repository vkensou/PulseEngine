#include "../graphics_internal.h"

#include "renderer.h"
#include <cstring>

namespace pulse_graphics_internal {

struct MeshCreateFromDataState {
    bool vertexBufferPrepared = false;
    bool indexBufferPrepared = false;
};

pulse_asset_loader_status_t step_mesh_create_from_data(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<MeshCreateFromDataState*>(state);

    auto vertexBufferHandle = ctx->dependencies[0].handle;
    pulse_buffer_t vertexBuffer = { vertexBufferHandle.index, vertexBufferHandle.generation };
    if (!s->vertexBufferPrepared) {
        auto vbState = pulse_asset_get_state(ctx->app, vertexBufferHandle);
        if (vbState == PULSE_ASSET_STATE_LOADED)
            s->vertexBufferPrepared = true;
        else if (vbState == PULSE_ASSET_STATE_FAILED || vbState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "mesh create loader: failed to wait vertex buffer";
            return PULSE_ASSET_LOADER_FAILED;
        } else
            s->vertexBufferPrepared = false;
    }

    auto indexBufferHandle = ctx->dependencies[1].handle;
    pulse_buffer_t indexBuffer = { indexBufferHandle.index, indexBufferHandle.generation };
    bool hasIndexBuffer = pulse_asset_handle_is_valid(indexBufferHandle);
    if (!s->indexBufferPrepared) {
        if (!hasIndexBuffer)
            s->indexBufferPrepared = true;
        else {
            auto ibState = pulse_asset_get_state(ctx->app, indexBufferHandle);
            if (ibState == PULSE_ASSET_STATE_LOADED)
                s->indexBufferPrepared = true;
            else if (ibState == PULSE_ASSET_STATE_FAILED || ibState == PULSE_ASSET_STATE_PENDING_DELETE) {
                *out_error = "mesh create loader: failed to wait index buffer";
                return PULSE_ASSET_LOADER_FAILED;
            } else
                s->indexBufferPrepared = false;
        }
    }

    if (!s->vertexBufferPrepared || !s->indexBufferPrepared)
        return PULSE_ASSET_LOADER_WAIT_DEPENDENCIES;

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    auto* mesh = static_cast<pulse_mesh_data_t*>(ctx->out_asset);
    auto* desc = static_cast<const pulse_graphics_mesh_create_from_data_desc*>(ctx->settings);
    
    if (ctx->dependency_count < 2) {
        *out_error = "mesh create loader: missing buffer dependencies";
        return PULSE_ASSET_LOADER_FAILED;
    }
    
    pulse_graphics_buffer_ref vb_ref{};
    if (!pulse_graphics_buffer_acquire(ctx->app, vertexBuffer, &vb_ref)) {
        *out_error = "mesh create loader: failed to acquire vertex buffer";
        return PULSE_ASSET_LOADER_FAILED;
    }

    pulse_graphics_buffer_ref ib_ref{};
    if (hasIndexBuffer && !pulse_graphics_buffer_acquire(ctx->app, indexBuffer, &ib_ref)) {
        pulse_graphics_buffer_release(ctx->app, &vb_ref);
        *out_error = "mesh create loader: failed to acquire index buffer";
        return PULSE_ASSET_LOADER_FAILED;
    }
    
    const CGPUVertexLayout& use_layout = desc->layout;
    mesh->vertex_layout = use_layout;
    mesh->p_vertex_attributes = new CGPUVertexAttribute[use_layout.attribute_count];
    std::copy(use_layout.p_attributes, use_layout.p_attributes + use_layout.attribute_count, mesh->p_vertex_attributes);
    mesh->vertex_layout.p_attributes = mesh->p_vertex_attributes;
    mesh->prim_topology = desc->topology;
    mesh->vertices_count = desc->vertex_count;
    mesh->vertex_stride = desc->vertex_stride;
    mesh->index_count = desc->index_count;
    mesh->index_stride = desc->index_stride;
    
    mesh->vertex_buffer = vb_ref.ptr;
    mesh->index_buffer = hasIndexBuffer ? ib_ref.ptr : nullptr;
    mesh->prepared = true;
    
    pulse_graphics_buffer_release(ctx->app, &vb_ref);
    if (hasIndexBuffer) pulse_graphics_buffer_release(ctx->app, &ib_ref);

    return PULSE_ASSET_LOADER_DONE;
}

struct MeshCreateDynamicState {
    bool initialized = false;
};

pulse_asset_loader_status_t step_mesh_create_dynamic(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<MeshCreateDynamicState*>(state);

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* mesh = static_cast<pulse_mesh_data_t*>(ctx->out_asset);
        auto* desc = static_cast<const pulse_graphics_mesh_create_dynamic_desc*>(ctx->settings);

        const CGPUVertexLayout& use_layout = desc->layout;
        mesh->vertex_layout = use_layout;
        mesh->p_vertex_attributes = new CGPUVertexAttribute[use_layout.attribute_count];
        std::copy(use_layout.p_attributes, use_layout.p_attributes + use_layout.attribute_count, mesh->p_vertex_attributes);
        mesh->vertex_layout.p_attributes = mesh->p_vertex_attributes;
        mesh->prim_topology = desc->topology;
        mesh->vertices_count = 0;
        mesh->vertex_stride = 0;
        for (auto i = 0; i < use_layout.attribute_count; ++i)
            mesh->vertex_stride += use_layout.p_attributes[i].elem_stride;
        mesh->index_stride = desc->index_stride;
        mesh->vertex_buffer = nullptr;
        mesh->index_buffer = nullptr;
        mesh->prepared = true;

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_DONE;
}

void register_mesh_create_loader(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld1{};
    ld1.struct_size = sizeof(pulse_asset_loader_desc);
    ld1.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld1.type_id = PULSE_TYPE_MESH;
    ld1.extensions = "";
    ld1.ctor = nullptr;
    ld1.dtor = nullptr;
    ld1.step = step_mesh_create_from_data;
    ld1.loader_size = sizeof(MeshCreateFromDataState);
    ld1.loader_align = alignof(MeshCreateFromDataState);
    ld1.settings_size = sizeof(pulse_graphics_mesh_create_from_data_desc);
    ld1.settings_align = alignof(pulse_graphics_mesh_create_from_data_desc);
    ld1.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld1);

    pulse_asset_loader_desc ld2{};
    ld2.struct_size = sizeof(pulse_asset_loader_desc);
    ld2.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld2.type_id = PULSE_TYPE_MESH;
    ld2.extensions = "";
    ld2.ctor = nullptr;
    ld2.dtor = nullptr;
    ld2.step = step_mesh_create_dynamic;
    ld2.loader_size = sizeof(MeshCreateDynamicState);
    ld2.loader_align = alignof(MeshCreateDynamicState);
    ld2.settings_size = sizeof(pulse_graphics_mesh_create_dynamic_desc);
    ld2.settings_align = alignof(pulse_graphics_mesh_create_dynamic_desc);
    ld2.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld2);
}

}

extern "C" {

pulse_mesh_t pulse_graphics_mesh_create_from_data(
    pulse_app_t app,
    const pulse_graphics_mesh_create_from_data_desc* desc)
{
    pulse_mesh_t result{};
    if (!desc)
        return {};

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return result;

    pulse_graphics_buffer_create_desc vertex_buffer_desc = {
        .desc = {
            .size = desc->vertex_count * desc->vertex_stride,
            .name = "vertex buffer",
            .descriptors = ECGPUResourceTypeFlags(desc->update_vertex_data_from_compute_shader ? CGPU_RESOURCE_TYPE_VERTEX_BUFFER | CGPU_RESOURCE_TYPE_RW_BUFFER : CGPU_RESOURCE_TYPE_VERTEX_BUFFER),
            .memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY,
            .flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP,
        },
        .data_size = desc->vertex_count * desc->vertex_stride,
        .data = desc->vertex_data,
    };
    auto vertex_buffer = pulse_graphics_buffer_create(app, &vertex_buffer_desc);

    pulse_buffer_t index_buffer = {};

    if (desc->index_count > 0) {
        pulse_graphics_buffer_create_desc index_buffer_desc = {
            .desc = {
                .size = desc->index_count * desc->index_stride,
                .name = "index buffer",
                .descriptors = ECGPUResourceTypeFlags(desc->update_index_data_from_compute_shader ? CGPU_RESOURCE_TYPE_INDEX_BUFFER | CGPU_RESOURCE_TYPE_RW_BUFFER : CGPU_RESOURCE_TYPE_INDEX_BUFFER),
                .memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY,
                .flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP,
            },
            .data_size = desc->index_count * desc->index_stride,
            .data = desc->index_data,
        };
        auto index_buffer = pulse_graphics_buffer_create(app, &index_buffer_desc);
    }

    pulse_asset_dependency deps[2] = {
        { pulse_graphics_buffer_to_handle(vertex_buffer), PULSE_DEP_REQUIRED },
        { pulse_graphics_buffer_to_handle(index_buffer), PULSE_DEP_OPTIONAL },
    };

    pulse_asset_handle asset_handle = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_MESH, nullptr, deps, 2, desc);
    if (!pulse_asset_handle_is_valid(asset_handle))
        return result;

    result.index = asset_handle.index;
    result.generation = asset_handle.generation;
    return result;
}

pulse_mesh_t pulse_graphics_mesh_create_dynamic(
    pulse_app_t app,
    const pulse_graphics_mesh_create_dynamic_desc* desc)
{
    pulse_mesh_t result{};
    if (!desc)
        return result;

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return result;

    pulse_asset_handle asset_handle = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_MESH, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(asset_handle))
        return result;

    result.index = asset_handle.index;
    result.generation = asset_handle.generation;
    return result;
}

}
