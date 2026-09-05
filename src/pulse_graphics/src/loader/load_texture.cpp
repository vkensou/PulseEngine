#include "../graphics_internal.h"

#include "renderer.h"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cmath>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ktx.h"
#include "KHR/khr_df.h"
#include "vkformat_enum.h"

extern "C" const char* vkFormatString(VkFormat format);

namespace pulse_graphics_internal {

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
        auto* pixels = stbi_load_from_memory(static_cast<const stbi_uc*>(ctx->p_bytes), static_cast<int>(ctx->bytes_size), &w, &h, &comp, 4);
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
            PulseTextureHandle handle = { ctx->request.index, ctx->request.generation };
            auto* staging = queue_staging_texture_full(gstate, handle, texture, 1, (mipLevels > 1), nullptr, &s->upload_completed);
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

namespace {

enum class TranscodeGate { Any, DualChannel, OpaqueOnly };

struct TranscodeCandidate {
    ktx_transcode_fmt_e target;
    ECGPUTextureFormat srgb_format;
    ECGPUTextureFormat unorm_format;
    bool always_available;
    TranscodeGate gate;
};

const TranscodeCandidate kTranscodeCandidates[] = {
    { KTX_TTF_ETC2_EAC_RG11,  CGPU_TEXTURE_FORMAT_EAC_R11G11_UNORM_BLOCK,     CGPU_TEXTURE_FORMAT_EAC_R11G11_UNORM_BLOCK,      false, TranscodeGate::DualChannel },
    { KTX_TTF_BC5_RG,         CGPU_TEXTURE_FORMAT_BC5_UNORM_BLOCK,            CGPU_TEXTURE_FORMAT_BC5_UNORM_BLOCK,             false, TranscodeGate::DualChannel },
    { KTX_TTF_BC7_RGBA,       CGPU_TEXTURE_FORMAT_BC7_SRGB_BLOCK,             CGPU_TEXTURE_FORMAT_BC7_UNORM_BLOCK,             false, TranscodeGate::Any },
    { KTX_TTF_BC1_OR_3,       CGPU_TEXTURE_FORMAT_BC3_SRGB_BLOCK,             CGPU_TEXTURE_FORMAT_BC3_UNORM_BLOCK,             false, TranscodeGate::Any },
    { KTX_TTF_ASTC_4x4_RGBA,  CGPU_TEXTURE_FORMAT_ASTC_4X4_SRGB_BLOCK,        CGPU_TEXTURE_FORMAT_ASTC_4X4_UNORM_BLOCK,        false, TranscodeGate::Any },
    { KTX_TTF_ETC1_RGB,       CGPU_TEXTURE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,     CGPU_TEXTURE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,     false, TranscodeGate::OpaqueOnly },
    { KTX_TTF_ETC2_RGBA,      CGPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,   CGPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,   false, TranscodeGate::Any },
    { KTX_TTF_RGBA32,         CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB,              CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM,              true,  TranscodeGate::Any },
};

ECGPUTextureFormat vk_format_to_cgpu(uint32_t vk_format)
{
    switch (vk_format)
    {
    case VK_FORMAT_R8_UNORM: return CGPU_TEXTURE_FORMAT_R8_UNORM;
    case VK_FORMAT_R8_SRGB: return CGPU_TEXTURE_FORMAT_R8_SRGB;
    case VK_FORMAT_R8G8_UNORM: return CGPU_TEXTURE_FORMAT_R8G8_UNORM;
    case VK_FORMAT_R8G8_SRGB: return CGPU_TEXTURE_FORMAT_R8G8_SRGB;
    case VK_FORMAT_R8G8B8_UNORM: return CGPU_TEXTURE_FORMAT_R8G8B8_UNORM;
    case VK_FORMAT_R8G8B8_SRGB: return CGPU_TEXTURE_FORMAT_R8G8B8_SRGB;
    case VK_FORMAT_R8G8B8A8_UNORM: return CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8A8_SRGB: return CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB;
    case VK_FORMAT_B8G8R8A8_UNORM: return CGPU_TEXTURE_FORMAT_B8G8R8A8_UNORM;
    case VK_FORMAT_B8G8R8A8_SRGB: return CGPU_TEXTURE_FORMAT_B8G8R8A8_SRGB;
    case VK_FORMAT_R16_SFLOAT: return CGPU_TEXTURE_FORMAT_R16_SFLOAT;
    case VK_FORMAT_R16G16_SFLOAT: return CGPU_TEXTURE_FORMAT_R16G16_SFLOAT;
    case VK_FORMAT_R16G16B16A16_SFLOAT: return CGPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT;
    case VK_FORMAT_R32_SFLOAT: return CGPU_TEXTURE_FORMAT_R32_SFLOAT;
    case VK_FORMAT_R32G32_SFLOAT: return CGPU_TEXTURE_FORMAT_R32G32_SFLOAT;
    case VK_FORMAT_R32G32B32A32_SFLOAT: return CGPU_TEXTURE_FORMAT_R32G32B32A32_SFLOAT;

    case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC1_RGB_UNORM_BLOCK;
    case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_BC1_RGB_SRGB_BLOCK;
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_BC1_RGBA_SRGB_BLOCK;
    case VK_FORMAT_BC2_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC2_UNORM_BLOCK;
    case VK_FORMAT_BC2_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_BC2_SRGB_BLOCK;
    case VK_FORMAT_BC3_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC3_UNORM_BLOCK;
    case VK_FORMAT_BC3_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_BC3_SRGB_BLOCK;
    case VK_FORMAT_BC4_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC4_UNORM_BLOCK;
    case VK_FORMAT_BC4_SNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC4_SNORM_BLOCK;
    case VK_FORMAT_BC5_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC5_UNORM_BLOCK;
    case VK_FORMAT_BC5_SNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC5_SNORM_BLOCK;
    case VK_FORMAT_BC6H_UFLOAT_BLOCK: return CGPU_TEXTURE_FORMAT_BC6H_UFLOAT_BLOCK;
    case VK_FORMAT_BC6H_SFLOAT_BLOCK: return CGPU_TEXTURE_FORMAT_BC6H_SFLOAT_BLOCK;
    case VK_FORMAT_BC7_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_BC7_UNORM_BLOCK;
    case VK_FORMAT_BC7_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_BC7_SRGB_BLOCK;

    case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ETC2_R8G8B8_SRGB_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
    case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK;
    case VK_FORMAT_EAC_R11_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_EAC_R11_UNORM_BLOCK;
    case VK_FORMAT_EAC_R11_SNORM_BLOCK: return CGPU_TEXTURE_FORMAT_EAC_R11_SNORM_BLOCK;
    case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_EAC_R11G11_UNORM_BLOCK;
    case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return CGPU_TEXTURE_FORMAT_EAC_R11G11_SNORM_BLOCK;

    case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_4X4_UNORM_BLOCK;
    case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_4X4_SRGB_BLOCK;
    case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_5X4_UNORM_BLOCK;
    case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_5X4_SRGB_BLOCK;
    case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_5X5_UNORM_BLOCK;
    case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_5X5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_6X5_UNORM_BLOCK;
    case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_6X5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_6X6_UNORM_BLOCK;
    case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_6X6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_8X5_UNORM_BLOCK;
    case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_8X5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_8X6_UNORM_BLOCK;
    case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_8X6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_8X8_UNORM_BLOCK;
    case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_8X8_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_10X5_UNORM_BLOCK;
    case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_10X5_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_10X6_UNORM_BLOCK;
    case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_10X6_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_10X8_UNORM_BLOCK;
    case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_10X8_SRGB_BLOCK;
    case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_10X10_UNORM_BLOCK;
    case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_10X10_SRGB_BLOCK;
    case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_12X10_UNORM_BLOCK;
    case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_12X10_SRGB_BLOCK;
    case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_12X12_UNORM_BLOCK;
    case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return CGPU_TEXTURE_FORMAT_ASTC_12X12_SRGB_BLOCK;
    default: return CGPU_TEXTURE_FORMAT_UNDEFINED;
    }
}

bool texture_format_supports(CGPUAdapterId adapter, ECGPUTextureFormat format, ECGPUTextureFormatSupportFlags flags)
{
    if (format <= CGPU_TEXTURE_FORMAT_UNDEFINED || format >= CGPU_TEXTURE_FORMAT_COUNT) return false;
    const CGPUAdapterDetail* detail = cgpu_adapter_query_adapter_detail(adapter);
    if (!detail) return false;
    return (detail->format_supports[format] & flags) == flags;
}

const char* basis_model_name(uint32_t* bdb)
{
    switch (KHR_DFDVAL(bdb, MODEL))
    {
    case KHR_DF_MODEL_ETC1S: return "etc1s";
    case KHR_DF_MODEL_UASTC: return "uastc";
    default: return "other";
    }
}

bool dfd_is_dual_channel(uint32_t* bdb)
{
    switch (KHR_DFDVAL(bdb, MODEL))
    {
    case KHR_DF_MODEL_UASTC: return KHR_DFDSVAL(bdb, 0, CHANNELID) == KHR_DF_CHANNEL_UASTC_RRRG;
    case KHR_DF_MODEL_ETC1S: return KHR_DFDSAMPLECOUNT(bdb) > 1 && KHR_DFDSVAL(bdb, 1, CHANNELID) == KHR_DF_CHANNEL_ETC1S_GGG;
    default: return false;
    }
}

bool dfd_is_opaque(uint32_t* bdb)
{
    switch (KHR_DFDVAL(bdb, MODEL))
    {
    case KHR_DF_MODEL_ETC1S: return KHR_DFDSAMPLECOUNT(bdb) < 2;
    case KHR_DF_MODEL_UASTC: { const uint32_t chan = KHR_DFDSVAL(bdb, 0, CHANNELID); return chan == KHR_DF_CHANNEL_UASTC_RGB || chan == KHR_DF_CHANNEL_UASTC_RRR; }
    default: return false;
    }
}

uint32_t ktx_slice_count(ktxTexture* ktx, uint32_t mip)
{
    if (ktx->isCubemap) return ktx->numFaces;
    return std::max(1u, ktx->baseDepth >> mip);
}

bool transcode_to_supported_format(ktxTexture2* ktx2, CGPUAdapterId adapter, const char* path, ECGPUTextureFormat* out_format, const char** out_error)
{
    uint32_t* bdb = ktx2->pDfd + 1;
    const bool srgb = KHR_DFDVAL(bdb, TRANSFER) == KHR_DF_TRANSFER_SRGB;
    const char* model = basis_model_name(bdb);
    const bool dual_channel = dfd_is_dual_channel(bdb);
    const bool opaque = dfd_is_opaque(bdb);
    const uint32_t source_format = ktx2->vkFormat;

    for (const TranscodeCandidate& candidate : kTranscodeCandidates)
    {
        if (candidate.gate == TranscodeGate::DualChannel && !dual_channel) continue;
        if (candidate.gate == TranscodeGate::OpaqueOnly && !opaque) continue;
        ECGPUTextureFormat probe = srgb ? candidate.srgb_format : candidate.unorm_format;
        if (!candidate.always_available && !texture_format_supports(adapter, probe, CGPU_TEXTURE_FORMAT_SUPPORT_SAMPLE)) continue;

        auto begin = std::chrono::steady_clock::now();
        KTX_error_code result = ktxTexture2_TranscodeBasis(ktx2, candidate.target, 0);
        if (result == KTX_UNSUPPORTED_TEXTURE_TYPE || result == KTX_UNSUPPORTED_FEATURE || result == KTX_INVALID_VALUE) continue;
        if (result != KTX_SUCCESS)
        {
            printf("ktx2: %s transcode to %s failed: %s\n", path, ktxTranscodeFormatString(candidate.target), ktxErrorString(result));
            *out_error = "texture ktx loader: basis transcode failed";
            return false;
        }

        ECGPUTextureFormat format = vk_format_to_cgpu(ktx2->vkFormat);
        if (format == CGPU_TEXTURE_FORMAT_UNDEFINED)
        {
            printf("ktx2: %s transcode to %s produced vkFormat %u, which has no CGPU mapping\n", path, ktxTranscodeFormatString(candidate.target), ktx2->vkFormat);
            *out_error = "texture ktx loader: transcoded format has no CGPU mapping";
            return false;
        }

        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - begin).count();
        printf("ktx2: %s %s %s -> %s, %ux%u x%u levels, in %lld ms\n", path, model, vkFormatString(static_cast<VkFormat>(source_format)), vkFormatString(static_cast<VkFormat>(ktx2->vkFormat)), ktx2->baseWidth, ktx2->baseHeight, ktx2->numLevels, (long long)elapsed);
        if (candidate.always_available)
            printf("ktx2: %s has no block-compressed target on this device, falling back to uncompressed 32bpp\n", path);
        *out_format = format;
        return true;
    }

