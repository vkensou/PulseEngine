#include "../graphics_internal.h"
#include "renderer.h"

namespace pulse_graphics_internal {

static void destroy_texture(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_texture_data_t* data = static_cast<pulse_texture_data_t*>(ptr);
    if (data->view) cgpu_device_free_texture_view(device, data->view);
    if (data->handle) cgpu_device_free_texture(device, data->handle);
}

void register_texture_type(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_TEXTURE;
    type_desc.size = sizeof(pulse_texture_data_t);
    type_desc.align = alignof(pulse_texture_data_t);
    type_desc.destroy = destroy_texture;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(pulse_get_asset_system(app), &type_desc);
}

}

extern "C" {

bool pulse_acquire_texture(PulseAppId app, PulseTextureHandle handle, PulseTexture* texture_ref) {
    PulseAssetRef ref{};
    if (pulse_asset_system_acquire(pulse_get_asset_system(app), pulse_texture_to_handle(handle), &ref)) {
        texture_ref->handle = handle;
        texture_ref->ptr = static_cast<pulse_texture_data_t*>(ref.ptr);
        return true;
    }

    texture_ref->handle = {};
    texture_ref->ptr = nullptr;
    return false;
}

void pulse_release_texture(PulseAppId app, PulseTexture* texture_ref) {
    PulseAssetRef ref{pulse_texture_to_handle(texture_ref->handle), nullptr};
    pulse_asset_system_release(pulse_get_asset_system(app), &ref);
    texture_ref->handle = {};
    texture_ref->ptr = nullptr;
}

} // extern "C"
