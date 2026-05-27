#include "graphic_internal.h"
#include "renderer.h"

extern "C" {

pulse_buffer_t pulse_graphic_buffer_create(
    pulse_app_t app,
    const CGPUBufferDescriptor* desc,
    const void* data, uint64_t data_size)
{
    pulse_buffer_t result{};
    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device || !desc) return result;

    auto cpp_buffer = HGEGraphics::create_buffer(device, *desc);
    if (!cpp_buffer) return result;

    pulse_asset_handle asset_handle = pulse_graphic_internal::asset_load_memory_path(
        app, PULSE_TYPE_BUFFER, "", nullptr, 0);
    if (!pulse_asset_handle_is_valid(asset_handle)) return result;

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_buffer_data_t* buf = static_cast<pulse_buffer_data_t*>(ref.ptr);
        buf->handle = cpp_buffer->handle;
        buf->type = desc->descriptors;
        cpp_buffer->handle = CGPU_NULLPTR;
        pulse_asset_release(app, &ref);
    }

    if (data && data_size > 0) {
        auto* gstate = pulse_graphic_internal::state_from_app(app);
        if (gstate) {
            pulse_asset_ref ref2{};
            if (pulse_asset_acquire(app, asset_handle, &ref2)) {
                auto* buf = static_cast<pulse_buffer_data_t*>(ref2.ptr);
                auto* staging = pulse_graphic_internal::queue_staging_buffer_full(gstate, buf, data_size, nullptr);
                memcpy(staging, data, data_size);
                pulse_asset_release(app, &ref2);
            }
        }
    }

    result.asset = asset_handle;
    return result;
}

pulse_buffer_data_t* pulse_graphic_buffer_acquire(pulse_app_t app, pulse_buffer_t* handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, handle->asset, &ref)) {
        return static_cast<pulse_buffer_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphic_buffer_release(pulse_app_t app, pulse_buffer_t* handle) {
    pulse_asset_ref ref{handle->asset, nullptr};
    pulse_asset_release(app, &ref);
}

} // extern "C"
