#include "graphic_internal.h"

#include "renderer.h"

namespace pulse_graphic_internal {

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
        if (!pulse_asset_is_available(ctx->app, shader_asset_handle)) {
            return PULSE_ASSET_LOADER_WAIT_DEPENDENCIES;
        } else {
            s->shader_done = true;
        }
    }

    if (!s->initialized) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* mat = static_cast<pulse_material_data_t*>(ctx->out_asset);

        pulse_shader_t shader_handle = { shader_asset_handle };
        pulse_shader_data_t* shader_data = pulse_graphic_shader_acquire(ctx->app, &shader_handle);
        if (!shader_data) {
            *out_error = "material loader: shader not available";
            return PULSE_ASSET_LOADER_FAILED;
        }

        HGEGraphics::init_material(mat, device, shader_data);
        pulse_graphic_shader_release(ctx->app, &shader_handle);

        s->initialized = true;
    }

    return PULSE_ASSET_LOADER_DONE;
}

void register_material_create_loader(pulse_app_t app, CGPUDeviceId device)
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

pulse_material_t pulse_graphic_material_create(
    pulse_app_t app,
    const pulse_graphics_material_create_desc* desc)
{
    pulse_material_t result{};
    if (!desc)
        return result;

    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device)
        return result;

    pulse_asset_dependency dependencies[1];
    dependencies[0] = { desc->shader.asset, PULSE_DEP_REQUIRED };

    pulse_asset_handle asset_handle = pulse_graphic_internal::asset_build(
        app, PULSE_TYPE_MATERIAL, nullptr, dependencies, 1, desc);
    if (!pulse_asset_handle_is_valid(asset_handle))
        return result;

    result.index = asset_handle.index;
    result.generation = asset_handle.generation;
    return result;
}

}
