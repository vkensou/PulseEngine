#pragma once

#include "pulse_renderer.h"
#include "pulse_transform.h"
#include "pulse_window.h"

#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <atomic>
#include <memory_resource>

namespace pulse_renderer_internal {

enum EPulseSortFlag : uint32_t {
    PULSE_SORT_NONE = 0,
    PULSE_SORT_SHADER = 1u << 0,
    PULSE_SORT_MATERIAL = 1u << 1,
    PULSE_SORT_MESH = 1u << 2,
    PULSE_SORT_DEPTH_FRONT_TO_BACK = 1u << 3,
    PULSE_SORT_DEPTH_BACK_TO_FRONT = 1u << 4,
    PULSE_SORT_SUBMISSION_ORDER = 1u << 5,
};

constexpr uint32_t kDefaultListSortFlags = PULSE_SORT_SHADER | PULSE_SORT_MATERIAL | PULSE_SORT_MESH | PULSE_SORT_SUBMISSION_ORDER;

constexpr uint32_t kDrawItemMaxTextures = 4;

struct DrawItem {
    ecs_entity_t entity;
    PulseMeshHandle mesh;
    PulseMaterialHandle material;
    PulseShaderHandle shader;
    HMM_Mat4 world_matrix;
    size_t ubo_start{0}, ubo_end{0};
    float view_depth{0.0f};
    uint32_t submission_index{0};
    uint16_t feature_id{0};
    uint32_t feature_slot{0};
    uint32_t instance_first{0};
    uint32_t instance_count{0};
    PulseTextureHandle textures[kDrawItemMaxTextures];
    uint32_t texture_count{0};
};

struct RendererListDesc {
    const char* name;
    uint32_t sort_flags;
};

struct RendererList {
    RendererListDesc desc{};
    std::pmr::vector<DrawItem> items;
    std::pmr::vector<std::pmr::vector<std::byte>> feature_data;
    std::pmr::memory_resource* resource{nullptr};

    explicit RendererList(std::pmr::memory_resource* r) : items(r), feature_data(r), resource(r) {}

    void init_feature_data(uint32_t count) {
        feature_data.clear();
        std::pmr::polymorphic_allocator<std::byte> alloc(resource);
        for (uint32_t i = 0; i < count; ++i) feature_data.emplace_back(std::pmr::vector<std::byte>(alloc));
    }
};

// ============================================================
// Render features
// ============================================================

struct pulse_renderer_state;
struct RendererView;

struct FeatureExtractContext {
    pulse_renderer_state* state;
    uint32_t view_index;
    uint32_t list_id;
    uint32_t feature_id;
    void submit(const DrawItem& item);
    void submit(const DrawItem& item, const void* data);
};

struct FeaturePrepareContext {
    pulse_renderer_state* state;
    PulseAppId app;
    PulseRenderGraphId graph;
    uint32_t view_index;
    uint32_t list_id;
    uint32_t feature_id;
    HMM_Mat4 vp;
    RendererView& view();
    RendererList& list();
    std::pmr::vector<DrawItem>& items();
    void* slot_data(uint32_t slot);
};

struct FeatureDrawContext {
    const pulse_renderer_state* state;
    const RendererView* view;
    const RendererList* list;
    const DrawItem* item;
    const void* slot_data(uint32_t slot) const;
};

using FeatureExtractFn = void (*)(PulseAppId app, ecs_world_t* world, FeatureExtractContext& ctx, void* userdata);
using FeaturePrepareFn = void (*)(FeaturePrepareContext& ctx, void* userdata);
using FeatureDrawFn = void (*)(PulseAppId app, PulseRenderPassEncoder* encoder, FeatureDrawContext& ctx, void* userdata);

struct RenderFeature {
    const char* name;
    FeatureExtractFn extract;
    FeaturePrepareFn prepare;
    FeatureDrawFn draw;
    uint32_t data_size;
    void* userdata;
};

// ============================================================
// Dynamic UBO column (renderer-managed, per (set,binding))
// ============================================================

struct CachedUboBlock {
    uint8_t* data{nullptr};
    uint32_t size{0};
    uint32_t last_used_frame{0};
    bool in_use{false};
};

class UboBlockCache {
public:
    static constexpr uint32_t kMinBlockSize = 1 * 1024 * 1024;
    static constexpr uint32_t kMaxIdleFrames = 60 * 5; // ~5 seconds at 60 FPS

    UboBlockCache() = default;
    ~UboBlockCache() { reset(); }

    UboBlockCache(const UboBlockCache&) = delete;
    UboBlockCache& operator=(const UboBlockCache&) = delete;

    // Acquire a free cached buffer with at least `size` bytes.
    uint8_t* acquire(uint32_t size, uint32_t frame_index, uint32_t& out_block_size);

    // Called after all per-view GpuBlocks for this frame were destroyed.
    void frame_end();

    // Free blocks that have not been used for kMaxIdleFrames.
    void release_idle(uint32_t current_frame);

