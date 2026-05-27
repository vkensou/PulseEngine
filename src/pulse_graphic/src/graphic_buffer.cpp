#include "graphic_internal.h"
#include "renderer.h"

namespace pulse_graphic_internal {

static void destroy_buffer(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_buffer_data_t* data = static_cast<pulse_buffer_data_t*>(ptr);
    if (data->handle) cgpu_device_free_buffer(device, data->handle);
}

void register_buffer_type(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_type_desc type_desc{};
    type_desc.struct_size = sizeof(pulse_asset_type_desc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_BUFFER;
    type_desc.size = sizeof(pulse_buffer_data_t);
    type_desc.align = alignof(pulse_buffer_data_t);
    type_desc.destroy = destroy_buffer;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_type(app, &type_desc);
}

}

extern "C" {

pulse_buffer_data_t* pulse_graphic_buffer_acquire(pulse_app_t app, pulse_buffer_t handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, pulse_graphic_buffer_to_handle(handle), &ref)) {
        return static_cast<pulse_buffer_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphic_buffer_release(pulse_app_t app, pulse_buffer_t handle) {
    pulse_asset_ref ref{ pulse_graphic_buffer_to_handle(handle), nullptr };
    pulse_asset_release(app, &ref);
}

} // extern "C"
