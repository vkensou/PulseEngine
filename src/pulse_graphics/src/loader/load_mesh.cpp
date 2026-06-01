#include "../graphics_internal.h"
#include "renderer.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <sstream>
#include <streambuf>

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

namespace pulse_graphics_internal {

// ── Local mesh types ──────────────────────────────────────────
struct MeshVec3 { float x, y, z; };
struct MeshVec2 { float x, y; };
struct MeshTexturedVertex { MeshVec3 position; MeshVec3 normal; MeshVec2 texCoord; };

struct MeshBufSrc : std::basic_streambuf<char> {
    MeshBufSrc(const uint8_t* d, size_t n) : buf(d, d + n) {
        char* p = (char*)buf.data();
        setg(p, p, p + n);
        setp(p, p + n);
    }
    std::vector<uint8_t> buf;
};

// ── OBJ parser from memory ────────────────────────────────────
static bool parse_obj_mesh(const uint8_t* data, size_t size,
                           std::vector<MeshTexturedVertex>& out_verts,
                           std::vector<uint32_t>& out_indices)
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    MeshBufSrc bs(data, size);
    std::istream reader(&bs);
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &reader))
        return false;

    const int rh = -1;
    std::vector<MeshVec3> coords, normals;
    std::vector<MeshVec2> texCoords;

    coords.reserve(attrib.vertices.size() / 3);
    for (size_t i = 0; i < attrib.vertices.size() / 3; ++i)
        coords.push_back({attrib.vertices[3*i+0]*rh, attrib.vertices[3*i+1], attrib.vertices[3*i+2]});
    normals.reserve(attrib.normals.size() / 3);
    for (size_t i = 0; i < attrib.normals.size() / 3; ++i)
        normals.push_back({attrib.normals[3*i+0]*rh, attrib.normals[3*i+1], attrib.normals[3*i+2]});
    texCoords.reserve(attrib.texcoords.size() / 2);
    for (size_t i = 0; i < attrib.texcoords.size() / 2; ++i)
        texCoords.push_back({attrib.texcoords[2*i+0], 1 - attrib.texcoords[2*i+1]});

    struct TripleHash {
        size_t operator()(const std::tuple<int,int,int>& t) const {
            return std::hash<int>{}(std::get<0>(t)) ^
                   (std::hash<int>{}(std::get<1>(t)) << 1) ^
                   (std::hash<int>{}(std::get<2>(t)) << 2);
        }
    };
    std::unordered_map<std::tuple<int,int,int>, uint32_t, TripleHash> vmap;

    out_verts.reserve(coords.size());
    for (auto& shape : shapes) {
        auto& m = shape.mesh;
        out_indices.reserve(m.indices.size());
        for (auto& idx : m.indices) {
            auto key = std::tuple{idx.vertex_index, idx.normal_index, idx.texcoord_index};
            auto it = vmap.find(key);
            if (it != vmap.end()) {
                out_indices.push_back(it->second);
            } else {
                MeshVec3 pos = coords[idx.vertex_index];
                MeshVec3 nml = idx.normal_index >= 0 ? normals[idx.normal_index] : MeshVec3{};
                MeshVec2 tc  = idx.texcoord_index >= 0 ? texCoords[idx.texcoord_index] : MeshVec2{};
                uint32_t vi = (uint32_t)out_verts.size();
                out_verts.push_back({pos, nml, tc});
                out_indices.push_back(vi);
                vmap[key] = vi;
            }
        }
        for (size_t j = 0; j + 2 < out_indices.size(); j += 3)
            std::swap(out_indices[j + 1], out_indices[j + 2]);
        break;
    }
    return true;
}

// ── Mesh load state machine ───────────────────────────────────

enum MeshLoadPhase {
    MESH_LOAD_PARSE = 0,
    MESH_LOAD_WAIT_BUFFERS = 1,
};

struct MeshLoadState {
    int phase = MESH_LOAD_PARSE;
    PulseGraphicsBufferHandle vb_handle = {};
    PulseGraphicsBufferHandle ib_handle = {};
    uint32_t vertex_stride = 0;
};

