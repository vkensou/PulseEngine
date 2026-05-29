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

    bool init(CGPUDeviceId device, CGPUQueueId queue);
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
    pulse_texture_t texture;
    pulse_buffer_t buffer;
    pulse_texture_data_t* texture_data = nullptr;
    pulse_buffer_data_t* buffer_data = nullptr;
    const void* data = nullptr;
    uint64_t data_size = 0;
    bool* completed = nullptr;
    uint8_t source_mip_levels = 1;
    bool generate_mipmap = false;
};

struct pulse_graphics_state {
    pulse_app_t app = nullptr;

    pulse_graphics_plugin_desc desc{};
    pulse_graphics_renderer renderer{};
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
    std::vector<pulse_graphics_renderer_record_callback_desc> record_callbacks;

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

pulse_graphics_state* state_from_app(pulse_app_t app);
CGPUDeviceId get_device(pulse_app_t app);
bool is_upload_pending(pulse_app_t app, pulse_asset_handle handle);
void clear_upload_pending(pulse_app_t app, pulse_asset_handle handle);
void install_upload_callback(pulse_app_t app);

inline pulse_asset_handle asset_load_path(
    pulse_app_t app,
    uint64_t type_id,
    const char* path,
    const void* settings = nullptr
) {
    pulse_asset_load_desc desc{};
    desc.struct_size = sizeof(pulse_asset_load_desc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    desc.settings = settings;
    return pulse_asset_load(app, &desc);
}

inline pulse_asset_handle asset_load_memory_path(
    pulse_app_t app,
    uint64_t type_id,
    const char* path,
    const void* data,
    uint64_t size,
    const void* settings = nullptr
) {
    pulse_asset_memory_load_desc desc{};
    desc.struct_size = sizeof(pulse_asset_memory_load_desc);
    desc.version = PULSE_ASSET_MEMORY_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    desc.data = data;
    desc.size = size;
    desc.settings = settings;
    return pulse_asset_load_from_memory(app, &desc);
}

inline pulse_asset_handle asset_build(
    pulse_app_t app,
    uint64_t type_id,
    const char* name = nullptr,
    const pulse_asset_dependency* dependencies = nullptr,
    uint32_t dependency_count = 0,
    const void* settings = nullptr
) {
    pulse_asset_build_desc desc{};
    desc.struct_size = sizeof(pulse_asset_build_desc);
    desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    desc.type_id = type_id;
    desc.name = name;
    desc.dependencies = dependencies;
    desc.dependency_count = dependency_count;
    desc.settings = settings;
    return pulse_asset_build(app, &desc);
}

uint8_t* queue_staging_texture_full(
    pulse_graphics_state* gstate,
    pulse_texture_data_t* texture,
    uint8_t source_mip_levels,
    bool generate_mipmap,
    uint64_t* out_size,
    bool* completed);

uint8_t* queue_staging_buffer_full(
    pulse_graphics_state* gstate,
    pulse_buffer_data_t* buffer,
    uint64_t size,
    bool* completed);


void register_texture_type(pulse_app_t app, CGPUDeviceId device);
void register_texture_create_loader(pulse_app_t app, CGPUDeviceId device);
void register_texture_load_loader(pulse_app_t app, CGPUDeviceId device);
void register_buffer_type(pulse_app_t app, CGPUDeviceId device);
void register_buffer_create_loader(pulse_app_t app, CGPUDeviceId device);
void register_material_type(pulse_app_t app, CGPUDeviceId device);
void register_material_create_loader(pulse_app_t app, CGPUDeviceId device);

void register_mesh_type(pulse_app_t app, CGPUDeviceId device);
void register_mesh_create_loader(pulse_app_t app, CGPUDeviceId device);
void register_mesh_load_loader(pulse_app_t app, CGPUDeviceId device);

void register_shader_type(pulse_app_t app, CGPUDeviceId device);
void register_shader_create_loaders(pulse_app_t app, CGPUDeviceId device);
pulse_result_t ctor_shader_from_deps(void* state, const pulse_asset_load_task* ctx);
void register_compute_shader_type(pulse_app_t app, CGPUDeviceId device);
void register_compute_shader_create_loaders(pulse_app_t app, CGPUDeviceId device);
void register_shader_library_type(pulse_app_t app, CGPUDeviceId device);
void register_shader_library_load_loader(pulse_app_t app, CGPUDeviceId device);
void register_shader_library_create_loader(pulse_app_t app, CGPUDeviceId device);

} // namespace pulse_graphics_internal
