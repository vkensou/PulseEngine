#include "graphic_internal.h"
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

namespace pulse_graphic_internal {

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

// ── Mesh loader state machine ─────────────────────────────────

struct MeshLoaderState {
    bool upload_requested = false;
    bool has_ib = false;
    bool vb_completed = false;
    bool ib_completed = false;
};

pulse_result_t start_mesh(const pulse_asset_load_task* ctx, void** out_state, void* user_data) {
    (void)ctx; (void)user_data;
    *out_state = new MeshLoaderState();
    return PULSE_OK;
}

void destroy_mesh_loader_state(void* state, void*) {
    delete static_cast<MeshLoaderState*>(state);
}

pulse_asset_loader_status_t step_mesh(
    const pulse_asset_load_task* ctx, void* state, void* out_asset,
    const char** out_error, void* user_data)
{
    auto* s = static_cast<MeshLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
        auto* mesh = static_cast<pulse_mesh_data_t*>(out_asset);

        std::vector<MeshTexturedVertex> verts;
        std::vector<uint32_t> indices;
        if (!parse_obj_mesh(ctx->bytes, ctx->byte_size, verts, indices)) {
            *out_error = "mesh loader: OBJ parse failed";
            return PULSE_ASSET_LOADER_FAILED;
        }

        CGPUVertexAttribute attrs[3] = {
            {"POSITION", 1, CGPU_VERTEX_FORMAT_FLOAT32X3, 0, offsetof(MeshTexturedVertex, position), sizeof(float)*3, CGPU_VERTEX_INPUT_RATE_VERTEX},
            {"NORMAL",   1, CGPU_VERTEX_FORMAT_FLOAT32X3, 0, offsetof(MeshTexturedVertex, normal),   sizeof(float)*3, CGPU_VERTEX_INPUT_RATE_VERTEX},
            {"TEXCOORD", 1, CGPU_VERTEX_FORMAT_FLOAT32X2, 0, offsetof(MeshTexturedVertex, texCoord), sizeof(float)*2, CGPU_VERTEX_INPUT_RATE_VERTEX},
        };
        CGPUVertexLayout layout{3, attrs};

        uint32_t vstride = sizeof(MeshTexturedVertex);
        HGEGraphics::init_mesh(mesh, device, (uint32_t)verts.size(),
            (uint32_t)indices.size(), CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            layout, sizeof(uint32_t), false, false);

        // Register deferred upload via upload queue
        auto* gstate = state_from_app(ctx->app);
        if (gstate) {
            size_t vb_bytes = verts.size() * vstride;
            size_t ib_bytes = indices.size() * sizeof(uint32_t);

            // VB
            auto& vb_blk = gstate->staging_pool[gstate->staging_write].emplace_back(vb_bytes);
            memcpy(vb_blk.data(), verts.data(), vb_bytes);
            gstate->pending_uploads.push_back({
                UPLOAD_BUFFER_DATA, {}, {},
                mesh->vertex_buffer,
                vb_blk.data(), vb_bytes,
                &s->vb_completed
            });

            // IB
            s->has_ib = ib_bytes > 0;
            if (s->has_ib) {
                auto& ib_blk = gstate->staging_pool[gstate->staging_write].emplace_back(ib_bytes);
                memcpy(ib_blk.data(), indices.data(), ib_bytes);
                gstate->pending_uploads.push_back({
                    UPLOAD_BUFFER_DATA, {}, {},
                    mesh->index_buffer,
                    ib_blk.data(), ib_bytes,
                    &s->ib_completed
                });
            }
        }

        s->upload_requested = true;
        return PULSE_ASSET_LOADER_PENDING;
    }

    // Wait for uploads to be queued by upload_record_callback
    if (s->vb_completed && (!s->has_ib || s->ib_completed)) {
        return PULSE_ASSET_LOADER_DONE;
    }

    return PULSE_ASSET_LOADER_PENDING;
}

} // namespace pulse_graphic_internal

// ── Public API ────────────────────────────────────────────────

extern "C" {

pulse_mesh_t pulse_graphic_mesh_load(pulse_app_t app, const char* filepath)
{
    pulse_mesh_t result{};
    if (!app || !filepath || !filepath[0]) return result;
    result.asset = pulse_asset_load(app, PULSE_TYPE_MESH, filepath);
    return result;
}

} // extern "C"
