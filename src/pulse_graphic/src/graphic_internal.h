#pragma once

#include "pulse_graphic.h"
#include "pulse_cgpu_render.h"
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace pulse_graphic_internal {

extern const char* kPluginName;

struct pulse_graphic_state {
    pulse_app_t app = nullptr;
    bool upload_pending = false;

    struct UploadEntry {
        pulse_asset_handle handle;
        bool is_texture;
        const void* data = nullptr;
        uint64_t data_size = 0;
    };
    std::vector<UploadEntry> pending_uploads;
    std::vector<UploadEntry> dynamic_updates;
    std::unordered_map<uint64_t, bool> upload_pending_map;
};

pulse_graphic_state* state_from_app(pulse_app_t app);
CGPUDeviceId get_device(pulse_app_t app);
bool is_upload_pending(pulse_app_t app, pulse_asset_handle handle);
void mark_upload_pending(pulse_app_t app, pulse_asset_handle handle);
void clear_upload_pending(pulse_app_t app, pulse_asset_handle handle);
void material_internal_destroy(void* data);
void install_upload_callback(pulse_app_t app);

} // namespace pulse_graphic_internal
