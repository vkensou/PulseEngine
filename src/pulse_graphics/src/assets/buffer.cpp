#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

static void destroy_buffer(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_buffer_data_t* data = static_cast<pulse_buffer_data_t*>(ptr);
    if (data->handle) cgpu_device_free_buffer(device, data->handle);
}

void register_buffer_type(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_GRAPHICS_BUFFER;
    type_desc.size = sizeof(pulse_buffer_data_t);
    type_desc.align = alignof(pulse_buffer_data_t);
    type_desc.destroy = destroy_buffer;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(pulse_get_asset_system(app), &type_desc);
}

}

extern "C" {

bool pulse_acquire_graphics_buffer(PulseAppId app, PulseGraphicsBufferHandle handle, PulseGraphicsBuffer* buffer_ref) {
    PulseAssetRef ref{};
    if (pulse_asset_system_acquire(pulse_get_asset_system(app), pulse_graphics_buffer_to_handle(handle), &ref)) {
        buffer_ref->handle = handle;
        buffer_ref->ptr = static_cast<pulse_buffer_data_t*>(ref.ptr);
        return true;
    }

    buffer_ref->handle = {};
    buffer_ref->ptr = nullptr;
    return false;
}

void pulse_release_graphics_buffer(PulseAppId app, PulseGraphicsBuffer* buffer_ref) {
    PulseAssetRef ref{ pulse_graphics_buffer_to_handle(buffer_ref->handle), nullptr };
    pulse_asset_system_release(pulse_get_asset_system(app), &ref);
    buffer_ref->handle = {};
    buffer_ref->ptr = nullptr;
}

} // extern "C"
