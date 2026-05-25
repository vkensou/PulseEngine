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
void material_internal_destroy(void* data);
void install_upload_callback(pulse_app_t app);

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

struct TextureLoaderState {
    bool upload_requested = false;
    bool upload_completed = false;
};

pulse_asset_loader_status_t step_texture_stb(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error);
pulse_asset_loader_status_t step_texture_ktx(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error);

struct MeshLoaderState {
    bool upload_requested = false;
    bool has_ib = false;
    bool vb_completed = false;
    bool ib_completed = false;
};

pulse_asset_loader_status_t step_mesh(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error);

} // namespace pulse_graphic_internal
