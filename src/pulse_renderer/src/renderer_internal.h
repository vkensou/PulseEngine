#pragma once

#include "pulse_renderer.h"
#include "pulse_transform.h"
#include "pulse_window.h"

#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cassert>
#include <atomic>
#include <memory_resource>

namespace pulse_renderer_internal {

enum EPulseSortFlag : uint32_t {
    PULSE_SORT_NONE = 0,
    PULSE_SORT_ORDER = 1u << 6,
    PULSE_SORT_SHADER = 1u << 0,
    PULSE_SORT_MATERIAL = 1u << 1,
    PULSE_SORT_MESH = 1u << 2,
    PULSE_SORT_DEPTH_FRONT_TO_BACK = 1u << 3,
    PULSE_SORT_DEPTH_BACK_TO_FRONT = 1u << 4,
    PULSE_SORT_SUBMISSION_ORDER = 1u << 5,
};

constexpr uint32_t kDefaultListSortFlags = PULSE_SORT_ORDER | PULSE_SORT_SHADER | PULSE_SORT_MATERIAL | PULSE_SORT_MESH | PULSE_SORT_SUBMISSION_ORDER;

constexpr const char* kPropertyNameVPMatrix = "vpMatrix";
constexpr const char* kPropertyNameModelMatrix = "wMatrix";

struct RendererPropertyValue {
    const char* name;
    EPulseShaderPropertyType type;
    uint32_t size;
    std::byte value[64];
};

struct RendererPropertyTable {
    std::pmr::vector<RendererPropertyValue> entries;

    explicit RendererPropertyTable(std::pmr::memory_resource* r) : entries(r) {}

    void set_mat4(const char* name, const HMM_Mat4& v) {
        RendererPropertyValue* entry = const_cast<RendererPropertyValue*>(find(name));
        if (!entry) {
            RendererPropertyValue new_entry = {};
            new_entry.name = name;
            new_entry.type = PULSE_SHADER_PROPERTY_TYPE_MAT4;
            new_entry.size = sizeof(HMM_Mat4);
            entries.push_back(new_entry);
            entry = &entries.back();
        }
        entry->type = PULSE_SHADER_PROPERTY_TYPE_MAT4;
        entry->size = sizeof(HMM_Mat4);
        memcpy(entry->value, &v, sizeof(HMM_Mat4));
    }

    const RendererPropertyValue* find(const char* name) const {
        for (const RendererPropertyValue& entry : entries) {
            if (strcmp(entry.name, name) == 0) return &entry;
        }
        return nullptr;
    }
};

// ============================================================
// Extraction snapshot (swapped, immutable afterwards)
// ============================================================

struct StagingItem {
    ecs_entity_t entity;
    PulseMeshHandle mesh;
    PulseMaterialHandle material;
    PulseShaderHandle shader;
    HMM_Mat4 world_matrix;
    uint32_t instance_count;
    uint32_t data_slot;
    int32_t sort_order;
};

struct FeatureStaging {
    std::pmr::vector<StagingItem> items;
    std::pmr::vector<std::byte> data_arena;

    explicit FeatureStaging(std::pmr::memory_resource* r) : items(r), data_arena(r) {
        items.reserve(256);
        data_arena.reserve(64 * 1024);
    }
};

struct CameraSnapshot {
    ecs_entity_t camera_entity;
    ecs_entity_t window_entity;
    HMM_Mat4 view_matrix;
    HMM_Mat4 proj_matrix;
    float fov;
    float near_plane;
    float far_plane;
    float orthographic_size;
    bool orthographic;
    int width;
    int height;
    uint32_t clear_color;
};

struct FrameRenderPacket {
    std::pmr::unsynchronized_pool_resource pool;
    std::pmr::vector<CameraSnapshot> cameras;
    std::pmr::vector<FeatureStaging> staging;

    explicit FrameRenderPacket(std::pmr::memory_resource* upstream = std::pmr::new_delete_resource())
        : pool(upstream), cameras(&pool), staging(&pool) {}

    void grow_staging(std::pmr::memory_resource* resource, uint32_t feature_count) {
        while (staging.size() < feature_count) staging.emplace_back(resource);
    }
};

// ============================================================
// Derived frame data (built once per frame, read by record/execute)
// ============================================================

struct DrawItem {
    uint32_t staging_index{0};
    uint32_t data_slot{0};
    float view_depth{0.0f};
    uint32_t submission_index{0};
    uint32_t global_column_start{0};
    uint32_t global_column_count{0};
    uint32_t feature_column_start{0};
    uint32_t feature_column_count{0};
    uint16_t feature_id{0};
};

struct RendererListDesc {
    const char* name;
    uint32_t sort_flags;
};

struct RendererGlobalColumns {
    PulseShaderHandle shader;
    uint32_t first_column;
    uint32_t column_count;
};

struct RendererList {
    RendererListDesc desc{};
    std::pmr::vector<DrawItem> items;
    std::pmr::vector<RendererGlobalColumns> global_columns;

    explicit RendererList(std::pmr::memory_resource* r) : items(r), global_columns(r) {}
};

struct pulse_renderer_state;
struct FrameRenderPacket;

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
    static constexpr uint32_t kMaxIdleFrames = 60 * 5;

    UboBlockCache() = default;
    ~UboBlockCache() { reset(); }

    UboBlockCache(const UboBlockCache&) = delete;
    UboBlockCache& operator=(const UboBlockCache&) = delete;

    uint8_t* acquire(uint32_t size, uint32_t frame_index, uint32_t& out_block_size);
    void frame_end();
    void release_idle(uint32_t current_frame);
    void reset();

private:
    std::vector<CachedUboBlock> blocks_;
};