    void reset();

private:
    std::vector<CachedUboBlock> blocks_;
};

struct GpuBlock {
    uint32_t size{0};
    uint32_t used{0};
    uint8_t* cpu_data{nullptr};
    PulseRGBufferHandle gpu_handle{}; // rendergraph handle (valid only after block finalize)
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
    std::pmr::vector<RendererList> lists;
    std::pmr::vector<RendererUboColumn> ubo_columns;
    std::pmr::vector<GpuBlock> blocks;

    explicit RendererView(std::pmr::memory_resource* resource)
        : camera_entity(0), window_entity(0),
          view_matrix{}, proj_matrix{},
          fov(0), near_plane(0), far_plane(0), width(0), height(0),
          lists(resource), ubo_columns(resource), blocks(resource) {}
};

// ============================================================
// Double-buffered frame packet
// ============================================================
struct FrameRenderPacket {
    std::pmr::unsynchronized_pool_resource pool;
    UboBlockCache ubo_cache;
    std::pmr::vector<RendererView> views;

    explicit FrameRenderPacket(std::pmr::memory_resource* upstream = std::pmr::new_delete_resource())
        : pool(upstream), views(&pool) {}
};

// ============================================================
// Plugin internal state
// ============================================================
struct pulse_renderer_state {
    PulseAppId app = nullptr;
    PulseAssetSystemId assetSystem = nullptr;

    std::pmr::synchronized_pool_resource global_pool;
    FrameRenderPacket packets[2] = {
        FrameRenderPacket(&global_pool), FrameRenderPacket(&global_pool)};

    std::atomic<int> write_index{0};
    std::atomic<int> read_index{1};

    uint32_t frame_index{0};

    bool record_callback_registered = false;

    std::vector<RendererListDesc> list_registry;
    std::vector<RenderFeature> features;

    uint32_t register_render_list(const RendererListDesc& desc) {
        list_registry.push_back(desc);
        return (uint32_t)list_registry.size() - 1;
    }

    void register_feature(const char* name, FeatureExtractFn extract, FeaturePrepareFn prepare, FeatureDrawFn draw, uint32_t data_size, void* userdata) {
        features.push_back({ name, extract, prepare, draw, data_size, userdata });
    }

    // ECS system entities (ctx points at this state; deleted on shutdown)
    ecs_entity_t extract_cameras_system = 0;
    ecs_entity_t extract_features_system = 0;
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

inline void FeatureExtractContext::submit(const DrawItem& item) {
    submit(item, nullptr);
}

inline void FeatureExtractContext::submit(const DrawItem& item, const void* data) {
    RendererList& list = state->write_packet().views[view_index].lists[list_id];
    const size_t data_size = state->features[feature_id].data_size;
    std::pmr::vector<std::byte>& arena = list.feature_data[feature_id];
    DrawItem copy = item;
    copy.submission_index = (uint32_t)list.items.size();
    copy.feature_id = (uint16_t)feature_id;
    copy.feature_slot = data_size ? (uint32_t)(arena.size() / data_size) : 0;
    list.items.push_back(copy);
    if (data_size && data) {
        const size_t old_size = arena.size();
        arena.resize(old_size + data_size);
        memcpy(arena.data() + old_size, data, data_size);
    }
}

inline RendererView& FeaturePrepareContext::view() {
    return state->read_packet_mutable().views[view_index];
}

inline RendererList& FeaturePrepareContext::list() {
    return view().lists[list_id];
}

inline std::pmr::vector<DrawItem>& FeaturePrepareContext::items() {
    return list().items;
}

inline void* FeaturePrepareContext::slot_data(uint32_t slot) {
    std::pmr::vector<std::byte>& arena = list().feature_data[feature_id];
    const size_t size = state->features[feature_id].data_size;
    if (size == 0 || (size_t)(slot + 1) * size > arena.size()) return nullptr;
    return arena.data() + (size_t)slot * size;
}

inline const void* FeatureDrawContext::slot_data(uint32_t slot) const {
    const std::pmr::vector<std::byte>& arena = list->feature_data[item->feature_id];
    const size_t size = state->features[item->feature_id].data_size;
    if (size == 0 || (size_t)(slot + 1) * size > arena.size()) return nullptr;
    return arena.data() + (size_t)slot * size;
}

// ============================================================
// Component registration and system installation
// ============================================================
void register_renderer_components(ecs_world_t* world);
void install_renderer_systems(ecs_world_t* world, pulse_renderer_state* state);

void draw_item_default(PulseAppId app, PulseRenderPassEncoder* encoder, const RendererView& view, const DrawItem& item);

void install_renderable_feature(pulse_renderer_state* state, ecs_world_t* world);
void shutdown_renderable_feature(void* userdata);

pulse_renderer_state* state_from_app(PulseAppId app);

} // namespace pulse_renderer_internal

struct pulse_renderer_state_resource {
    pulse_renderer_internal::pulse_renderer_state* state;
};
extern ECS_COMPONENT_DECLARE(pulse_renderer_state_resource);