    printf("ktx2: %s has no usable transcode target\n", path);
    *out_error = "texture ktx loader: no transcode target available";
    return false;
}

struct UploadLayout {
    ECGPUTextureFormat source_format;
    ECGPUTextureFormat upload_format;
    bool expand_rgb_to_rgba;
};

uint32_t rgba8_counterpart(uint32_t vk_format)
{
    switch (vk_format)
    {
    case VK_FORMAT_R8G8B8_UNORM: return VK_FORMAT_R8G8B8A8_UNORM;
    case VK_FORMAT_R8G8B8_SRGB: return VK_FORMAT_R8G8B8A8_SRGB;
    default: return VK_FORMAT_UNDEFINED;
    }
}

uint64_t image_texel_count(ECGPUTextureFormat format, uint64_t image_size)
{
    return image_size * 8 / FormatUtil_BitSizeOfBlock(format);
}

void expand_rgb_to_rgba(uint8_t* dst, const uint8_t* src, uint64_t texels)
{
    for (uint64_t i = 0; i < texels; ++i) {
        const uint8_t* pixel = src + i * 3;
        uint8_t* rgba = dst + i * 4;
        rgba[0] = pixel[0];
        rgba[1] = pixel[1];
        rgba[2] = pixel[2];
        rgba[3] = 0xff;
    }
}

bool resolve_upload_format(ktxTexture2* ktx2, CGPUAdapterId adapter, const char* path, UploadLayout* out_layout, const char** out_error)
{
    if (ktxTexture2_NeedsTranscoding(ktx2))
    {
        ECGPUTextureFormat transcoded = CGPU_TEXTURE_FORMAT_UNDEFINED;
        if (!transcode_to_supported_format(ktx2, adapter, path, &transcoded, out_error))
            return false;
        out_layout->source_format = transcoded;
        out_layout->upload_format = transcoded;
        out_layout->expand_rgb_to_rgba = false;
        return true;
    }

    ECGPUTextureFormat format = vk_format_to_cgpu(ktx2->vkFormat);
    if (format == CGPU_TEXTURE_FORMAT_UNDEFINED)
    {
        printf("ktx2: %s uses vkFormat %s, which has no CGPU mapping\n", path, vkFormatString(static_cast<VkFormat>(ktx2->vkFormat)));
        *out_error = "texture ktx loader: unsupported vkFormat";
        return false;
    }
    if (texture_format_supports(adapter, format, CGPU_TEXTURE_FORMAT_SUPPORT_SAMPLE))
    {
        out_layout->source_format = format;
        out_layout->upload_format = format;
        out_layout->expand_rgb_to_rgba = false;
        return true;
    }

    uint32_t rgba8_vk = rgba8_counterpart(ktx2->vkFormat);
    if (rgba8_vk != VK_FORMAT_UNDEFINED)
    {
        out_layout->source_format = format;
        out_layout->upload_format = vk_format_to_cgpu(rgba8_vk);
        out_layout->expand_rgb_to_rgba = true;
        printf("ktx2: %s is %s %ux%u x%u levels, which this device cannot sample, expanding to %s on the cpu\n", path, vkFormatString(static_cast<VkFormat>(ktx2->vkFormat)), ktx2->baseWidth, ktx2->baseHeight, ktx2->numLevels, vkFormatString(static_cast<VkFormat>(rgba8_vk)));
        return true;
    }

    printf("ktx2: %s is %s %ux%u, which this device cannot sample", path, vkFormatString(static_cast<VkFormat>(ktx2->vkFormat)), ktx2->baseWidth, ktx2->baseHeight);
    if (ktx2->isCompressed)
        printf("; produce this file from a UASTC or ETC1S master with 'ktx transcode --target <format>' (ktx transcode cannot read an already block-compressed file)");
    printf("\n");
    *out_error = "texture ktx loader: format not supported by device";
    return false;
}

}

