#pragma once

#include "pulse_graphic.h"
#include "pulse_cgpu_render.h"
#include "pulse_asset.h"
#include <vector>
#include <deque>
#include <unordered_map>
#include <cstdint>
#include <memory_resource>

namespace pulse_graphic_internal {

extern const char* kPluginName;

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

struct pulse_graphic_state {
    pulse_app_t app = nullptr;
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

pulse_graphic_state* state_from_app(pulse_app_t app);
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
    pulse_graphic_state* gstate,
    pulse_texture_data_t* texture,
    uint8_t source_mip_levels,
    bool generate_mipmap,
    uint64_t* out_size,
    bool* completed);

uint8_t* queue_staging_buffer_full(
    pulse_graphic_state* gstate,
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

} // namespace pulse_graphic_internal