struct GpuBlock {
    uint32_t size{0};
    uint32_t used{0};
    uint8_t* cpu_data{nullptr};
    PulseRGBufferHandle gpu_handle{};
};

struct GpuBlockRef {
    size_t index;
    size_t offset;
    size_t size;
    uint8_t* ptr;
};

struct RendererUboColumn {
    uint32_t set;
    uint32_t binding;
    GpuBlockRef block_ref;
};

template <typename ResolveFn>
void fill_ubo_block(PulseAppId app, PulseShaderHandle shader, uint32_t set, uint32_t binding, const GpuBlockRef& block, ResolveFn resolve) {
    for (uint32_t p = 0; p < pulse_shader_get_shader_property_count(app, shader); ++p) {
        const PulseShaderProperty prop = pulse_shader_get_shader_property(app, shader, p);
        if (!prop.name || prop.set != set || prop.binding != binding) continue;
        const RendererPropertyValue* value = resolve(prop.name);
        if (!value || value->type != prop.type || value->size != prop.size) continue;
        memcpy(block.ptr + prop.offset, value->value, prop.size);
    }
}

struct ViewFrameData {
    std::pmr::vector<RendererList> lists;
    std::pmr::vector<RendererUboColumn> ubo_columns;
    std::pmr::vector<GpuBlock> blocks;
    RendererPropertyTable properties;

    explicit ViewFrameData(std::pmr::memory_resource* r) : lists(r), ubo_columns(r), blocks(r), properties(r) {}
};

struct FrameViewData {
    std::pmr::unsynchronized_pool_resource pool;
    const FrameRenderPacket* snapshot{nullptr};
    std::pmr::vector<ViewFrameData> views;

    FrameViewData() : pool(std::pmr::new_delete_resource()), views(&pool) {}

    void reset(const FrameRenderPacket* packet) {
        views.clear();
        snapshot = packet;
    }
};

// ============================================================
// Render features
// ============================================================

GpuBlockRef alloc_ubo_block(pulse_renderer_state& state, ViewFrameData& view, uint32_t size);
uint32_t alloc_feature_ubo_column(PulseFeaturePrepareContext& ctx, DrawItem& item, uint32_t set, uint32_t binding, uint32_t size);
void bind_item_ubo_columns(PulseRenderPassEncoder* encoder, const ViewFrameData& view, const DrawItem& item);
void collect_feature_items(PulseFeatureRecordContext& ctx);
const void* feature_item_data(const pulse_renderer_state* state, const FrameRenderPacket* snapshot, uint32_t feature_id, uint32_t data_slot);
PulseFeatureItem to_public_feature_item(const StagingItem& staging);

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

    FrameViewData view_data;
    UboBlockCache ubo_cache;

    uint32_t frame_index{0};

    bool record_callback_registered = false;

    std::vector<RendererListDesc> list_registry;
    std::vector<PulseRenderFeatureDesc> features;

    uint32_t register_render_list(const RendererListDesc& desc) {
        list_registry.push_back(desc);
        return (uint32_t)list_registry.size() - 1;
    }

    ecs_entity_t begin_extract_system = 0;
    ecs_entity_t extract_cameras_system = 0;
    ecs_entity_t extract_features_system = 0;
    ecs_entity_t packets_swap_system = 0;
    ecs_entity_t build_views_system = 0;

    uint32_t ubo_alignment = 256;

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

void install_renderable_feature(PulseAppId app, ecs_world_t* world);

pulse_renderer_state* state_from_app(PulseAppId app);

} // namespace pulse_renderer_internal

struct PulseFeatureExtractContext {
    pulse_renderer_internal::pulse_renderer_state* state;
    uint32_t feature_id;
};

struct PulseFeatureCullContext {
    pulse_renderer_internal::pulse_renderer_state* state;
    const pulse_renderer_internal::FrameRenderPacket* snapshot;
    const pulse_renderer_internal::CameraSnapshot* camera;
    uint32_t view_index;
    uint32_t feature_id;
};

struct PulseFeaturePrepareContext {
    pulse_renderer_internal::pulse_renderer_state* state;
    pulse_renderer_internal::ViewFrameData* view;
    const pulse_renderer_internal::FrameRenderPacket* snapshot;
    pulse_renderer_internal::RendererList* list;
    uint32_t feature_id;
};

struct PulseFeatureDrawContext {
    const pulse_renderer_internal::pulse_renderer_state* state;
    const pulse_renderer_internal::FrameRenderPacket* snapshot;
    const pulse_renderer_internal::ViewFrameData* view;
    const pulse_renderer_internal::DrawItem* item;
    PulseFeatureItem feature_item;
};

struct PulseFeatureRecordContext {
    pulse_renderer_internal::pulse_renderer_state* state;
    const pulse_renderer_internal::FrameRenderPacket* snapshot;
    const pulse_renderer_internal::ViewFrameData* view;
    uint32_t feature_id;
    std::pmr::vector<const pulse_renderer_internal::DrawItem*> items;

    PulseFeatureRecordContext(pulse_renderer_internal::pulse_renderer_state* s, const pulse_renderer_internal::FrameRenderPacket* snap, const pulse_renderer_internal::ViewFrameData* v, uint32_t fi, std::pmr::memory_resource* r)
        : state(s), snapshot(snap), view(v), feature_id(fi), items(r) {}
};

struct pulse_renderer_state_resource {
    pulse_renderer_internal::pulse_renderer_state* state;
};
extern ECS_COMPONENT_DECLARE(pulse_renderer_state_resource);
