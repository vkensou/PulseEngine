#pragma once

#include "pulse_graphics.h"
#include "pulse_asset.h"
#include <vector>
#include <deque>
#include <unordered_map>
#include <cstdint>
#include <memory_resource>
#include "renderer.h"
#include "rendergraph_cpp.h"

namespace pulse_graphics_internal {

extern const char* kPluginName;

struct frame_data {
    CGPUFenceId fence = CGPU_NULLPTR;
    CGPUCommandPoolId pool = CGPU_NULLPTR;
    std::vector<CGPUCommandBufferId> available_cmds;
    std::vector<CGPUCommandBufferId> submitted_cmds;
    std::unique_ptr<std::pmr::unsynchronized_pool_resource> exec_memory;
    std::unique_ptr<HGEGraphics::ExecutorContext> exec_context;

    bool init(PulseAssetSystemId asset_system, CGPUDeviceId device, CGPUQueueId queue, CGPUTextureViewId default_texture, CGPUSamplerId default_sampler);
    void begin_frame();
    CGPUCommandBufferId request_command_buffer();
    void destroy();
};

struct render_frame_context {
    uint32_t frame_index = 0;
    frame_data* frame = nullptr;
    bool active = false;
    bool submitted = false;
    bool failed = false;
    std::unique_ptr<std::pmr::unsynchronized_pool_resource> graph_memory;
    std::unique_ptr<HGEGraphics::rendergraph_t> graph;
    std::vector<ecs_entity_t> prepared_entities;
    std::vector<CGPUSemaphoreId> wait_semaphores;
    std::vector<CGPUSemaphoreId> signal_semaphores;

    void reset();
};

enum UploadContentType {
    UPLOAD_TEXTURE = 0,
    UPLOAD_BUFFER = 1,
    UPLOAD_TEXTURE_DATA = 2,
    UPLOAD_BUFFER_DATA = 3,
};

struct UploadEntry {
    int content;  // UploadContentType
    PulseTextureHandle texture;
    PulseGraphicsBufferHandle buffer;
    PulseTextureData* texture_data = nullptr;
    PulseGraphicsBufferData* buffer_data = nullptr;
    const void* data = nullptr;
    uint64_t data_size = 0;
    bool* completed = nullptr;
    uint8_t source_mip_levels = 1;
    bool generate_mipmap = false;
};

struct pulse_graphics_state {
    PulseAppId app = nullptr;
    PulseAssetSystemId asset_system = nullptr;

    PulseGraphicsPluginDesc desc{};
    PulseRenderer renderer{};
    std::vector<frame_data> frames;
    render_frame_context frame_context;
    ecs_entity_t sdl_window_on_set_observer = 0;
    ecs_entity_t sdl_window_on_remove_observer = 0;
    ecs_entity_t window_on_set_observer = 0;
    ecs_entity_t begin_frame_system = 0;
    ecs_entity_t reset_backbuffer_system = 0;
    ecs_entity_t prepare_windows_system = 0;
    ecs_entity_t build_graph_system = 0;
    ecs_entity_t execute_graph_system = 0;
    ecs_entity_t submit_system = 0;
    ecs_entity_t present_system = 0;
    bool existing_sdl_windows_bootstrapped = false;
    std::vector<PulseRenderRecordCallbackDesc> record_callbacks;

    PulseShader blit_shader;
    PulseSampler blit_linear_sampler;

    PulseSampler default_sampler;

    void sort_record_callbacks();

    bool upload_pending = false;

    std::deque<UploadEntry> pending_uploads;
    std::vector<UploadEntry> dynamic_updates;
    std::unordered_map<uint64_t, bool> upload_pending_map;

    std::pmr::unsynchronized_pool_resource staging_pool;

    struct DeferredFree {
        void* ptr;
        size_t size;
    };
    std::vector<DeferredFree> pending_release;
};

struct pulse_graphics_state_resource {
    pulse_graphics_state* state;
};
extern ECS_COMPONENT_DECLARE(pulse_graphics_state_resource);

pulse_graphics_state* state_from_app(PulseAppId app);
pulse_graphics_state* state_from_world(ecs_world_t* world);
CGPUDeviceId get_device(PulseAppId app);
bool is_upload_pending(PulseAppId app, PulseAssetHandle handle);
void clear_upload_pending(PulseAppId app, PulseAssetHandle handle);
void install_upload_callback(PulseAppId app);
void register_graphics_asset_types_and_loaders(PulseAssetSystemId asset_system, CGPUDeviceId device);

inline PulseAssetSystemId asset_system_from_app(PulseAppId app) {
    pulse_graphics_state* st = state_from_app(app);
    return st ? st->asset_system : nullptr;
}

inline PulseAssetRequest asset_load_path(
    PulseAssetSystemId asset_system,
    uint64_t type_id,
    const char* path,
    const void* settings = nullptr
) {
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    desc.settings = settings;
    return pulse_asset_system_load(asset_system, &desc);
}

inline PulseAssetRequest asset_load_memory_path(
    PulseAssetSystemId asset_system,
    uint64_t type_id,
    const char* path,
    const void* data,
    uint64_t size,
    const void* settings = nullptr
) {
    PulseAssetMemoryLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetMemoryLoadDesc);
    desc.version = PULSE_ASSET_MEMORY_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    desc.data = data;
    desc.size = size;
    desc.settings = settings;
    return pulse_asset_system_load_from_memory(asset_system, &desc);
}

