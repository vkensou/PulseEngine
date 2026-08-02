#include "../graphics_internal.h"
#include "renderer.h"
#include <vector>
#include <unordered_map>

namespace pulse_graphics_internal {

static void destroy_material(void* ptr, void* user_data) {
    (void)user_data;
    PulseMaterialData* data = static_cast<PulseMaterialData*>(ptr);
    HGEGraphics::free_material(data);
}

void register_material_type(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_MATERIAL;
    type_desc.size = sizeof(PulseMaterialData);
    type_desc.align = alignof(PulseMaterialData);
    type_desc.destroy = destroy_material;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(pulse_get_asset_system(app), &type_desc);
}

const PulseShaderProperty* get_material_shader_property(PulseMaterialData* _this, const char* name, EPulseShaderPropertyType type)
{
    if (!_this || !name) return nullptr;
    if (!_this->shader.ptr) return nullptr;

    const auto* prop = pulse_find_shader_property(_this->shader.ptr, name);
    if (!prop || prop->role != PULSE_SHADER_PROPERTY_ROLE_MATERIAL || (EPulseShaderPropertyType)prop->type != type) return nullptr;
    return prop;
}

std::tuple<const PulseShaderProperty*, pulse_material_ubo_column_t*> get_material_ubo_column(PulseMaterialData* _this, const char* name, EPulseShaderPropertyType type)
{
    auto prop = get_material_shader_property(_this, name, type);
    if (!prop || (EPulseShaderPropertyType)prop->type != type) return { nullptr, nullptr };

    return { prop, HGEGraphics::material_find_ubo_column(_this, prop->set, prop->binding) };
}

void pulse_material_set_float4(PulseMaterialData* _this, const char* name, HMM_Vec4 value)
{
    auto[prop, col] = pulse_graphics_internal::get_material_ubo_column(_this, name, PULSE_SHADER_PROPERTY_TYPE_FLOAT4);
    if (!prop || !col || !col->cpu_data) return;

    static_assert(sizeof(HMM_Vec4) == 16);
    memcpy(col->cpu_data + prop->offset, &value, sizeof(HMM_Vec4));
    col->dirty = true;
}

void pulse_material_set_mat4(PulseMaterialData* _this, const char* name, HMM_Mat4 value)
{
    auto[prop, col] = pulse_graphics_internal::get_material_ubo_column(_this, name, PULSE_SHADER_PROPERTY_TYPE_MAT4);
    if (!prop || !col || !col->cpu_data) return;

    static_assert(sizeof(HMM_Mat4) == 64);
    memcpy(col->cpu_data + prop->offset, &value, sizeof(HMM_Mat4));
    col->dirty = true;
}

void pulse_material_set_texture(PulseMaterialData* _this, const char* name, PulseTexture texture)
{
    auto prop = pulse_graphics_internal::get_material_shader_property(_this, name, PULSE_SHADER_PROPERTY_TYPE_TEXTURE);
    if (!prop) return;
    HGEGraphics::material_bindTexture(_this, (int)prop->set, (int)prop->binding, texture.ptr);
    HGEGraphics::material_mark_dset_binding_dirty(_this, prop->set);
}

void pulse_material_set_sampler(PulseMaterialData* _this, const char* name, PulseSampler sampler)
{
    auto prop = pulse_graphics_internal::get_material_shader_property(_this, name, PULSE_SHADER_PROPERTY_TYPE_SAMPLER);
    if (!prop) return;
    HGEGraphics::material_bindSampler(_this, (int)prop->set, (int)prop->binding, sampler.ptr);
    HGEGraphics::material_mark_dset_binding_dirty(_this, prop->set);
}

} // namespace pulse_graphics_internal

extern "C" {

PulseShader pulse_material_get_shader(PulseMaterial self)
{
    return self.ptr->shader;
}

void pulse_material_set_property_float4(PulseMaterial self, const char* name, HMM_Vec4 value)
{
    pulse_graphics_internal::pulse_material_set_float4(self.ptr, name, value);
}

void pulse_material_set_property_mat4(PulseMaterial self, const char* name, HMM_Mat4 value)
{
    pulse_graphics_internal::pulse_material_set_mat4(self.ptr, name, value);
}

void pulse_material_set_property_texture(PulseMaterial self, const char* name, PulseTexture texture)
{
    pulse_graphics_internal::pulse_material_set_texture(self.ptr, name, texture);
}

void pulse_material_set_property_sampler(PulseMaterial self, const char* name, PulseSampler sampler)
{
    pulse_graphics_internal::pulse_material_set_sampler(self.ptr, name, sampler);
}

bool pulse_acquire_material(PulseAppId app, PulseMaterialHandle handle, PulseMaterial* material_ref) {
    PulseAssetRef ref{};
    if (pulse_asset_system_acquire(pulse_get_asset_system(app), pulse_material_to_handle(handle), &ref)) {
        material_ref->handle = handle;
        material_ref->ptr = static_cast<PulseMaterialData*>(ref.ptr);
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
