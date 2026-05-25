#include "graphic_internal.h"

#include "renderer.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ktx.h"

namespace pulse_graphic_internal {

pulse_asset_loader_status_t step_texture_stb(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<TextureLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* texture = static_cast<pulse_texture_data_t*>(ctx->out_asset);

        int w = 0, h = 0, comp = 0;
        auto* pixels = stbi_load_from_memory(ctx->bytes, ctx->byte_size, &w, &h, &comp, 4);
        if (!pixels) {
            *out_error = "texture stb loader: texture parse failed";
            return PULSE_ASSET_LOADER_FAILED;
        }

        bool mipmap = false;
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
            return PULSE_ASSET_LOADER_FAILED;
        }

        stbi_image_free(pixels);

        s->upload_requested = true;
        return PULSE_ASSET_LOADER_PENDING;
    }

    if (s->upload_completed) {
        return PULSE_ASSET_LOADER_DONE;
    }

    return PULSE_ASSET_LOADER_PENDING;
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

pulse_asset_loader_status_t step_texture_ktx(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    auto* s = static_cast<TextureLoaderState*>(state);

    if (!s->upload_requested) {
        CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
        auto* texture = static_cast<pulse_texture_data_t*>(ctx->out_asset);

        ktxResult result = KTX_SUCCESS;
        ktxTexture* ktxTexture;
        result = ktxTexture_CreateFromMemory(ctx->bytes, ctx->byte_size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktxTexture);
        if (result != KTX_SUCCESS)
        {
            *out_error = "texture ktx loader: texture parse failed";
            return PULSE_ASSET_LOADER_FAILED;
        }

        auto [format, component] = detectKtxTextureFormat(ktxTexture);
        if (ktxTexture->isCompressed || format == CGPU_TEXTURE_FORMAT_UNDEFINED)
        {
            ktxTexture_Destroy(ktxTexture);
            return PULSE_ASSET_LOADER_FAILED;
        }

        uint32_t width = ktxTexture->baseWidth;
        uint32_t height = ktxTexture->baseHeight;
        uint32_t mipLevels = ktxTexture->numLevels;
        uint32_t arraySize = 1;

        bool mipmap = true;
        bool generateMipmap = mipmap && mipLevels <= 1;
        generateMipmap = false;
        mipLevels = mipmap ? (mipLevels > 1 ? mipLevels : (static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1)) : 1;
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
            return PULSE_ASSET_LOADER_FAILED;
        }

        uint32_t textureComponent = FormatUtil_BitSizeOfBlock(format) / 8;
        auto mipedSize = [](uint64_t size, uint64_t mip) { return std::max<uint64_t>(size >> mip, 1ull); };

        bool genMip = mipmap && mipLevels > ktxTexture->numLevels;
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
        return PULSE_ASSET_LOADER_PENDING;
    }

    if (s->upload_completed) {
        return PULSE_ASSET_LOADER_DONE;
    }

    return PULSE_ASSET_LOADER_PENDING;
}

}
