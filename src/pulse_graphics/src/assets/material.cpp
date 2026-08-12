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

void register_material_type(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_MATERIAL;
    type_desc.size = sizeof(PulseMaterialData);
    type_desc.align = alignof(PulseMaterialData);
    type_desc.destroy = destroy_material;
    type_desc.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_type(asset_system, &type_desc);
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

void pulse_material_set_texture(PulseMaterialData* _this, const char* name, PulseTextureData* texture)
{
    auto prop = pulse_graphics_internal::get_material_shader_property(_this, name, PULSE_SHADER_PROPERTY_TYPE_TEXTURE);
    if (!prop || !texture) return;
    HGEGraphics::material_bindTexture(_this, (int)prop->set, (int)prop->binding, texture);
    HGEGraphics::material_mark_dset_binding_dirty(_this, prop->set);
}

void pulse_material_set_sampler(PulseMaterialData* _this, const char* name, PulseSamplerData* sampler)
{
    auto prop = pulse_graphics_internal::get_material_shader_property(_this, name, PULSE_SHADER_PROPERTY_TYPE_SAMPLER);
    if (!prop || !sampler) return;
    HGEGraphics::material_bindSampler(_this, (int)prop->set, (int)prop->binding, sampler);
    HGEGraphics::material_mark_dset_binding_dirty(_this, prop->set);
}

const uint8_t* pulse_material_get_ubo_column(PulseMaterialData* _this, uint32_t index)
{
    if (!_this) return nullptr;
    if (index >= _this->uboColumns.size) return nullptr;
    return _this->uboColumns.data[index].cpu_data;
}

} // namespace pulse_graphics_internal

extern "C" {

PulseMaterialHandle pulse_material_get_handle(PulseAppId app, PulseMaterialRequest request) {
    PulseAssetHandle h = pulse_asset_system_get_handle(
        pulse_graphics_internal::asset_system_from_app(app), pulse_material_request_to_asset_request(request));
    return !pulse_asset_handle_is_valid(h) ? PulseMaterialHandle{} : PulseMaterialHandle{h.index, h.generation};
}

bool pulse_material_is_ready(PulseAppId app, PulseMaterialRequest request) {
    return pulse_asset_system_is_ready(
        pulse_graphics_internal::asset_system_from_app(app), pulse_material_request_to_asset_request(request));
}

bool pulse_material_is_alive(PulseAppId app, PulseMaterialRequest request) {
    return pulse_asset_system_is_alive(
        pulse_graphics_internal::asset_system_from_app(app), pulse_material_request_to_asset_request(request));
}

PulseShaderHandle pulse_material_get_shader(PulseAppId app, PulseMaterialHandle self)
{
    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(pulse_graphics_internal::asset_system_from_app(app), self);
    return mat ? mat->shader.handle : PulseShaderHandle{};
}

void pulse_material_set_property_float4(PulseAppId app, PulseMaterialHandle self, const char* name, HMM_Vec4 value)
{
    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(pulse_graphics_internal::asset_system_from_app(app), self);
    if (!mat) return;
    pulse_graphics_internal::pulse_material_set_float4(mat, name, value);
}

void pulse_material_set_property_mat4(PulseAppId app, PulseMaterialHandle self, const char* name, HMM_Mat4 value)
{
    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(pulse_graphics_internal::asset_system_from_app(app), self);
    if (!mat) return;
    pulse_graphics_internal::pulse_material_set_mat4(mat, name, value);
}

void pulse_material_set_property_texture(PulseAppId app, PulseMaterialHandle self, const char* name, PulseTextureHandle texture)
{
    PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(as, self);
    if (!mat) return;
    PulseTextureData* tex = pulse_graphics_internal::internal_borrow_texture(as, texture);
    if (!tex) return;
    pulse_graphics_internal::pulse_material_set_texture(mat, name, tex);
}

void pulse_material_set_property_sampler(PulseAppId app, PulseMaterialHandle self, const char* name, PulseSamplerHandle sampler)
{
    PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(as, self);
    if (!mat) return;
    PulseSamplerData* smp = pulse_graphics_internal::internal_borrow_sampler(as, sampler);
    if (!smp) return;
    pulse_graphics_internal::pulse_material_set_sampler(mat, name, smp);
}

const uint8_t* pulse_material_get_ubo_column(PulseAppId app, PulseMaterialHandle self, uint32_t index)
{
    PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
    PulseMaterialData* mat = pulse_graphics_internal::internal_borrow_material(as, self);
    if (!mat) return nullptr;
    return pulse_graphics_internal::pulse_material_get_ubo_column(mat, index);
}

} // extern "C"
