#include "graphic_internal.h"
#include "renderer.h"
#include <vector>
#include <unordered_map>

namespace pulse_graphic_internal {

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

} // namespace pulse_graphic_internal

extern "C" {

void pulse_graphic_material_bind_buffer(
    pulse_app_t app, pulse_material_data_t* material,
    uint32_t set, uint32_t binding,
    pulse_buffer_t buffer)
{
    pulse_graphic_buffer_ref ref{};
    if (!pulse_graphic_buffer_acquire(app, buffer, &ref)) return;

    HGEGraphics::material_bindBuffer(material, set, binding, ref.ptr);

    pulse_graphic_buffer_release(app, &ref);
}

void pulse_graphic_material_bind_texture(
    pulse_app_t app, pulse_material_data_t* material,
    uint32_t set, uint32_t binding,
    pulse_texture_t texture)
{
    pulse_graphic_texture_ref ref{};
    if (!pulse_graphic_texture_acquire(app, texture, &ref)) return;


    HGEGraphics::material_bindTexture(material, set, binding, ref.ptr);

    pulse_graphic_texture_release(app, &ref);
}

void pulse_graphic_material_bind_sampler(
    pulse_app_t app, pulse_material_data_t* material,
    uint32_t set, uint32_t binding,
    pulse_sampler_t sampler)
{
    pulse_sampler_data_t* smp_data = pulse_graphic_sampler_acquire(app, &sampler);
    if (!smp_data) return;

    HGEGraphics::material_bindSampler(material, set, binding, smp_data->handle);

    pulse_graphic_sampler_release(app, &sampler);
}

bool pulse_graphic_material_acquire(pulse_app_t app, pulse_material_t handle, pulse_graphic_material_ref* material_ref) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, pulse_graphic_material_to_handle(handle), &ref)) {
        material_ref->handle = handle;
        material_ref->ptr = static_cast<pulse_material_data_t*>(ref.ptr);
        return true;
    }

    material_ref->handle = {};
    material_ref->ptr = nullptr;
    return false;
}

void pulse_graphic_material_release(pulse_app_t app, pulse_graphic_material_ref* material_ref) {
    pulse_asset_ref ref{ pulse_graphic_material_to_handle(material_ref->handle), nullptr };
    pulse_asset_release(app, &ref);
    material_ref->handle = {};
    material_ref->ptr = nullptr;
}

} // extern "C"
