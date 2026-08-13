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
    HMM_Mat4 world_matrix;      // cached from transform
    size_t ubo_start{0}, ubo_end{0};
};

// ============================================================
// Dynamic UBO column (renderer-managed, per (set,binding))
// ============================================================

struct GpuBlock {
    uint32_t size;
    uint32_t used;
    std::vector<uint8_t> cpu_data;
    PulseRGBufferHandle gpu_handle; // rendergraph handle
};

struct GpuBlockRef {
    size_t index;
    size_t offset;
    size_t size;
    uint8_t* ptr;
};

struct RendererUboColumn {
    PulseMaterialHandle material;
    PulseShaderHandle shader;
    uint64_t layout_hash;
    uint32_t ubo_info_index;
    uint32_t set;
    uint32_t binding;
    GpuBlockRef block_ref;
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
    std::vector<GpuBlock> blocks;
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
    PulseAssetSystemId assetSystem = nullptr;

    FrameRenderPacket packets[2];
    std::atomic<int> write_index{0};
    std::atomic<int> read_index{1};

    bool record_callback_registered = false;

    // ECS system entities (ctx points at this state; deleted on shutdown)
    ecs_entity_t extract_cameras_system = 0;
    ecs_entity_t collect_renderables_system = 0;
    ecs_entity_t sort_and_pack_system = 0;
    ecs_entity_t packets_swap_system = 0;

    // Property name mapping: EPulseRendererPropertyType → shader property name
    const char* property_names[PULSE_RENDERER_PROPERTY_TYPE_COUNT] = {};

    // Device UBO offset alignment, queried once at plugin init (see renderer_plugin_post_build)
    uint32_t ubo_alignment = 256;

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
