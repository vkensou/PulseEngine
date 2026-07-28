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

void register_material_type(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_MATERIAL;
    type_desc.size = sizeof(pulse_material_data_t);
    type_desc.align = alignof(pulse_material_data_t);
    type_desc.destroy = destroy_material;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(pulse_get_asset_system(app), &type_desc);
}

} // namespace pulse_graphics_internal

extern "C" {

void pulse_material_bind_buffer(
    PulseMaterial* material,
    uint32_t set, uint32_t binding,
    PulseGraphicsBuffer buffer)
{
    HGEGraphics::material_bindBuffer(static_cast<pulse_material_data_t*>(material->ptr), set, binding, static_cast<pulse_buffer_data_t*>(buffer.ptr));
}

void pulse_material_bind_texture(
    PulseMaterial* material,
    uint32_t set, uint32_t binding,
    PulseTexture texture)
{
    HGEGraphics::material_bindTexture(static_cast<pulse_material_data_t*>(material->ptr), set, binding, static_cast<pulse_texture_data_t*>(texture.ptr));
}

void pulse_material_bind_sampler(
    PulseMaterial* material,
    uint32_t set, uint32_t binding,
    PulseSampler sampler)
{
    HGEGraphics::material_bindSampler(static_cast<pulse_material_data_t*>(material->ptr), set, binding, static_cast<pulse_sampler_data_t*>(sampler.ptr));
}

void pulse_material_bind_data(PulseMaterial* material, uint32_t set, uint32_t binding, size_t size, const void* data)
{
    HGEGraphics::material_bindBuffer(static_cast<pulse_material_data_t*>(material->ptr), set, binding, size, data);
}

void pulse_material_set_float4(PulseMaterial* _this, const char* name, float x, float y, float z, float w)
{
    if (!_this || !_this->ptr || !name) return;
    auto* mat = static_cast<pulse_material_data_t*>(_this->ptr);
    if (!mat->shader) return;

    const auto* prop = pulse_find_shader_property(mat->shader, name);
    if (!prop) return;

    auto* col = HGEGraphics::material_find_or_create_ubo_column(mat, prop->set, prop->binding);

    uint32_t needed = prop->offset + sizeof(float) * 4;
    if (col->size < needed)
    {
        uint8_t* new_data = (uint8_t*)realloc(col->cpu_data, needed);
        memset(new_data + col->size, 0, needed - col->size);
        col->cpu_data = new_data;
        col->size = needed;
    }

    float* dst = (float*)(col->cpu_data + prop->offset);
    dst[0] = x; dst[1] = y; dst[2] = z; dst[3] = w;
    col->dirty = true;

    // Mark the material descriptor set dirty for the UBO's set
    for (int i = 0; i < mat->materialDsets.size; ++i)
    {
        if (mat->materialDsets.data[i].set_index == prop->set)
        {
            mat->materialDsets.data[i].dirty = true;
            break;
        }
    }
}

void pulse_material_set_texture(PulseMaterial* _this, const char* name, PulseTextureHandle texture)
{
    if (!_this || !_this->ptr || !name) return;
    auto* mat = static_cast<pulse_material_data_t*>(_this->ptr);
    if (!mat->shader) return;

    const auto* prop = pulse_find_shader_property(mat->shader, name);
    if (!prop) return;

    PulseTexture tex_ref{};
    if (pulse_acquire_texture(pulse_graphics_internal::g_loader_app, texture, &tex_ref)) {
        auto* tex_data = static_cast<pulse_texture_data_t*>(tex_ref.ptr);
        HGEGraphics::material_bindTexture(mat, (int)prop->set, (int)prop->binding, tex_data);
        pulse_release_texture(pulse_graphics_internal::g_loader_app, &tex_ref);
    }
}

bool pulse_acquire_material(PulseAppId app, PulseMaterialHandle handle, PulseMaterial* material_ref) {
    PulseAssetRef ref{};
    if (pulse_asset_system_acquire(pulse_get_asset_system(app), pulse_material_to_handle(handle), &ref)) {
        material_ref->handle = handle;
        material_ref->ptr = static_cast<pulse_material_data_t*>(ref.ptr);
        return true;
    }

    material_ref->handle = {};
    material_ref->ptr = nullptr;
    return false;
}

void pulse_release_material(PulseAppId app, PulseMaterial* material_ref) {
    PulseAssetRef ref{ pulse_material_to_handle(material_ref->handle), nullptr };
    pulse_asset_system_release(pulse_get_asset_system(app), &ref);
    material_ref->handle = {};
    material_ref->ptr = nullptr;
}

} // extern "C"
