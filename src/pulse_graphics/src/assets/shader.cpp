#include "../graphics_internal.h"

namespace pulse_graphics_internal {

static void destroy_shader(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_shader_data_t* data = static_cast<pulse_shader_data_t*>(ptr);
    if (data->root_sig) cgpu_device_free_root_signature(device, data->root_sig);
	if (data->p_blend_attachment_states) delete[] data->p_blend_attachment_states;
}

void register_shader_type(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_SHADER;
    type_desc.size = sizeof(pulse_shader_data_t);
    type_desc.align = alignof(pulse_shader_data_t);
    type_desc.destroy = destroy_shader;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(pulse_get_asset_system(app), &type_desc);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

bool pulse_acquire_shader(PulseAppId app, PulseShaderHandle handle, PulseShader* ref) {
    PulseAssetRef aref{};
    if (pulse_asset_system_acquire(pulse_get_asset_system(app), pulse_shader_to_handle(handle), &aref)) {
        ref->handle = handle;
        ref->ptr = static_cast<pulse_shader_data_t*>(aref.ptr);
        return true;
    }
    ref->handle = {};
    ref->ptr = nullptr;
    return false;
}

void pulse_release_shader(PulseAppId app, PulseShader* ref) {
    PulseAssetRef aref{ pulse_shader_to_handle(ref->handle), nullptr };
    pulse_asset_system_release(pulse_get_asset_system(app), &aref);
    ref->handle = {};
    ref->ptr = nullptr;
}

} // extern "C"
