#include "../graphics_internal.h"

#include "renderer.h"

namespace pulse_graphics_internal {

struct MaterialLoaderState {
    bool initialized = false;
    bool shader_done = false;
};

pulse_asset_loader_status_t step_material_create(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<MaterialLoaderState*>(state);

    auto shader_asset_handle = ctx->dependencies[0].handle;
    if (!s->shader_done) {
        if (!pulse_asset_is_ready(ctx->app, shader_asset_handle)) {
            return PULSE_ASSET_LOADER_WAIT_DEPENDENCIES;
        } else {
            s->shader_done = true;
        }
    }

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* mat = static_cast<pulse_material_data_t*>(ctx->out_asset);

        pulse_shader_t shader_handle = { shader_asset_handle.index, shader_asset_handle.generation };
        pulse_graphics_shader_ref shader_ref{};
        if (!pulse_graphics_shader_acquire(ctx->app, shader_handle, &shader_ref)) {
            *out_error = "material loader: shader not available";
            return PULSE_ASSET_LOADER_FAILED;
        }

        HGEGraphics::init_material(mat, device, shader_ref.ptr);
        pulse_graphics_shader_release(ctx->app, &shader_ref);

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_DONE;
}

void register_material_create_loader(PulseAppId app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld{};
    ld.struct_size = sizeof(pulse_asset_loader_desc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_MATERIAL;
    ld.extensions = "";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_material_create;
    ld.loader_size = sizeof(MaterialLoaderState);
    ld.loader_align = alignof(MaterialLoaderState);
    ld.settings_size = sizeof(pulse_graphics_material_create_desc);
    ld.settings_align = alignof(pulse_graphics_material_create_desc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld);
}

}

extern "C" {

pulse_material_t pulse_graphics_material_create(
    PulseAppId app,
    const pulse_graphics_material_create_desc* desc)
{
    pulse_material_t result{};
    if (!desc)
        return result;

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return result;

    pulse_asset_dependency dependencies[1];
    dependencies[0] = { pulse_graphics_shader_to_handle(desc->shader), PULSE_DEP_REQUIRED };

    pulse_asset_handle asset_handle = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_MATERIAL, nullptr, dependencies, 1, desc);
    if (!pulse_asset_handle_is_valid(asset_handle))
        return result;

    result.index = asset_handle.index;
    result.generation = asset_handle.generation;
    return result;
}

}
