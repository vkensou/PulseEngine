#include "graphic_internal.h"
#include "renderer.h"

extern "C" {

pulse_texture_t pulse_graphic_texture_create_from_data(
    pulse_app_t app,
    const CGPUTextureDescriptor* desc,
    const void* pixel_data, uint64_t pixel_data_size)
{
    pulse_texture_t result{};
    CGPUDeviceId device = pulse_graphic_internal::get_device(app);
    if (!device || !desc) return result;

    auto cpp_texture = HGEGraphics::create_texture(device, *desc);
    if (!cpp_texture) return result;

    pulse_asset_handle asset_handle = pulse_asset_load_from_memory(
        app, PULSE_TYPE_TEXTURE, "", nullptr, 0);
    if (asset_handle.index == PULSE_ASSET_INVALID_INDEX) return result;

    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, asset_handle, &ref)) {
        pulse_texture_data_t* tex = static_cast<pulse_texture_data_t*>(ref.ptr);
        tex->handle = cpp_texture->handle;
        tex->view = cpp_texture->view;
        tex->width = desc->width;
        tex->height = desc->height;
        tex->depth = desc->depth;
        tex->mip_levels = desc->mip_levels;
        tex->format = desc->format;
        cpp_texture->handle = CGPU_NULLPTR;
        cpp_texture->view = CGPU_NULLPTR;
        pulse_asset_release(app, &ref);
    }

    if (pixel_data && pixel_data_size > 0) {
        pulse_graphic_internal::mark_upload_pending(app, asset_handle);
    }

    result.asset = asset_handle;
    return result;
}

pulse_texture_data_t* pulse_graphic_texture_acquire(pulse_app_t app, pulse_texture_t* handle) {
    pulse_asset_ref ref{};
    if (pulse_asset_acquire(app, handle->asset, &ref)) {
        return static_cast<pulse_texture_data_t*>(ref.ptr);
    }
    return nullptr;
}

void pulse_graphic_texture_release(pulse_app_t app, pulse_texture_t* handle) {
    pulse_asset_ref ref{handle->asset, nullptr};
    pulse_asset_release(app, &ref);
}

} // extern "C"
