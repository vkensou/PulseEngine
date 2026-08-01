#pragma once

#include "pulse_renderer.h"
#include "pulse_transform.h"
#include "pulse_window.h"
#include "pulse_renderer_asset.h"

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
    HMM_Mat4 world_matrix;      // cached from transform
};

// ============================================================
// Dynamic UBO column (renderer-managed, per (set,binding))
// ============================================================
struct RendererUboColumn {
    const struct pulse_shader_data_t* shader; // owning shader
    uint32_t set;
    uint32_t binding;
    uint32_t stride;            // per-object byte size (0 = not per-draw)
    uint64_t layout_hash;       // from shader's ubo_info
    uint64_t data_hash;         // hash of the filled data (for lazy switching)
    std::vector<uint8_t> cpu_data;
    pulse_buffer_handle_t gpu_handle; // rendergraph handle
    bool is_per_draw;           // true if holds per-object array
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
    // renderer-managed UBO columns for this view
    std::vector<RendererUboColumn> ubo_columns;
};

// ============================================================
// Double-buffered frame packet
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

    // Property name mapping: EPulseRendererPropertyType → shader property name
    const char* property_names[PULSE_RENDERER_PROPERTY_TYPE_COUNT] = {};

    void swap_packets() {
        int w = write_index.load(std::memory_order_relaxed);
        write_index.store(1 - w, std::memory_order_release);
        read_index.store(w, std::memory_order_release);
    }

    FrameRenderPacket& write_packet() { return packets[write_index.load(std::memory_order_relaxed)]; }
    const FrameRenderPacket& read_packet() const { return packets[read_index.load(std::memory_order_acquire)]; }
    FrameRenderPacket& read_packet_mutable() { return packets[read_index.load(std::memory_order_acquire)]; }
};

// ============================================================
// Component registration and system installation
// ============================================================
void register_renderer_components(ecs_world_t* world);
void install_renderer_systems(ecs_world_t* world, pulse_renderer_state* state);

pulse_renderer_state* state_from_app(PulseAppId app);

} // namespace pulse_renderer_internal

struct pulse_renderer_state_resource {
    pulse_renderer_internal::pulse_renderer_state* state;
};
extern ECS_COMPONENT_DECLARE(pulse_renderer_state_resource);
