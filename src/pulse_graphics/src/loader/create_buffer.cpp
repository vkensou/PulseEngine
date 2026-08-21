#include "../graphics_internal.h"

#include "renderer.h"
#include <cstring>

namespace pulse_graphics_internal {

struct BufferLoaderState {
    bool upload_requested = false;
    bool upload_completed = false;
};

// Settings deep-copy: the asset system allocates one block of the returned size and
// copies the struct bytes to its head; this callback lays out the nested data
// (name string + buffer data) right after the struct and fixes the pointers into the block.
static uint64_t buffer_settings_size_fn(const void* settings, void* user_data) {
    const auto* s = static_cast<const PulseGraphicsBufferCreateDesc*>(settings);
    uint64_t total = sizeof(PulseGraphicsBufferCreateDesc);
    if (s->desc.name) {
        total += strlen(s->desc.name) + 1;
    }
    if (s->data && s->data_size > 0) {
        total += s->data_size;
    }
    return total;
}

static bool buffer_settings_copy_fn(void* dst, const void* src, uint64_t byte_size, void* user_data) {
    auto* d = static_cast<PulseGraphicsBufferCreateDesc*>(dst);
    const auto* s = static_cast<const PulseGraphicsBufferCreateDesc*>(src);
    uint8_t* cursor = reinterpret_cast<uint8_t*>(dst) + sizeof(PulseGraphicsBufferCreateDesc);
    const uint8_t* end = reinterpret_cast<const uint8_t*>(dst) + byte_size;

    if (s->desc.name) {
        size_t len = strlen(s->desc.name) + 1;
        if (cursor + len > end) {
            return false;
        }
        memcpy(cursor, s->desc.name, len);
        d->desc.name = reinterpret_cast<const char*>(cursor);
        cursor += len;
    } else {
        d->desc.name = nullptr;
    }
    if (s->data && s->data_size > 0) {
        if (cursor + s->data_size > end) {
            return false;
        }
        memcpy(cursor, s->data, s->data_size);
        d->data = cursor;
        cursor += s->data_size;
    } else {
        d->data = nullptr;
    }
    return true;
}

EPulseAssetLoaderStatus step_buffer_create(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<BufferLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* buf = static_cast<PulseGraphicsBufferData*>(ctx->out_asset);

        auto create_desc = static_cast<const PulseGraphicsBufferCreateDesc*>(ctx->settings);

        buf->handle = cgpu_device_create_buffer(device, &create_desc->desc);
        if (!buf->handle) {
            *out_error = "buffer loader: create buffer failed";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        buf->type = (ECGPUResourceTypeFlags)create_desc->desc.descriptors;
        buf->cur_state = CGPU_RESOURCE_STATE_UNDEFINED;
        buf->dynamic_handle = {};

        if (create_desc->data && create_desc->data_size > 0) {
            auto* gstate = state_from_app(ctx->app);
            if (gstate) {
                PulseGraphicsBufferHandle handle = { ctx->request.index, ctx->request.generation };
                auto* staging = queue_staging_buffer_full(gstate, handle, buf, create_desc->data_size, &s->upload_completed);
                memcpy(staging, create_desc->data, create_desc->data_size);
            }
            else {
                return PULSE_ASSET_LOADER_STATUS_FAILED;
            }

            s->upload_requested = true;
            return PULSE_ASSET_LOADER_STATUS_PENDING;
        } else {
            return PULSE_ASSET_LOADER_STATUS_DONE;
        }
    }

    if (s->upload_completed) {
        return PULSE_ASSET_LOADER_STATUS_DONE;
    }

    return PULSE_ASSET_LOADER_STATUS_PENDING;
}

void register_buffer_create_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_GRAPHICS_BUFFER;
    ld.extensions = "";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_buffer_create;
    ld.loader_size = sizeof(BufferLoaderState);
    ld.loader_align = alignof(BufferLoaderState);
    ld.settings_size = sizeof(PulseGraphicsBufferCreateDesc);
    ld.settings_align = alignof(PulseGraphicsBufferCreateDesc);
    ld.settings_size_fn = buffer_settings_size_fn;
    ld.settings_copy_fn = buffer_settings_copy_fn;
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld);
}

}

extern "C" {

PulseGraphicsBufferRequest pulse_create_graphics_buffer(
    PulseAppId app,
    const PulseGraphicsBufferCreateDesc* desc)
{
    if (!desc)
        return {};

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device)
        return {};

    PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
    PulseAssetRequest request = pulse_graphics_internal::asset_build(
        as, PULSE_TYPE_GRAPHICS_BUFFER, nullptr, nullptr, 0, desc);
    if (!pulse_asset_request_is_valid(request))
        return {};

    return { request.index, request.generation };
}

}
