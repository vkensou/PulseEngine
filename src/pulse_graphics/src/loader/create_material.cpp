#include "../graphics_internal.h"

#include "renderer.h"

namespace pulse_graphics_internal {

struct MaterialLoaderState {
    bool initialized = false;
    bool shader_done = false;
};

EPulseAssetLoaderStatus step_material_create(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<MaterialLoaderState*>(state);

    auto shader_asset_handle = ctx->dependencies[0].handle;
    if (!s->shader_done) {
        if (!pulse_asset_system_is_ready(ctx->asset_system, shader_asset_handle)) {
            return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
        } else {
            s->shader_done = true;
        }
    }

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* mat = static_cast<pulse_material_data_t*>(ctx->out_asset);

        PulseShaderHandle shader_handle = { shader_asset_handle.index, shader_asset_handle.generation };
        PulseShader shader_ref{};
        if (!internal_acquire_shader(ctx->asset_system, shader_handle, &shader_ref)) {
            *out_error = "material loader: shader not available";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        HGEGraphics::init_material(mat, device, static_cast<pulse_shader_data_t*>(shader_ref.ptr));
        internal_release_shader(ctx->asset_system, &shader_ref);

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_material_create_loader(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_MATERIAL;
    ld.extensions = "";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_material_create;
    ld.loader_size = sizeof(MaterialLoaderState);
    ld.loader_align = alignof(MaterialLoaderState);
    ld.settings_size = sizeof(PulseGraphicsMaterialCreateDesc);
    ld.settings_align = alignof(PulseGraphicsMaterialCreateDesc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld);
}

}

extern "C" {

PulseMaterialHandle pulse_graphics_material_create(
    PulseAppId app,
    const PulseGraphicsMaterialCreateDesc* desc)
{
    PulseMaterialHandle result{};
    if (!desc)
        return result;

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return result;

    PulseAssetDependency dependencies[1];
    dependencies[0] = { pulse_graphics_shader_to_handle(desc->shader), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED };

    PulseAssetHandle asset_handle = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_MATERIAL, nullptr, dependencies, 1, desc);
    if (!pulse_asset_handle_is_valid(asset_handle))
        return result;

    result.index = asset_handle.index;
    result.generation = asset_handle.generation;
    return result;
}

}
