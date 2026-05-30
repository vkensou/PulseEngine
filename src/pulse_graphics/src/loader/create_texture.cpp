#include "../graphics_internal.h"

#include "renderer.h"

namespace pulse_graphics_internal {

struct TextureLoaderState {
    bool upload_requested = false;
    bool upload_completed = false;
};

pulse_asset_loader_status_t step_texture_create(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<TextureLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* texture = static_cast<pulse_texture_data_t*>(ctx->out_asset);

		auto create_desc = static_cast<const pulse_graphics_texture_create_desc*>(ctx->settings);

        HGEGraphics::init_texture(texture, device, create_desc->desc);

		if (create_desc->pixel_data && create_desc->pixel_data_size > 0) {
			auto* gstate = state_from_app(ctx->app);
			if (gstate) {
                uint64_t staging_size = 0;
				auto* staging = queue_staging_texture_full(gstate, texture, 1, create_desc->generate_mipmaps, &staging_size, &s->upload_completed);
				if (staging_size < create_desc->pixel_data_size) {
					*out_error = "texture loader: pixel data size exceeds staging buffer size";
					return PULSE_ASSET_LOADER_FAILED;
				}
				memcpy(staging, create_desc->pixel_data, create_desc->pixel_data_size);
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

void register_texture_create_loader(PulseAppId app, CGPUDeviceId device)
{
    pulse_asset_loader_desc ld{};
    ld.struct_size = sizeof(pulse_asset_loader_desc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_TEXTURE;
    ld.extensions = "";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_texture_create;
    ld.loader_size = sizeof(TextureLoaderState);
    ld.loader_align = alignof(TextureLoaderState);
    ld.settings_size = sizeof(pulse_graphics_texture_create_desc);
    ld.settings_align = alignof(pulse_graphics_texture_create_desc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_register_loader(app, &ld);
}

}

extern "C" {

pulse_texture_t pulse_graphics_texture_create(
    PulseAppId app,
    const pulse_graphics_texture_create_desc* desc)
{
	if (!desc)
        return {};

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device) 
        return {};

    pulse_asset_handle asset_handle = pulse_graphics_internal::asset_build(app, PULSE_TYPE_TEXTURE, nullptr, nullptr, 0, desc);
	return { asset_handle.index, asset_handle.generation };
}

}