EPulseAssetLoaderStatus step_mesh_load(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<MeshLoadState*>(state);

    if (s->phase == MESH_LOAD_PARSE) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* mesh = static_cast<pulse_mesh_data_t*>(ctx->out_asset);

        std::vector<MeshTexturedVertex> verts;
        std::vector<uint32_t> indices;
        if (!parse_obj_mesh(ctx->bytes, ctx->byte_size, verts, indices)) {
            *out_error = "mesh loader: OBJ parse failed";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        CGPUVertexAttribute attrs[3] = {
            {"POSITION", 1, CGPU_VERTEX_FORMAT_FLOAT32X3, 0, offsetof(MeshTexturedVertex, position), sizeof(float)*3, CGPU_VERTEX_INPUT_RATE_VERTEX},
            {"NORMAL",   1, CGPU_VERTEX_FORMAT_FLOAT32X3, 0, offsetof(MeshTexturedVertex, normal),   sizeof(float)*3, CGPU_VERTEX_INPUT_RATE_VERTEX},
            {"TEXCOORD", 1, CGPU_VERTEX_FORMAT_FLOAT32X2, 0, offsetof(MeshTexturedVertex, texCoord), sizeof(float)*2, CGPU_VERTEX_INPUT_RATE_VERTEX},
        };
        CGPUVertexLayout layout{3, attrs};
        uint32_t vstride = sizeof(MeshTexturedVertex);

        // Set up mesh layout
        mesh->vertex_layout = layout;
        mesh->p_vertex_attributes = new CGPUVertexAttribute[layout.attribute_count];
        std::copy(layout.p_attributes, layout.p_attributes + layout.attribute_count, mesh->p_vertex_attributes);
        mesh->vertex_layout.p_attributes = mesh->p_vertex_attributes;
        mesh->prim_topology = CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        mesh->vertices_count = (uint32_t)verts.size();
        mesh->vertex_stride = vstride;
        mesh->index_count = (uint32_t)indices.size();
        mesh->index_stride = sizeof(uint32_t);
        mesh->vertex_buffer = nullptr;
        mesh->index_buffer = nullptr;
        mesh->prepared = false;
        s->vertex_stride = vstride;

        // Create vertex buffer asset
        CGPUBufferDescriptor vb_desc = {};
        vb_desc.name = ctx->path;
        vb_desc.flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP;
        vb_desc.descriptors = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
        vb_desc.memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY;
        vb_desc.size = verts.size() * vstride;

        PulseGraphicsBufferCreateDesc vb_create_desc = { vb_desc, vb_desc.size, verts.data() };
        s->vb_handle = pulse_create_graphics_buffer(pulse_graphics_internal::g_loader_app, &vb_create_desc);
        pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_graphics_buffer_to_handle(s->vb_handle), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED);

        // Create index buffer asset
        size_t ib_bytes = indices.size() * sizeof(uint32_t);
        CGPUBufferDescriptor ib_desc = {};
        ib_desc.name = ctx->path;
        ib_desc.flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP;
        ib_desc.descriptors = CGPU_RESOURCE_TYPE_INDEX_BUFFER;
        ib_desc.memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY;
        ib_desc.size = ib_bytes;

        PulseGraphicsBufferCreateDesc ib_create_desc = { ib_desc, ib_bytes, indices.data() };
        s->ib_handle = pulse_create_graphics_buffer(pulse_graphics_internal::g_loader_app, &ib_create_desc);
        pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_graphics_buffer_to_handle(s->ib_handle), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED);

        s->phase = MESH_LOAD_WAIT_BUFFERS;
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
    }

    if (s->phase == MESH_LOAD_WAIT_BUFFERS) {
        auto* mesh = static_cast<pulse_mesh_data_t*>(ctx->out_asset);

        PulseGraphicsBuffer vb_ref{};
        PulseGraphicsBuffer ib_ref{};

        if (!internal_acquire_buffer(ctx->asset_system, s->vb_handle, &vb_ref))
            return PULSE_ASSET_LOADER_STATUS_PENDING;
        if (!internal_acquire_buffer(ctx->asset_system, s->ib_handle, &ib_ref)) {
            internal_release_buffer(ctx->asset_system, &vb_ref);
            return PULSE_ASSET_LOADER_STATUS_PENDING;
        }

        mesh->vertex_buffer = static_cast<pulse_buffer_data_t*>(vb_ref.ptr);
        mesh->index_buffer = static_cast<pulse_buffer_data_t*>(ib_ref.ptr);
        mesh->prepared = true;

        internal_release_buffer(ctx->asset_system, &vb_ref);
        internal_release_buffer(ctx->asset_system, &ib_ref);

        return PULSE_ASSET_LOADER_STATUS_DONE;
    }

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_mesh_load_loader(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_MESH;
    ld.extensions = "obj";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_mesh_load;
    ld.loader_size = sizeof(MeshLoadState);
    ld.loader_align = alignof(MeshLoadState);
    ld.settings_size = 0;
    ld.settings_align = 0;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld);
}

} // namespace pulse_graphics_internal

extern "C" {

PulseMeshHandle pulse_load_mesh(PulseAppId app, const char* filepath)
{
    PulseMeshHandle result{};
    if (!app || !filepath || !filepath[0]) return result;

    PulseAssetHandle asset_handle = pulse_graphics_internal::asset_load_path(app, PULSE_TYPE_MESH, filepath);
    if (!pulse_asset_handle_is_valid(asset_handle)) return result;

    result.index = asset_handle.index;
    result.generation = asset_handle.generation;
    return result;
}

} // extern "C"
