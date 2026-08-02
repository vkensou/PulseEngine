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

std::tuple<PulseMaterialData*, const pulse_shader_property_t*> get_material_shader_property(PulseMaterial* _this, const char* name, EPulseShaderPropertyType type)
{
    if (!_this || !_this->ptr || !name) return { nullptr, nullptr };
    auto* mat = _this->ptr;
    if (!mat->shader) return { nullptr, nullptr };

    const auto* prop = pulse_find_shader_property(mat->shader, name);
    if (!prop || prop->role != PULSE_SHADER_PROPERTY_ROLE_MATERIAL || (EPulseShaderPropertyType)prop->type != type) return { nullptr, nullptr };
    return { mat, prop };
}

std::tuple<const pulse_shader_property_t*, pulse_material_ubo_column_t*> get_material_ubo_column(PulseMaterial* _this, const char* name, EPulseShaderPropertyType type)
{
    auto[mat, prop] = get_material_shader_property(_this, name, type);
    if (!prop || (EPulseShaderPropertyType)prop->type != type) return { nullptr, nullptr };

    return { prop, HGEGraphics::material_find_ubo_column(mat, prop->set, prop->binding) };
}

} // namespace pulse_graphics_internal

extern "C" {

void pulse_material_set_float4(PulseMaterial* _this, const char* name, HMM_Vec4 value)
{
    auto[prop, col] = pulse_graphics_internal::get_material_ubo_column(_this, name, PULSE_SHADER_PROPERTY_TYPE_FLOAT4);
    if (!prop || !col || !col->cpu_data) return;

    static_assert(sizeof(HMM_Vec4) == 16);
    memcpy(col->cpu_data + prop->offset, &value, sizeof(HMM_Vec4));
    col->dirty = true;
}

void pulse_material_set_mat4(PulseMaterial* _this, const char* name, HMM_Mat4 value)
{
    auto[prop, col] = pulse_graphics_internal::get_material_ubo_column(_this, name, PULSE_SHADER_PROPERTY_TYPE_MAT4);
    if (!prop || !col || !col->cpu_data) return;

    static_assert(sizeof(HMM_Mat4) == 64);
    memcpy(col->cpu_data + prop->offset, &value, sizeof(HMM_Mat4));
    col->dirty = true;
}

void pulse_material_set_texture(PulseMaterial* _this, const char* name, PulseTextureHandle texture)
{
    auto [mat, prop] = pulse_graphics_internal::get_material_shader_property(_this, name, PULSE_SHADER_PROPERTY_TYPE_TEXTURE);
    if (!prop) return;

    PulseTexture tex_ref{};
    if (pulse_acquire_texture(pulse_graphics_internal::g_loader_app, texture, &tex_ref)) {
        auto* tex_data = tex_ref.ptr;
        HGEGraphics::material_bindTexture(mat, (int)prop->set, (int)prop->binding, tex_data);
        pulse_release_texture(pulse_graphics_internal::g_loader_app, &tex_ref);

        HGEGraphics::material_mark_dset_binding_dirty(mat, prop->set);
    }
}

void pulse_material_set_sampler(PulseMaterial* _this, const char* name, PulseSamplerHandle sampler)
{
    auto [mat, prop] = pulse_graphics_internal::get_material_shader_property(_this, name, PULSE_SHADER_PROPERTY_TYPE_SAMPLER);
    if (!prop) return;

    PulseSampler smp_ref{};
    if (pulse_acquire_sampler(pulse_graphics_internal::g_loader_app, sampler, &smp_ref)) {
        auto* smp_data = smp_ref.ptr;
        HGEGraphics::material_bindSampler(mat, (int)prop->set, (int)prop->binding, smp_data);
        pulse_release_sampler(pulse_graphics_internal::g_loader_app, &smp_ref);

        HGEGraphics::material_mark_dset_binding_dirty(mat, prop->set);
    }
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
