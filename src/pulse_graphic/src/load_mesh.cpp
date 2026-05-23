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

    const int rh = -1; // right_hand = true
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
        // right_hand: swap indices 1 and 2 per triangle
        for (size_t j = 0; j + 2 < out_indices.size(); j += 3)
            std::swap(out_indices[j + 1], out_indices[j + 2]);
        break;
    }
    return true;
}

// ── Mesh create from OBJ ──────────────────────────────────────
pulse_mesh_t pulse_graphic_mesh_load(pulse_app_t app, const char* filepath)
{
    pulse_mesh_t result{};
    if (!app || !filepath || !filepath[0]) return result;

    // Read file
    FILE* f = nullptr;
    fopen_s(&f, filepath, "rb");
    if (!f) return result;
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    if (len <= 0) { fclose(f); return result; }
    fseek(f, 0, SEEK_SET);
    std::vector<uint8_t> file_bytes(static_cast<size_t>(len));
    fread(file_bytes.data(), 1, file_bytes.size(), f);
    fclose(f);

    // Parse OBJ
    std::vector<MeshTexturedVertex> verts;
    std::vector<uint32_t> indices;
    if (!parse_obj_mesh(file_bytes.data(), file_bytes.size(), verts, indices))
        return result;

    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device) return result;

    uint32_t vcount = (uint32_t)verts.size();
    uint32_t icount = (uint32_t)indices.size();
    uint32_t vstride = sizeof(MeshTexturedVertex);
    uint32_t istride = sizeof(uint32_t);

    // Create vertex buffer
    CGPUBufferDescriptor vbd{};
    vbd.name = "mesh_vb";
    vbd.size = vcount * vstride;
    vbd.descriptors = CGPU_RESOURCE_TYPE_VERTEX_BUFFER;
    vbd.memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY;
    vbd.flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP;
    CGPUBufferId vbuf = cgpu_device_create_buffer(device, &vbd);
    if (!vbuf) return result;

    // Upload vertex data
    CGPUBufferRange vrange{};
    vrange.size = vbd.size;
    cgpu_buffer_map(vbuf, &vrange);
    if (vbuf->info->cpu_mapped_address)
        memcpy(vbuf->info->cpu_mapped_address, verts.data(), vbd.size);
    cgpu_buffer_unmap(vbuf);

    CGPUBufferId ibuf = CGPU_NULLPTR;
    if (icount > 0) {
        CGPUBufferDescriptor ibd{};
        ibd.name = "mesh_ib";
        ibd.size = icount * istride;
        ibd.descriptors = CGPU_RESOURCE_TYPE_INDEX_BUFFER;
        ibd.memory_usage = CGPU_MEMORY_USAGE_GPU_ONLY;
        ibd.flags = CGPU_BUFFER_CREATION_USAGE_PERSISTENT_MAP;
        ibuf = cgpu_device_create_buffer(device, &ibd);
        if (!ibuf) { cgpu_device_free_buffer(device, vbuf); return result; }

        CGPUBufferRange irange{};
        irange.size = ibd.size;
        cgpu_buffer_map(ibuf, &irange);
        if (ibuf->info->cpu_mapped_address)
            memcpy(ibuf->info->cpu_mapped_address, indices.data(), ibd.size);
        cgpu_buffer_unmap(ibuf);
    }

    // Build vertex layout
    CGPUVertexAttribute attrs[3] = {
        {"POSITION", 1, CGPU_VERTEX_FORMAT_FLOAT32X3, 0, offsetof(MeshTexturedVertex, position), sizeof(float)*3, CGPU_VERTEX_INPUT_RATE_VERTEX},
        {"NORMAL",   1, CGPU_VERTEX_FORMAT_FLOAT32X3, 0, offsetof(MeshTexturedVertex, normal),   sizeof(float)*3, CGPU_VERTEX_INPUT_RATE_VERTEX},
        {"TEXCOORD", 1, CGPU_VERTEX_FORMAT_FLOAT32X2, 0, offsetof(MeshTexturedVertex, texCoord), sizeof(float)*2, CGPU_VERTEX_INPUT_RATE_VERTEX},
    };
    CGPUVertexLayout layout{3, attrs};

    // Allocate pulse_asset slot
    pulse_asset_handle ah = pulse_asset_load_from_memory(app, PULSE_TYPE_MESH, "", nullptr, 0);
    if (ah.index == PULSE_ASSET_INVALID_INDEX) {
        cgpu_device_free_buffer(device, vbuf);
        if (ibuf) cgpu_device_free_buffer(device, ibuf);
        return result;
    }

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, ah, &ref)) {
        auto* mesh = static_cast<pulse_mesh_data_t*>(ref.ptr);
        mesh->vertex_layout = layout;
        mesh->vertex_stride = vstride;
        mesh->index_stride  = istride;
        mesh->vertices_count = vcount;
        mesh->index_count    = icount;
        mesh->prim_topology  = CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        mesh->vertex_buffer  = nullptr;
        mesh->index_buffer   = nullptr;
        pulse_asset_release(app, &ref);
    }

    result.asset = ah;
    return result;
}
