#include "../graphics_internal.h"

#include "renderer.h"

namespace pulse_graphics_internal {

struct BufferLoaderState {
    bool upload_requested = false;
    bool upload_completed = false;
};

pulse_asset_loader_status_t step_buffer_create(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<BufferLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* buf = static_cast<pulse_buffer_data_t*>(ctx->out_asset);

        auto create_desc = static_cast<const pulse_graphics_buffer_create_desc*>(ctx->settings);

        buf->handle = cgpu_device_create_buffer(device, &create_desc->desc);
        if (!buf->handle) {
            *out_error = "buffer loader: create buffer failed";
            return PULSE_ASSET_LOADER_FAILED;
        }
        buf->type = (ECGPUResourceTypeFlags)create_desc->desc.descriptors;

        if (create_desc->data && create_desc->data_size > 0) {
            auto* gstate = state_from_app(ctx->app);
            if (gstate) {
                auto* staging = queue_staging_buffer_full(gstate, buf, create_desc->data_size, &s->upload_completed);
                memcpy(staging, create_desc->data, create_desc->data_size);
            }
            else {
                return PULSE_ASSET_LOADER_FAILED;
            }

            s->upload_requested = true;
            return PULSE_ASSET_LOADER_PENDING;
        } else {
            return PULSE_ASSET_LOADER_DONE;
        }
    }

    if (s->upload_completed) {
        return PULSE_ASSET_LOADER_DONE;
    }

    return PULSE_ASSET_LOADER_PENDING;
}

void register_buffer_create_loader(pulse_app_t app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld{};
    ld.struct_size = sizeof(pulse_asset_loader_desc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_BUFFER;
    ld.extensions = "";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_buffer_create;
    ld.loader_size = sizeof(BufferLoaderState);
    ld.loader_align = alignof(BufferLoaderState);
    ld.settings_size = sizeof(pulse_graphics_buffer_create_desc);
    ld.settings_align = alignof(pulse_graphics_buffer_create_desc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld);
}

}

extern "C" {

pulse_buffer_t pulse_graphics_buffer_create(
    pulse_app_t app,
    const pulse_graphics_buffer_create_desc* desc)
{
    if (!desc)
        return {};

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return {};

    pulse_asset_handle asset_handle = pulse_graphics_internal::asset_build(
        app, PULSE_TYPE_BUFFER, nullptr, nullptr, 0, desc);
    if (!pulse_asset_handle_is_valid(asset_handle))
        return {};

    return { asset_handle.index, asset_handle.generation };
}

}
