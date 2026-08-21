#include "../graphics_internal.h"

namespace pulse_graphics_internal {

static void destroy_shader(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    PulseShaderData* data = static_cast<PulseShaderData*>(ptr);
    data->vs.library = CGPU_NULLPTR;
    data->ps.library = CGPU_NULLPTR;
    HGEGraphics::free_shader(data);
}

void register_shader_type(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_SHADER;
    type_desc.size = sizeof(PulseShaderData);
    type_desc.align = alignof(PulseShaderData);
    type_desc.destroy = destroy_shader;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(asset_system, &type_desc);
}

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseShaderHandle pulse_shader_get_handle(PulseAppId app, PulseShaderRequest request) {
    PulseAssetHandle h = pulse_asset_system_get_handle(
        pulse_graphics_internal::asset_system_from_app(app), pulse_shader_request_to_asset_request(request));
    return !pulse_asset_handle_is_valid(h) ? PulseShaderHandle{} : PulseShaderHandle{h.index, h.generation};
}

bool pulse_shader_is_ready(PulseAppId app, PulseShaderRequest request) {
    return pulse_asset_system_is_ready(
        pulse_graphics_internal::asset_system_from_app(app), pulse_shader_request_to_asset_request(request));
}

bool pulse_shader_is_alive(PulseAppId app, PulseShaderRequest request) {
    return pulse_asset_system_is_alive(
        pulse_graphics_internal::asset_system_from_app(app), pulse_shader_request_to_asset_request(request));
}

uint32_t pulse_shader_get_shader_property_count(PulseAppId app, PulseShaderHandle self) {
    PulseShaderData* shader = pulse_graphics_internal::internal_borrow_shader(pulse_graphics_internal::asset_system_from_app(app), self);
    return shader ? shader->property_count : 0;
}

PulseShaderProperty pulse_shader_get_shader_property(PulseAppId app, PulseShaderHandle self, uint32_t index) {
    PulseShaderData* shader = pulse_graphics_internal::internal_borrow_shader(pulse_graphics_internal::asset_system_from_app(app), self);
    return shader ? shader->p_properties[index] : PulseShaderProperty{};
}

uint32_t pulse_shader_get_ubo_info_count(PulseAppId app, PulseShaderHandle self) {
    PulseShaderData* shader = pulse_graphics_internal::internal_borrow_shader(pulse_graphics_internal::asset_system_from_app(app), self);
    return shader ? shader->ubo_info_count : 0;
}

PulseUboInfo pulse_shader_get_ubo_info(PulseAppId app, PulseShaderHandle self, uint32_t index) {
    PulseShaderData* shader = pulse_graphics_internal::internal_borrow_shader(pulse_graphics_internal::asset_system_from_app(app), self);
    return shader ? shader->p_ubo_infos[index] : PulseUboInfo{};
}

} // extern "C"
