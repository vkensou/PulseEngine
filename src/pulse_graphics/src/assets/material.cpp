#include "../graphics_internal.h"
#include "renderer.h"
#include <vector>
#include <unordered_map>

namespace pulse_graphics_internal {

static void destroy_material(void* ptr, void* user_data) {
    (void)user_data;
    pulse_material_data_t* data = static_cast<pulse_material_data_t*>(ptr);
    HGEGraphics::free_material(data);
}

void register_material_type(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_type_desc type_desc{};
    type_desc.struct_size = sizeof(pulse_asset_type_desc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_MATERIAL;
    type_desc.size = sizeof(pulse_material_data_t);
    type_desc.align = alignof(pulse_material_data_t);
    type_desc.destroy = destroy_material;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_type(app, &type_desc);
}

} // namespace pulse_graphics_internal

extern "C" {

void pulse_graphics_material_bind_buffer(
    pulse_app_t app, pulse_material_data_t* material,
    uint32_t set, uint32_t binding,
    pulse_buffer_t buffer)
{
    pulse_graphics_buffer_ref ref{};
    if (!pulse_graphics_buffer_acquire(app, buffer, &ref)) return;

    HGEGraphics::material_bindBuffer(material, set, binding, ref.ptr);

    pulse_graphics_buffer_release(app, &ref);
}

void pulse_graphics_material_bind_texture(
    pulse_app_t app, pulse_material_data_t* material,
    uint32_t set, uint32_t binding,
    pulse_texture_t texture)
{
    pulse_graphics_texture_ref ref{};
    if (!pulse_graphics_texture_acquire(app, texture, &ref)) return;


    HGEGraphics::material_bindTexture(material, set, binding, ref.ptr);

    pulse_graphics_texture_release(app, &ref);
}

void pulse_graphics_material_bind_sampler(
    pulse_app_t app, pulse_material_data_t* material,
    uint32_t set, uint32_t binding,
    pulse_sampler_t sampler)
{
    pulse_graphics_sampler_ref ref{};
    if (!pulse_graphics_sampler_acquire(app, sampler, &ref)) return;

    HGEGraphics::material_bindSampler(material, set, binding, ref.ptr->handle);

    pulse_graphics_sampler_release(app, &ref);
}

bool pulse_graphics_material_acquire(pulse_app_t app, pulse_material_t handle, pulse_graphics_material_ref* material_ref) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, pulse_graphics_material_to_handle(handle), &ref)) {
        material_ref->handle = handle;
        material_ref->ptr = static_cast<pulse_material_data_t*>(ref.ptr);
        return true;
    }

    material_ref->handle = {};
    material_ref->ptr = nullptr;
    return false;
}

void pulse_graphics_material_release(pulse_app_t app, pulse_graphics_material_ref* material_ref) {
    pulse_asset_ref ref{ pulse_graphics_material_to_handle(material_ref->handle), nullptr };
    pulse_asset_release(app, &ref);
    material_ref->handle = {};
    material_ref->ptr = nullptr;
}

} // extern "C"
