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

    auto shader_request = pulse_asset_system_to_asset_request_from_dep_ref(
        ctx->asset_system, ctx->dependencies[0].dep_ref);
    if (!s->shader_done) {
        if (!pulse_asset_system_is_ready(ctx->asset_system, shader_request)) {
            return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
        } else {
            s->shader_done = true;
        }
    }

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* mat = static_cast<PulseMaterialData*>(ctx->out_asset);

        PulseShaderHandle shader_handle = pulse_shader_get_handle(ctx->app, { shader_request.index, shader_request.generation });
        PulseShaderData* shader_data = internal_borrow_shader(ctx->asset_system, shader_handle);
        if (!shader_data) {
            *out_error = "material loader: shader not available";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        PulseShader shader_ref = { shader_handle, shader_data };
        HGEGraphics::init_material(mat, device, shader_ref);

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_STATUS_DONE;
}

void register_material_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
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
    ld.settings_size = sizeof(PulseMaterialCreateDesc);
    ld.settings_align = alignof(PulseMaterialCreateDesc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

}

extern "C" {

PulseMaterialHandle pulse_create_material(
    PulseAppId app,
    const PulseMaterialCreateDesc* desc)
{
    PulseMaterialHandle result{};
    if (!desc)
        return result;

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return result;

    PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
    PulseAssetDependency dependencies[1];
    dependencies[0] = { pulse_asset_system_to_asset_dep_ref_from_handle(as, pulse_shader_to_handle(desc->shader)), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED };

    PulseAssetHandle handle = pulse_graphics_internal::asset_build_sync(
        as, PULSE_TYPE_MATERIAL, nullptr, nullptr, dependencies, 1, desc);
    if (!pulse_asset_handle_is_valid(handle))
        return result;

    result.index = handle.index;
    result.generation = handle.generation;
    return result;
}

}
