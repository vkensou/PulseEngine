#include "../graphics_internal.h"

#include "renderer.h"
#include <cstring>

namespace pulse_graphics_internal {

struct MeshCreateFromDataState {
    bool vertexBufferPrepared = false;
    bool indexBufferPrepared = false;
};

EPulseAssetLoaderStatus step_mesh_create_from_data(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<MeshCreateFromDataState*>(state);

    auto vertexBufferHandle = ctx->dependencies[0].handle;
    PulseBufferHandle vertexBuffer = { vertexBufferHandle.index, vertexBufferHandle.generation };
    if (!s->vertexBufferPrepared) {
        auto vbState = pulse_asset_system_get_state(ctx->asset_system, vertexBufferHandle);
        if (vbState == PULSE_ASSET_STATE_LOADED)
            s->vertexBufferPrepared = true;
        else if (vbState == PULSE_ASSET_STATE_FAILED || vbState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "mesh create loader: failed to wait vertex buffer";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        } else
            s->vertexBufferPrepared = false;
    }

    auto indexBufferHandle = ctx->dependencies[1].handle;
    PulseBufferHandle indexBuffer = { indexBufferHandle.index, indexBufferHandle.generation };
    bool hasIndexBuffer = pulse_asset_handle_is_valid(indexBufferHandle);
    if (!s->indexBufferPrepared) {
        if (!hasIndexBuffer)
            s->indexBufferPrepared = true;
        else {
            auto ibState = pulse_asset_system_get_state(ctx->asset_system, indexBufferHandle);
            if (ibState == PULSE_ASSET_STATE_LOADED)
                s->indexBufferPrepared = true;
            else if (ibState == PULSE_ASSET_STATE_FAILED || ibState == PULSE_ASSET_STATE_PENDING_DELETE) {
                *out_error = "mesh create loader: failed to wait index buffer";
                return PULSE_ASSET_LOADER_STATUS_FAILED;
            } else
                s->indexBufferPrepared = false;
        }
    }

    if (!s->vertexBufferPrepared || !s->indexBufferPrepared)
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    auto* mesh = static_cast<pulse_mesh_data_t*>(ctx->out_asset);
    auto* desc = static_cast<const PulseGraphicsMeshCreateFromDataDesc*>(ctx->settings);
    
    if (ctx->dependency_count < 2) {
        *out_error = "mesh create loader: missing buffer dependencies";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    
    PulseBuffer vb_ref{};
    if (!internal_acquire_buffer(ctx->asset_system, vertexBuffer, &vb_ref)) {
        *out_error = "mesh create loader: failed to acquire vertex buffer";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    PulseBuffer ib_ref{};
    if (hasIndexBuffer && !internal_acquire_buffer(ctx->asset_system, indexBuffer, &ib_ref)) {
        internal_release_buffer(ctx->asset_system, &vb_ref);
        *out_error = "mesh create loader: failed to acquire index buffer";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
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
    
    mesh->vertex_buffer = static_cast<pulse_buffer_data_t*>(vb_ref.ptr);
    mesh->index_buffer = hasIndexBuffer ? static_cast<pulse_buffer_data_t*>(ib_ref.ptr) : nullptr;
    mesh->prepared = true;
    
    internal_release_buffer(ctx->asset_system, &vb_ref);
    if (hasIndexBuffer) internal_release_buffer(ctx->asset_system, &ib_ref);

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

struct MeshCreateDynamicState {
    bool initialized = false;
};

EPulseAssetLoaderStatus step_mesh_create_dynamic(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<MeshCreateDynamicState*>(state);

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* mesh = static_cast<pulse_mesh_data_t*>(ctx->out_asset);
        auto* desc = static_cast<const PulseGraphicsMeshCreateDynamicDesc*>(ctx->settings);

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

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_mesh_create_loader(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld1{};
    ld1.struct_size = sizeof(PulseAssetLoaderDesc);
    ld1.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld1.type_id = PULSE_TYPE_MESH;
    ld1.extensions = "";
    ld1.ctor = nullptr;
    ld1.dtor = nullptr;
    ld1.step = step_mesh_create_from_data;
    ld1.loader_size = sizeof(MeshCreateFromDataState);
    ld1.loader_align = alignof(MeshCreateFromDataState);
    ld1.settings_size = sizeof(PulseGraphicsMeshCreateFromDataDesc);
    ld1.settings_align = alignof(PulseGraphicsMeshCreateFromDataDesc);
    ld1.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld1);

    PulseAssetLoaderDesc ld2{};
    ld2.struct_size = sizeof(PulseAssetLoaderDesc);
    ld2.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld2.type_id = PULSE_TYPE_MESH;
    ld2.extensions = "";
    ld2.ctor = nullptr;
    ld2.dtor = nullptr;
    ld2.step = step_mesh_create_dynamic;
    ld2.loader_size = sizeof(MeshCreateDynamicState);
    ld2.loader_align = alignof(MeshCreateDynamicState);
    ld2.settings_size = sizeof(PulseGraphicsMeshCreateDynamicDesc);
    ld2.settings_align = alignof(PulseGraphicsMeshCreateDynamicDesc);
    ld2.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld2);
}

}

extern "C" {

PulseMeshHandle pulse_graphics_mesh_create_from_data(
    PulseAppId app,
    const PulseGraphicsMeshCreateFromDataDesc* desc)
{
    PulseMeshHandle result{};
    if (!desc)
        return {};

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return result;

    PulseGraphicsBufferCreateDesc vertex_buffer_desc = {
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

    PulseBufferHandle index_buffer = {};

    if (desc->index_count > 0) {
        PulseGraphicsBufferCreateDesc index_buffer_desc = {
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

    PulseAssetDependency deps[2] = {
        { pulse_graphics_buffer_to_handle(vertex_buffer), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
        { pulse_graphics_buffer_to_handle(index_buffer), PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL },
    };

    PulseAssetHandle asset_handle = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_MESH, nullptr, deps, 2, desc);
    if (!pulse_asset_handle_is_valid(asset_handle))
        return result;

    result.index = asset_handle.index;
    result.generation = asset_handle.generation;
    return result;
}

PulseMeshHandle pulse_graphics_mesh_create_dynamic(
    PulseAppId app,
    const PulseGraphicsMeshCreateDynamicDesc* desc)
{
    PulseMeshHandle result{};
    if (!desc)
        return result;

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return result;

    PulseAssetHandle asset_handle = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_MESH, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(asset_handle))
        return result;

    result.index = asset_handle.index;
    result.generation = asset_handle.generation;
    return result;
}

}
