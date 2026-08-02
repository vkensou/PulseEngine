#include "../graphics_internal.h"

#include "renderer.h"

namespace pulse_graphics_internal {

struct TextureLoaderState {
    bool upload_requested = false;
    bool upload_completed = false;
};

EPulseAssetLoaderStatus step_texture_create(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<TextureLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* texture = static_cast<PulseTextureData*>(ctx->out_asset);

		auto create_desc = static_cast<const PulseTextureCreateDesc*>(ctx->settings);

        HGEGraphics::init_texture(texture, device, create_desc->desc);

		if (create_desc->pixel_data && create_desc->pixel_data_size > 0) {
			auto* gstate = state_from_app(ctx->app);
			if (gstate) {
                uint64_t staging_size = 0;
				auto* staging = queue_staging_texture_full(gstate, texture, 1, create_desc->generate_mipmaps, &staging_size, &s->upload_completed);
				if (staging_size < create_desc->pixel_data_size) {
					*out_error = "texture loader: pixel data size exceeds staging buffer size";
					return PULSE_ASSET_LOADER_STATUS_FAILED;
				}
				memcpy(staging, create_desc->pixel_data, create_desc->pixel_data_size);
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

void register_texture_create_loader(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_TEXTURE;
    ld.extensions = "";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_texture_create;
    ld.loader_size = sizeof(TextureLoaderState);
    ld.loader_align = alignof(TextureLoaderState);
    ld.settings_size = sizeof(PulseTextureCreateDesc);
    ld.settings_align = alignof(PulseTextureCreateDesc);
    ld.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld);
}

}

extern "C" {

PulseTextureHandle pulse_create_texture(
    PulseAppId app,
    const PulseTextureCreateDesc* desc)
{
	if (!desc)
        return {};

    CGPUDeviceId device = pulse_graphics_internal::get_device(app);
    if (!device) 
        return {};

    PulseAssetHandle asset_handle = pulse_graphics_internal::asset_build(app, PULSE_TYPE_TEXTURE, nullptr, nullptr, 0, desc);
	return { asset_handle.index, asset_handle.generation };
}

}