inline PulseAssetRequest asset_build(
    PulseAssetSystemId asset_system,
    uint64_t type_id,
    const char* name = nullptr,
    const PulseAssetDependency* dependencies = nullptr,
    uint32_t dependency_count = 0,
    const void* settings = nullptr
) {
    PulseAssetBuildDesc desc{};
    desc.struct_size = sizeof(PulseAssetBuildDesc);
    desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    desc.type_id = type_id;
    desc.name = name;
    desc.dependencies = dependencies;
    desc.dependency_count = dependency_count;
    desc.settings = settings;
    return pulse_asset_system_build(asset_system, &desc);
}

inline PulseAssetHandle asset_build_sync(
    PulseAssetSystemId asset_system,
    uint64_t type_id,
    const char* name = nullptr,
    const PulseAssetDependency* dependencies = nullptr,
    uint32_t dependency_count = 0,
    const void* settings = nullptr
) {
    PulseAssetBuildDesc desc{};
    desc.struct_size = sizeof(PulseAssetBuildDesc);
    desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    desc.type_id = type_id;
    desc.name = name;
    desc.dependencies = dependencies;
    desc.dependency_count = dependency_count;
    desc.settings = settings;
    return pulse_asset_system_build_sync(asset_system, &desc);
}

// Inline helpers for loader callbacks (use PulseAssetSystemId directly)
inline PulseShaderData* internal_borrow_shader(PulseAssetSystemId as, PulseShaderHandle handle) {
    void* ptr = nullptr;
    return pulse_asset_system_borrow(as, pulse_shader_to_handle(handle), &ptr, nullptr) ? static_cast<PulseShaderData*>(ptr) : nullptr;
}
inline PulseShaderLibraryData* internal_borrow_shader_library(PulseAssetSystemId as, PulseShaderLibraryHandle handle) {
    void* ptr = nullptr;
    return pulse_asset_system_borrow(as, pulse_shader_library_to_handle(handle), &ptr, nullptr) ? static_cast<PulseShaderLibraryData*>(ptr) : nullptr;
}
inline PulseGraphicsBufferData* internal_borrow_buffer(PulseAssetSystemId as, PulseGraphicsBufferHandle handle) {
    void* ptr = nullptr;
    return pulse_asset_system_borrow(as, pulse_graphics_buffer_to_handle(handle), &ptr, nullptr) ? static_cast<PulseGraphicsBufferData*>(ptr) : nullptr;
}
inline PulseTextureData* internal_borrow_texture(PulseAssetSystemId as, PulseTextureHandle handle) {
    void* ptr = nullptr;
    return pulse_asset_system_borrow(as, pulse_texture_to_handle(handle), &ptr, nullptr) ? static_cast<PulseTextureData*>(ptr) : nullptr;
}
inline PulseMeshData* internal_borrow_mesh(PulseAssetSystemId as, PulseMeshHandle handle) {
    void* ptr = nullptr;
    return pulse_asset_system_borrow(as, pulse_mesh_to_handle(handle), &ptr, nullptr) ? static_cast<PulseMeshData*>(ptr) : nullptr;
}
inline PulseComputeShaderData* internal_borrow_compute_shader(PulseAssetSystemId as, PulseComputeShaderHandle handle) {
    void* ptr = nullptr;
    return pulse_asset_system_borrow(as, pulse_compute_shader_to_handle(handle), &ptr, nullptr) ? static_cast<PulseComputeShaderData*>(ptr) : nullptr;
}
inline PulseMaterialData* internal_borrow_material(PulseAssetSystemId as, PulseMaterialHandle handle) {
    void* ptr = nullptr;
    return pulse_asset_system_borrow(as, pulse_material_to_handle(handle), &ptr, nullptr) ? static_cast<PulseMaterialData*>(ptr) : nullptr;
}
inline PulseSamplerData* internal_borrow_sampler(PulseAssetSystemId as, PulseSamplerHandle handle) {
    void* ptr = nullptr;
    return pulse_asset_system_borrow(as, pulse_sampler_to_handle(handle), &ptr, nullptr) ? static_cast<PulseSamplerData*>(ptr) : nullptr;
}

uint8_t* queue_staging_texture_full(
    pulse_graphics_state* gstate,
    PulseTextureHandle handle,
    PulseTextureData* texture,
    uint8_t source_mip_levels,
    bool generate_mipmap,
    uint64_t* out_size,
    bool* completed);

uint8_t* queue_staging_buffer_full(
    pulse_graphics_state* gstate,
    PulseGraphicsBufferHandle handle,
    PulseGraphicsBufferData* buffer,
    uint64_t size,
    bool* completed);


void register_texture_type(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_texture_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_texture_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_buffer_type(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_buffer_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_material_type(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_material_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);

void register_mesh_type(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_mesh_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_mesh_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);

void register_shader_type(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_shader_create_loaders(PulseAssetSystemId asset_system, CGPUDeviceId device);
EPulseResult ctor_shader_from_deps(void* state, const PulseAssetLoadTask* ctx);
void register_compute_shader_type(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_compute_shader_create_loaders(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_shader_library_type(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_shader_library_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_shader_library_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);

void register_sampler_type(PulseAssetSystemId asset_system, CGPUDeviceId device);
void register_sampler_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device);

} // namespace pulse_graphics_internal
