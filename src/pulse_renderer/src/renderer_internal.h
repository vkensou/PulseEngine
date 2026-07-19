#pragma once

#include "pulse_renderer.h"
#include "pulse_transform.h"
#include "pulse_window.h"

#include <vector>
#include <cstdint>
#include <atomic>

namespace pulse_renderer_internal {

// ============================================================
// Per-object render data (sorted for minimal state switch)
// ============================================================
struct RenderObject {
    uint64_t sort_key;          // packed: material_index<<32 | mesh_index
    ecs_entity_t entity;
    PulseMeshHandle mesh;
    PulseMaterialHandle material;
};

// ============================================================
// Uniform data layouts (must match shaders)
// ============================================================
struct CameraUniformData {
    HMM_Mat4 view_proj;
};

struct ObjectUniformData {
    HMM_Mat4 model;
};

// ============================================================
// Per-camera view data (built during extraction phase)
// ============================================================
struct RendererView {
    ecs_entity_t camera_entity;
    ecs_entity_t window_entity;
    HMM_Mat4 view_matrix;
    HMM_Mat4 proj_matrix;
    float fov;
    float near_plane;
    float far_plane;
    int width;
    int height;
    std::vector<RenderObject> render_objects;
    CameraUniformData cam_data;
    std::pmr::vector<ObjectUniformData> render_data;
};

// ============================================================
// Double-buffered frame packet
//   - write_index: logic thread writes this slot
//   - read_index:  render callback reads this slot
// ============================================================
struct FrameRenderPacket {
    std::vector<RendererView> views;
};

// ============================================================
// Plugin internal state
// ============================================================
struct pulse_renderer_state {
    PulseAppId app = nullptr;

    FrameRenderPacket packets[2];
    std::atomic<int> write_index{0};
    std::atomic<int> read_index{1};

    bool record_callback_registered = false;

    void swap_packets() {
        int w = write_index.load(std::memory_order_relaxed);
        write_index.store(1 - w, std::memory_order_release);
        read_index.store(w, std::memory_order_release);
    }

    FrameRenderPacket& write_packet() { return packets[write_index.load(std::memory_order_relaxed)]; }
    const FrameRenderPacket& read_packet() const { return packets[read_index.load(std::memory_order_acquire)]; }
};

// ============================================================
// Component registration and system installation
// ============================================================
void register_renderer_components(ecs_world_t* world);
void install_renderer_systems(ecs_world_t* world, pulse_renderer_state* state);

} // namespace pulse_renderer_internal