EPulseAssetLoaderStatus step_texture_ktx(
    void* state, const PulseAssetLoadTask* ctx,
    const char** out_error)
{
    auto* s = static_cast<TextureLoaderState*>(state);

    if (s->upload_requested) {
        return s->upload_completed ? PULSE_ASSET_LOADER_STATUS_DONE : PULSE_ASSET_LOADER_STATUS_PENDING;
    }

    CGPUDeviceId device = static_cast<CGPUDeviceId>(ctx->user_data);
    auto* texture = static_cast<PulseTextureData*>(ctx->out_asset);

    ktxTexture* ktx = nullptr;
    KTX_error_code create_result = ktxTexture_CreateFromMemory(static_cast<const ktx_uint8_t*>(ctx->p_bytes), static_cast<ktx_size_t>(ctx->bytes_size), KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &ktx);
    if (create_result != KTX_SUCCESS || !ktx) {
        printf("ktx2: %s parse failed: %s\n", ctx->path, ktxErrorString(create_result));
        *out_error = "texture ktx loader: texture parse failed";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    if (ktx->classId != ktxTexture2_c) {
        ktxTexture_Destroy(ktx);
        *out_error = "texture ktx loader: KTX1 is not supported, repack the file with ktx create or ktx2ktx2";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    auto* ktx2 = reinterpret_cast<ktxTexture2*>(ktx);

    if (ktx->numFaces != (ktx->isCubemap ? 6u : 1u)) {
        ktxTexture_Destroy(ktx);
        *out_error = "texture ktx loader: invalid face count";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    UploadLayout layout = {};
    if (!resolve_upload_format(ktx2, device->adapter, ctx->path, &layout, out_error)) {
        ktxTexture_Destroy(ktx);
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    const uint32_t width = ktx->baseWidth;
    const uint32_t height = ktx->baseHeight;
    const uint32_t depth = ktx->baseDepth;
    const uint32_t sourceLevels = ktx->numLevels;
    const uint32_t arraySize = ktx->numLayers * ktx->numFaces;
    uint32_t mipLevels = sourceLevels;
    bool generate_mipmaps = false;
    if (ktx->generateMipmaps) {
        if (depth > 1) {
            printf("ktx2: %s is a 3D texture, mipmaps are never generated for 3D, uploading its %u stored level(s)\n", ctx->path, sourceLevels);
        } else if (ktx->isCompressed) {
            printf("ktx2: %s asks for runtime mipmaps but is block-compressed, bake the chain with --generate-mipmap\n", ctx->path);
        } else {
            mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
            generate_mipmaps = mipLevels > 1;
            if (generate_mipmaps)
                printf("ktx2: %s generates %u mips for %u slices from the stored level at upload\n", ctx->path, mipLevels, arraySize);
        }
    }

    uint64_t sourceTotal = 0;
    for (uint32_t mip = 0; mip < sourceLevels; ++mip) {
        const ktx_size_t imageSize = ktxTexture_GetImageSize(ktx, mip);
        if (imageSize != HGEGraphics::texture_image_size(layout.source_format, width, height, 1, mip)) {
            ktxTexture_Destroy(ktx);
            *out_error = "texture ktx loader: image size does not match the cgpu block layout";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        const uint32_t slices = ktx_slice_count(ktx, mip);
        for (ktx_uint32_t layer = 0; layer < ktx->numLayers; ++layer) {
            for (ktx_uint32_t slice = 0; slice < slices; ++slice) {
                ktx_size_t offset = 0;
                if (ktxTexture_GetImageOffset(ktx, mip, layer, slice, &offset) != KTX_SUCCESS) {
                    ktxTexture_Destroy(ktx);
                    *out_error = "texture ktx loader: image offsets are unavailable, the texture is not decoded";
                    return PULSE_ASSET_LOADER_STATUS_FAILED;
                }
                sourceTotal += imageSize;
            }
        }
    }
    if (sourceTotal != HGEGraphics::texture_data_size(layout.source_format, width, height, depth, sourceLevels, arraySize)) {
        ktxTexture_Destroy(ktx);
        *out_error = "texture ktx loader: image data does not match the cgpu texture layout";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    ECGPUResourceTypeFlags descriptors = CGPU_RESOURCE_TYPE_TEXTURE;
    if (generate_mipmaps)
        descriptors |= CGPU_RESOURCE_TYPE_RENDER_TARGET;
    if (ktx->isCubemap)
        descriptors |= CGPU_RESOURCE_TYPE_TEXTURE_CUBE;

    CGPUTextureDescriptor texture_desc =
    {
        .name = ctx->path,
        .width = (uint64_t)width,
        .height = (uint64_t)height,
        .depth = (uint64_t)depth,
        .array_size = arraySize,
        .format = layout.upload_format,
        .mip_levels = mipLevels,
        .owner_queue = CGPU_NULLPTR,
        .start_state = CGPU_RESOURCE_STATE_UNDEFINED,
        .descriptors = descriptors,
    };

    auto* gstate = state_from_app(ctx->app);
    if (!gstate) {
        ktxTexture_Destroy(ktx);
        *out_error = "texture ktx loader: graphics state unavailable";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    HGEGraphics::init_texture(texture, device, texture_desc);

    uint64_t totalSize = 0;
    PulseTextureHandle handle = { ctx->request.index, ctx->request.generation };
    auto* staging = queue_staging_texture_full(gstate, handle, texture,
        static_cast<uint8_t>(sourceLevels), generate_mipmaps, &totalSize, &s->upload_completed);

    const uint8_t* ktxData = static_cast<const uint8_t*>(ktxTexture_GetData(ktx));
    uint64_t written = 0;
    for (uint32_t mip = 0; mip < sourceLevels; ++mip) {
        const ktx_size_t imageSize = ktxTexture_GetImageSize(ktx, mip);
        const uint64_t texels = image_texel_count(layout.source_format, imageSize);
        const uint64_t uploadSize = HGEGraphics::texture_image_size(layout.upload_format, width, height, 1, mip);
        const uint32_t slices = ktx_slice_count(ktx, mip);
        for (ktx_uint32_t layer = 0; layer < ktx->numLayers; ++layer) {
            for (ktx_uint32_t slice = 0; slice < slices; ++slice) {
                ktx_size_t ktxOffset = 0;
                assert(ktxTexture_GetImageOffset(ktx, mip, layer, slice, &ktxOffset) == KTX_SUCCESS);
                if (layout.expand_rgb_to_rgba)
                    expand_rgb_to_rgba(staging + written, ktxData + ktxOffset, texels);
                else
                    memcpy(staging + written, ktxData + ktxOffset, imageSize);
                written += uploadSize;
            }
        }
    }
    assert(written == totalSize);

    ktxTexture_Destroy(ktx);

    s->upload_requested = true;
    return PULSE_ASSET_LOADER_STATUS_PENDING;
}

void register_texture_load_loader(PulseAssetSystemId asset_system, CGPUDeviceId device)
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
    pulse_asset_system_register_loader(asset_system, &ld1);

    PulseAssetLoaderDesc ld2{};
    ld2.struct_size = sizeof(PulseAssetLoaderDesc);
    ld2.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld2.type_id = PULSE_TYPE_TEXTURE;
    ld2.extensions = "ktx,ktx2";
    ld2.ctor = nullptr;
    ld2.dtor = nullptr;
    ld2.step = step_texture_ktx;
    ld2.loader_size = sizeof(TextureLoaderState);
    ld2.loader_align = alignof(TextureLoaderState);
    ld2.settings_size = sizeof(PulseTextureLoadDesc);
    ld2.settings_align = alignof(PulseTextureLoadDesc);
    ld2.user_data = const_cast<struct CGPUDevice*>(device);
    pulse_asset_system_register_loader(asset_system, &ld2);
}

}

extern "C" {

    PulseTextureRequest pulse_load_texture(
        PulseAppId app,
        const PulseTextureLoadDesc* desc)
    {
        PulseAssetSystemId as = pulse_graphics_internal::asset_system_from_app(app);
        PulseAssetRequest request = pulse_graphics_internal::asset_load_path(as, PULSE_TYPE_TEXTURE, desc->filepath, desc);
        if (!pulse_asset_request_is_valid(request)) return PulseTextureRequest{};
        return PulseTextureRequest{ request.index, request.generation };
    }

}
