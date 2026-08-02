#include "../graphics_internal.h"

#include "renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ktx.h"

namespace pulse_graphics_internal {

struct TextureLoaderState {
    bool upload_requested = false;
    bool upload_completed = false;
};

EPulseAssetLoaderStatus step_texture_stb(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<TextureLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* texture = static_cast<PulseTextureData*>(ctx->out_asset);

        auto load_desc = static_cast<const PulseTextureLoadDesc*>(ctx->settings);

        int w = 0, h = 0, comp = 0;
        auto* pixels = stbi_load_from_memory(ctx->bytes, ctx->byte_size, &w, &h, &comp, 4);
        if (!pixels) {
            *out_error = "texture stb loader: texture parse failed";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        bool mipmap = load_desc->generate_mipmaps;
        auto mipLevels = mipmap ? static_cast<uint32_t>(std::floor(std::log2(std::max(w, h)))) + 1 : 1;
        CGPUTextureDescriptor texture_desc =
        {
            .name = ctx->path,
            .width = (uint64_t)w,
            .height = (uint64_t)h,
            .depth = 1,
            .array_size = 1,
            .format = CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB,
            .mip_levels = mipLevels,
            .owner_queue = CGPU_NULLPTR,
            .start_state = CGPU_RESOURCE_STATE_UNDEFINED,
            .descriptors = ECGPUResourceTypeFlags(mipmap ? CGPU_RESOURCE_TYPE_TEXTURE | CGPU_RESOURCE_TYPE_RENDER_TARGET : CGPU_RESOURCE_TYPE_TEXTURE),
        };

        HGEGraphics::init_texture(texture, device, texture_desc);

        auto* gstate = state_from_app(ctx->app);
        if (gstate) {
            auto* staging = queue_staging_texture_full(gstate, texture, 1, (mipLevels > 1), nullptr, &s->upload_completed);
            memcpy(staging, pixels, w * h * 4);
        } else {
            stbi_image_free(pixels);
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        stbi_image_free(pixels);

        s->upload_requested = true;
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }

    if (s->upload_completed) {
        return PULSE_ASSET_LOADER_STATUS_DONE;
    }

    return PULSE_ASSET_LOADER_STATUS_PENDING;
}

std::pair<ECGPUTextureFormat, int> detectKtxTextureFormat(ktxTexture* ktxTexture)
{
    if (ktxTexture->classId == ktxTexture1_c)
    {
        auto ktx1 = (ktxTexture1*)ktxTexture;
        switch (ktx1->glInternalformat)
        {
        case 0x1908:
            return { CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB, 4 };
        case 0x8058:
            return { CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM, 4 };
        case 0x881A:
            return { CGPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT, 8 };
        }
        printf("format: %d\n", ktx1->glFormat);
    }
    else if (ktxTexture->classId == ktxTexture2_c)
    {
        auto ktx2 = (ktxTexture2*)ktxTexture;
        switch (ktx2->vkFormat)
        {
        case 23:
            return { CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB, 3 };
        case 37:
            return { CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM, 4 };
        }
        printf("format: %d\n", ktx2->vkFormat);
    }
    return { CGPU_TEXTURE_FORMAT_UNDEFINED, 0 };
}

EPulseAssetLoaderStatus step_texture_ktx(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<TextureLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* texture = static_cast<PulseTextureData*>(ctx->out_asset);

        auto load_desc = static_cast<const PulseTextureLoadDesc*>(ctx->settings);

        ktxResult result = KTX_SUCCESS;
        ktxTexture* ktxTexture;
        result = ktxTexture_CreateFromMemory(ctx->bytes, ctx->byte_size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
        if (result != KTX_SUCCESS)
        {
            *out_error = "texture ktx loader: texture parse failed";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        auto [format, component] = detectKtxTextureFormat(ktxTexture);
        if (ktxTexture->isCompressed || format == CGPU_TEXTURE_FORMAT_UNDEFINED)
        {
            ktxTexture_Destroy(ktxTexture);
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        uint32_t width = ktxTexture->baseWidth;
        uint32_t height = ktxTexture->baseHeight;
        uint32_t mipLevels = ktxTexture->numLevels;
        uint32_t arraySize = 1;

        bool generateMipmap = load_desc->generate_mipmaps && mipLevels <= 1;
		if (generateMipmap) {
			mipLevels = std::max(mipLevels, static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1);
		}
        ECGPUResourceTypeFlags descriptors = CGPU_RESOURCE_TYPE_TEXTURE;
        if (generateMipmap)
            descriptors |= CGPU_RESOURCE_TYPE_RENDER_TARGET;
        if (ktxTexture->isCubemap)
        {
            descriptors |= CGPU_RESOURCE_TYPE_TEXTURE_CUBE;
            arraySize = 6;
        }
        CGPUTextureDescriptor texture_desc =
        {
            .name = ctx->path,
            .width = (uint64_t)width,
            .height = (uint64_t)height,
            .depth = 1,
            .array_size = arraySize,
            .format = format,
            .mip_levels = mipLevels,
            .owner_queue = CGPU_NULLPTR,
            .start_state = CGPU_RESOURCE_STATE_UNDEFINED,
            .descriptors = descriptors,
        };

        HGEGraphics::init_texture(texture, device, texture_desc);

        auto* gstate = state_from_app(ctx->app);
        if (!gstate) {
            ktxTexture_Destroy(ktxTexture);
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }

        uint32_t textureComponent = FormatUtil_BitSizeOfBlock(format) / 8;
        auto mipedSize = [](uint64_t size, uint64_t mip) { return std::max<uint64_t>(size >> mip, 1ull); };

        bool genMip = mipLevels > ktxTexture->numLevels;
        uint64_t totalSize = 0;
        auto* staging = queue_staging_texture_full(gstate, texture,
            static_cast<uint8_t>(ktxTexture->numLevels), genMip,
            &totalSize, &s->upload_completed);

        uint8_t* offset_data = staging;
        auto ktxTextureData = ktxTexture_GetData(ktxTexture);
        for (uint32_t mip = 0; mip < ktxTexture->numLevels; ++mip) {
            uint64_t mipW = mipedSize(width, mip);
            uint64_t mipH = mipedSize(height, mip);
            for (ktx_uint32_t slice = 0; slice < ktxTexture->numLayers; ++slice) {
                for (ktx_uint32_t face = 0; face < ktxTexture->numFaces; ++face) {
                    ktx_size_t ktxOffset;
                    ktxTexture_GetImageOffset(ktxTexture, mip, slice, face, &ktxOffset);
                    if (textureComponent == static_cast<uint32_t>(component)) {
                        memcpy(offset_data, ktxTextureData + ktxOffset, mipW * mipH * component);
                    } else {
                        for (size_t i = 0; i < mipW * mipH; ++i) {
                            memcpy(offset_data + i * textureComponent, ktxTextureData + ktxOffset + i * component, component);
                        }
                    }
                    offset_data += mipW * mipH * textureComponent;
                }
            }
        }

        ktxTexture_Destroy(ktxTexture);

        s->upload_requested = true;
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }

    if (s->upload_completed) {
        return PULSE_ASSET_LOADER_STATUS_DONE;
    }

    return PULSE_ASSET_LOADER_STATUS_PENDING;
}

void register_texture_load_loader(PulseAppId app, CGPUDeviceId device)
{
    PulseAssetLoaderDesc ld1{};
    ld1.struct_size = sizeof(PulseAssetLoaderDesc);
    ld1.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld1.type_id = PULSE_TYPE_TEXTURE;
    ld1.extensions = "png,jpg,bmp,tga";
    ld1.ctor = nullptr;
    ld1.dtor = nullptr;
    ld1.step = step_texture_stb;
    ld1.loader_size = sizeof(TextureLoaderState);
    ld1.loader_align = alignof(TextureLoaderState);
    ld1.settings_size = sizeof(PulseTextureLoadDesc);
    ld1.settings_align = alignof(PulseTextureLoadDesc);
    ld1.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld1);

    PulseAssetLoaderDesc ld2{};
    ld2.struct_size = sizeof(PulseAssetLoaderDesc);
    ld2.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld2.type_id = PULSE_TYPE_TEXTURE;
    ld2.extensions = "ktx";
    ld2.ctor = nullptr;
    ld2.dtor = nullptr;
    ld2.step = step_texture_ktx;
    ld2.loader_size = sizeof(TextureLoaderState);
    ld2.loader_align = alignof(TextureLoaderState);
    ld2.settings_size = sizeof(PulseTextureLoadDesc);
    ld2.settings_align = alignof(PulseTextureLoadDesc);
    ld2.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(pulse_get_asset_system(app), &ld2);
}

}

extern "C" {

    PulseTextureHandle pulse_load_texture(
        PulseAppId app,
        const PulseTextureLoadDesc* desc)
    {
        PulseAssetHandle h = pulse_graphics_internal::asset_load_path(app, PULSE_TYPE_TEXTURE, desc->filepath, desc);
        if (!pulse_asset_handle_is_valid(h)) return PulseTextureHandle{};
        return PulseTextureHandle{ h.index, h.generation };
    }

}