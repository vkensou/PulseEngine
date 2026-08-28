#include "../graphics_internal.h"

#include "renderer.h"
#include <cstring>

namespace pulse_graphics_internal {

struct MeshCreateFromDataState {
    bool vertexBufferPrepared = false;
    bool indexBufferPrepared = false;
};

// Settings deep-copy: the asset system allocates one block of the returned size and
// copies the struct bytes to its head; these callbacks lay out the nested data right
// after the struct (attribute array, then semantic names, then vertex/index data)
// and fix the pointers into the block. size/copy share the same layout.
static uint64_t mesh_from_data_settings_size_fn(const void* settings, void* user_data) {
    const auto* s = static_cast<const PulseMeshCreateFromDataDesc*>(settings);
    uint64_t total = sizeof(PulseMeshCreateFromDataDesc);

    if (s->layout.p_attributes && s->layout.attribute_count > 0) {
        // Mirror the align_up_pointer in the copy callback so both compute the same layout.
        total = align_up_value(total, alignof(CGPUVertexAttribute));
        total += (uint64_t)s->layout.attribute_count * sizeof(CGPUVertexAttribute);
        for (uint32_t i = 0; i < s->layout.attribute_count; ++i) {
            if (s->layout.p_attributes[i].semantic_name) {
                total += strlen(s->layout.p_attributes[i].semantic_name) + 1;
            }
        }
    }
    if (s->vertex_data && s->vertex_count > 0 && s->vertex_stride > 0) {
        total += (uint64_t)s->vertex_count * s->vertex_stride;
    }
    if (s->index_data && s->index_count > 0 && s->index_stride > 0) {
        total += (uint64_t)s->index_count * s->index_stride;
    }
    return total;
}

static bool mesh_from_data_settings_copy_fn(void* dst, const void* src, uint64_t byte_size, void* user_data) {
    auto* d = static_cast<PulseMeshCreateFromDataDesc*>(dst);
    const auto* s = static_cast<const PulseMeshCreateFromDataDesc*>(src);
    uint8_t* end = reinterpret_cast<uint8_t*>(dst) + byte_size;
    uint8_t* cursor = reinterpret_cast<uint8_t*>(dst) + sizeof(PulseMeshCreateFromDataDesc);

    if (s->layout.p_attributes && s->layout.attribute_count > 0) {
        cursor = align_up_pointer(cursor, alignof(CGPUVertexAttribute));
        uint64_t n = (uint64_t)s->layout.attribute_count * sizeof(CGPUVertexAttribute);
        if (cursor + n > end) {
            return false;
        }
        auto* attrs = reinterpret_cast<CGPUVertexAttribute*>(cursor);
        memcpy(attrs, s->layout.p_attributes, n);
        cursor += n;

        for (uint32_t i = 0; i < s->layout.attribute_count; ++i) {
            const char* name = s->layout.p_attributes[i].semantic_name;
            if (!name) {
                continue;
            }
            size_t len = strlen(name) + 1;
            if (cursor + len > end) {
                return false;
            }
            memcpy(cursor, name, len);
            attrs[i].semantic_name = reinterpret_cast<const char*>(cursor);
            cursor += len;
        }
        d->layout.p_attributes = attrs;
    } else {
        d->layout.p_attributes = nullptr;
    }

    if (s->vertex_data && s->vertex_count > 0 && s->vertex_stride > 0) {
        uint64_t n = (uint64_t)s->vertex_count * s->vertex_stride;
        if (cursor + n > end) {
            return false;
        }
        memcpy(cursor, s->vertex_data, n);
        d->vertex_data = cursor;
        cursor += n;
    } else {
        d->vertex_data = nullptr;
    }
    if (s->index_data && s->index_count > 0 && s->index_stride > 0) {
        uint64_t n = (uint64_t)s->index_count * s->index_stride;
        if (cursor + n > end) {
            return false;
        }
        memcpy(cursor, s->index_data, n);
        d->index_data = cursor;
        cursor += n;
    } else {
        d->index_data = nullptr;
    }
    return true;
}

EPulseAssetLoaderStatus step_mesh_create_from_data(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<MeshCreateFromDataState*>(state);

    auto vertexBufferRequest = pulse_asset_system_to_asset_request_from_dep_ref(ctx->asset_system, ctx->p_dependencies[0].dep_ref);
    if (!s->vertexBufferPrepared) {
        auto vbState = pulse_asset_system_get_state(ctx->asset_system, vertexBufferRequest);
        if (vbState == PULSE_ASSET_STATE_LOADED)
            s->vertexBufferPrepared = true;
        else if (vbState == PULSE_ASSET_STATE_FAILED || vbState == PULSE_ASSET_STATE_PENDING_DELETE) {
            *out_error = "mesh create loader: failed to wait vertex buffer";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        } else
            s->vertexBufferPrepared = false;
    }

    bool hasIndexBuffer = pulse_asset_dep_ref_is_valid(ctx->p_dependencies[1].dep_ref);
    if (!s->indexBufferPrepared) {
        if (!hasIndexBuffer)
            s->indexBufferPrepared = true;
        else {
            auto indexBufferRequest = pulse_asset_system_to_asset_request_from_dep_ref(ctx->asset_system, ctx->p_dependencies[1].dep_ref);
            auto ibState = pulse_asset_system_get_state(ctx->asset_system, indexBufferRequest);
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
    auto* mesh = static_cast<PulseMeshData*>(ctx->out_asset);
    auto* desc = static_cast<const PulseMeshCreateFromDataDesc*>(ctx->settings);

    if (ctx->dependencies_count < 2) {
        *out_error = "mesh create loader: missing buffer dependencies";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    PulseGraphicsBufferHandle vertexBuffer = pulse_graphics_buffer_get_handle(ctx->app, { vertexBufferRequest.index, vertexBufferRequest.generation });
    PulseGraphicsBufferData* vb_data = internal_borrow_buffer(ctx->asset_system, vertexBuffer);
    if (!vb_data) {
        *out_error = "mesh create loader: failed to acquire vertex buffer";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    PulseGraphicsBufferData* ib_data = nullptr;
    PulseGraphicsBufferHandle indexBuffer = {};
    if (hasIndexBuffer) {
        auto indexBufferRequest = pulse_asset_system_to_asset_request_from_dep_ref(ctx->asset_system, ctx->p_dependencies[1].dep_ref);
        indexBuffer = pulse_graphics_buffer_get_handle(ctx->app, { indexBufferRequest.index, indexBufferRequest.generation });
        ib_data = internal_borrow_buffer(ctx->asset_system, indexBuffer);
        if (!ib_data) {
            *out_error = "mesh create loader: failed to acquire index buffer";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
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

    mesh->vertex_buffer = { vertexBuffer, vb_data };
    mesh->index_buffer = { indexBuffer, ib_data };
    mesh->prepared = true;

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
        auto* desc = static_cast<const PulseMeshCreateDynamicDesc*>(ctx->settings);
        auto* mesh = static_cast<PulseMeshData*>(ctx->out_asset);
        HGEGraphics::create_dynamic_mesh(mesh, desc->topology, desc->layout, desc->index_stride);
        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_mesh_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
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
    ld1.settings_size = sizeof(PulseMeshCreateFromDataDesc);
    ld1.settings_align = alignof(PulseMeshCreateFromDataDesc);
    ld1.settings_size_fn = mesh_from_data_settings_size_fn;
    ld1.settings_copy_fn = mesh_from_data_settings_copy_fn;
    ld1.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld1);

    PulseAssetLoaderDesc ld2{};
    ld2.struct_size = sizeof(PulseAssetLoaderDesc);
    ld2.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld2.type_id = PULSE_TYPE_MESH;
    ld2.extensions = "";
    ld2.loader_identifier = "dynamic";
    ld2.ctor = nullptr;
    ld2.dtor = nullptr;
    ld2.step = step_mesh_create_dynamic;
    ld2.loader_size = sizeof(MeshCreateDynamicState);
    ld2.loader_align = alignof(MeshCreateDynamicState);
    ld2.settings_size = sizeof(PulseMeshCreateDynamicDesc);
    ld2.settings_align = alignof(PulseMeshCreateDynamicDesc);
    ld2.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld2);
}

}

extern "C" {

PulseMeshRequest pulse_create_mesh_from_data(
    PulseAppId app,
    const PulseMeshCreateFromDataDesc* desc)
{
    PulseMeshRequest result{};
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
        .p_data = desc->vertex_data,
        .data_size = desc->vertex_count * desc->vertex_stride,
    };
    auto vertex_buffer = pulse_create_graphics_buffer(app, &vertex_buffer_desc);

    PulseGraphicsBufferRequest index_buffer{};

    if (desc->index_count > 0) {
        PulseGraphicsBufferCreateDesc index_buffer_desc = {
            .desc = {
                .size = desc->index_count * desc->index_stride,
                .name = "index buffer",
                .descriptors = ECGPUResourceTypeFlags(desc->update_index_data_from_compute_shader ? CGPU_RESOURCE_TYPE_INDEX_BUFFER | CGPU_RESOURCE_TYPE_RW_BUFFER : CGPU_RESOURCE_TYPE_INDEX_BUFFER),
                .memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY,
                .flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP,
            },
            .p_data = desc->index_data,
            .data_size = desc->index_count * desc->index_stride,
        };
        index_buffer = pulse_create_graphics_buffer(app, &index_buffer_desc);
    }

    PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
    PulseAssetDependency deps[2] = {
        { pulse_asset_system_to_asset_dep_ref_from_request(as, pulse_graphics_buffer_request_to_asset_request(vertex_buffer)), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED },
        { pulse_asset_system_to_asset_dep_ref_from_request(as, pulse_graphics_buffer_request_to_asset_request(index_buffer)), PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL },
    };

    PulseAssetRequest request = pulse_graphics_internal::asset_build(
        as, PULSE_TYPE_MESH, nullptr, deps, 2, desc);
    if (!pulse_asset_request_is_valid(request))
        return result;

    result.index = request.index;
    result.generation = request.generation;
    return result;
}

PulseMeshHandle pulse_create_mesh_dynamic(
    PulseAppId app,
    const PulseMeshCreateDynamicDesc* desc)
{
    PulseMeshHandle result{};
    if (!desc)
        return result;

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return result;

    PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
    PulseAssetHandle h = pulse_graphics_internal::asset_build_sync(
        as, PULSE_TYPE_MESH, "dynamic", nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(h))
        return result;

    result.index = h.index;
    result.generation = h.generation;
    return result;
}

}
