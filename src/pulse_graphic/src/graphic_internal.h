#pragma once

#include "pulse_graphic.h"
#include "pulse_cgpu_render.h"
#include "pulse_asset.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace pulse_graphic_internal {

extern const char* kPluginName;

enum UploadContentType {
    UPLOAD_TEXTURE = 0,
    UPLOAD_BUFFER = 1,
    UPLOAD_BUFFER_DATA = 2,
};

struct UploadEntry {
    int content;  // UploadContentType
    pulse_texture_t texture;
    pulse_buffer_t buffer;
    pulse_buffer_data_t* buffer_data = nullptr;
    const void* data = nullptr;
    uint64_t data_size = 0;
    bool* completed = nullptr;
};

struct pulse_graphic_state {
    pulse_app_t app = nullptr;
    bool upload_pending = false;

    std::vector<UploadEntry> pending_uploads;
    std::vector<UploadEntry> dynamic_updates;
    std::unordered_map<uint64_t, bool> upload_pending_map;

    // Deferred upload staging (double-buffered for GPU fence lifecycle)
    std::vector<std::vector<uint8_t>> staging_pool[2];
    int staging_write = 0;
};

pulse_graphic_state* state_from_app(pulse_app_t app);
CGPUDeviceId get_device(pulse_app_t app);
bool is_upload_pending(pulse_app_t app, pulse_asset_handle handle);
void clear_upload_pending(pulse_app_t app, pulse_asset_handle handle);
void material_internal_destroy(void* data);
void install_upload_callback(pulse_app_t app);

// Mesh loader entry points (defined in load_mesh.cpp)
pulse_result_t start_mesh(const pulse_asset_load_task* ctx, void** out_state, void* user_data);
pulse_asset_loader_status_t step_mesh(
    const pulse_asset_load_task* ctx, void* state, void* out_asset,
    const char** out_error, void* user_data);
void destroy_mesh_loader_state(void* state, void* user_data);

} // namespace pulse_graphic_internal
