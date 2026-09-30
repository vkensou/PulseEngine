#include "font_internal.h"

#include <cstdio>

namespace pulse_font_internal {

namespace {

constexpr uint32_t kNameIdFamily = 1;

void encode_utf16be(const uint8_t* src, uint32_t length, std::string& out) {
    for (uint32_t i = 0; i + 1 < length; i += 2) {
        uint32_t code = ((uint32_t)src[i] << 8) | src[i + 1];
        if (code >= 0xD800 && code <= 0xDBFF && i + 3 < length) {
            const uint32_t low = ((uint32_t)src[i + 2] << 8) | src[i + 3];
            if (low >= 0xDC00 && low <= 0xDFFF) {
                code = 0x10000 + ((code - 0xD800) << 10) + (low - 0xDC00);
                i += 2;
            }
        }
        if (code < 0x80) {
            out.push_back((char)code);
        } else if (code < 0x800) {
            out.push_back((char)(0xC0 | (code >> 6)));
            out.push_back((char)(0x80 | (code & 0x3F)));
        } else if (code < 0x10000) {
            out.push_back((char)(0xE0 | (code >> 12)));
            out.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (code & 0x3F)));
        } else {
            out.push_back((char)(0xF0 | (code >> 18)));
            out.push_back((char)(0x80 | ((code >> 12) & 0x3F)));
            out.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (code & 0x3F)));
        }
    }
}

std::string read_family_name(const stbtt_fontinfo& info) {
    int length = 0;
    const char* raw = stbtt_GetFontNameString(&info, &length, 1, 0, 0, kNameIdFamily);
    if (raw && length > 0) {
        return std::string(raw, (size_t)length);
    }
    raw = stbtt_GetFontNameString(&info, &length, 3, 1, 0x409, kNameIdFamily);
    if (raw && length > 0) {
        std::string name;
        encode_utf16be(reinterpret_cast<const uint8_t*>(raw), (uint32_t)length, name);
        return name;
    }
    raw = stbtt_GetFontNameString(&info, &length, 3, 1, 0, kNameIdFamily);
    if (raw && length > 0) {
        std::string name;
        encode_utf16be(reinterpret_cast<const uint8_t*>(raw), (uint32_t)length, name);
        return name;
    }
    return std::string();
}

void font_track_loaded(pulse_font_plugin_state* state, PulseAssetHandle handle) {
    for (size_t i = 0; i < state->loaded_fonts.size(); ++i) {
        if (pulse_asset_handle_equals(state->loaded_fonts[i], handle)) {
            return;
        }
    }
    state->loaded_fonts.push_back(handle);
}

bool step_font_common(void* state, const PulseAssetLoadTask* ctx, font_asset_impl* impl) {
    (void)state;
    auto* data = static_cast<PulseFontAssetData*>(ctx->out_asset);
    data->impl = impl;
    data->self = { ctx->request.type_id, ctx->request.index, ctx->request.generation };
    pulse_font_plugin_state* plugin_state = state_from_app(ctx->app);
    if (!plugin_state) {
        return false;
    }
    font_track_loaded(plugin_state, data->self);
    return true;
}

void destroy_font_asset(void* ptr, void* user_data) {
    auto* data = static_cast<PulseFontAssetData*>(ptr);
    delete data->impl;
    data->impl = nullptr;
    pulse_font_plugin_state* state = state_from_app(static_cast<PulseAppId>(user_data));
    if (!state) {
        return;
    }
    for (size_t i = 0; i < state->loaded_fonts.size(); ++i) {
        if (pulse_asset_handle_equals(state->loaded_fonts[i], data->self)) {
            state->loaded_fonts.erase(state->loaded_fonts.begin() + (ptrdiff_t)i);
            break;
        }
    }
    if (pulse_asset_handle_equals(state->default_font, data->self)) {
        state->default_font = PulseAssetHandle{};
    }
    atlas_purge_font(state, data->self);
}

constexpr const char* kDefaultFontLoaderId = "pulse_font_default";

font_asset_impl* build_bitmap_impl(const uint8_t* fnt, size_t fnt_size, const uint8_t* png, size_t png_size, const char** out_error) {
    bitmap_font_parse_result parsed{};
    if (!bitmap_font_parse(fnt, fnt_size, parsed, out_error)) {
        return nullptr;
    }
    if (!bitmap_font_attach_page(parsed.data, png, png_size, out_error)) {
        return nullptr;
    }
    auto* impl = new font_asset_impl();
    impl->kind = kFontKindBitmap;
    impl->bitmap = std::make_unique<bitmap_font_data>(std::move(parsed.data));
    impl->family = impl->bitmap->family;
    return impl;
}

EPulseAssetLoaderStatus step_font_loader(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    (void)state;
    const auto* settings = static_cast<const PulseFontLoadSettings*>(ctx->settings);
    const uint32_t face_index = settings ? settings->face_index : 0u;
    const auto* bytes = static_cast<const uint8_t*>(ctx->p_bytes);
    const int face_count = stbtt_GetNumberOfFonts(bytes);
    if (face_count <= 0) {
        *out_error = "font loader: not a TrueType/OpenType font";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    if (face_index >= (uint32_t)face_count) {
        *out_error = "font loader: face index out of range";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    const int offset = stbtt_GetFontOffsetForIndex(bytes, (int)face_index);
    if (offset < 0) {
        *out_error = "font loader: font face offset not found";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    auto* impl = new font_asset_impl();
    impl->bytes.assign(bytes, bytes + ctx->bytes_size);
    if (!stbtt_InitFont(&impl->info, impl->bytes.data(), offset)) {
        delete impl;
        *out_error = "font loader: failed to initialize font face";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    impl->family = read_family_name(impl->info);
    step_font_common(state, ctx, impl);
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

EPulseAssetLoaderStatus step_font_bitmap_loader(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    (void)state;
    const auto* settings = static_cast<const PulseFontLoadSettings*>(ctx->settings);
    if (settings && settings->face_index != 0) {
        *out_error = "bitmap font loader: face index is not supported";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    const auto* bytes = static_cast<const uint8_t*>(ctx->p_bytes);
    bitmap_font_parse_result parsed{};
    if (!bitmap_font_parse(bytes, (size_t)ctx->bytes_size, parsed, out_error)) {
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    const std::string page_path = bitmap_font_page_path(ctx->path, parsed.page_file);
    std::vector<uint8_t> png_bytes;
    if (!bitmap_font_read_file(page_path.c_str(), png_bytes)) {
        std::fprintf(stderr, "pulse_font: failed to read bitmap page texture '%s'\n", page_path.c_str());
        *out_error = "bitmap font loader: failed to read page texture";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    if (!bitmap_font_attach_page(parsed.data, png_bytes.data(), png_bytes.size(), out_error)) {
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    auto* impl = new font_asset_impl();
    impl->kind = kFontKindBitmap;
    impl->bitmap = std::make_unique<bitmap_font_data>(std::move(parsed.data));
    impl->family = impl->bitmap->family;
    step_font_common(state, ctx, impl);
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

EPulseAssetLoaderStatus step_font_default_loader(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    (void)state;
    const std::vector<uint8_t>& fnt = default_font_fnt_bytes();
    const std::vector<uint8_t>& png = default_font_png_bytes();
    font_asset_impl* impl = build_bitmap_impl(fnt.data(), fnt.size(), png.data(), png.size(), out_error);
    if (!impl) {
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    step_font_common(state, ctx, impl);
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

PulseAssetRequest asset_load_with_face_index(PulseAssetSystemId asset_system, const char* path, uint32_t face_index, const void* data, uint64_t size) {
    PulseFontLoadSettings settings{};
    settings.face_index = face_index;
    if (data) {
        PulseAssetMemoryLoadDesc desc{};
        desc.struct_size = sizeof(PulseAssetMemoryLoadDesc);
        desc.version = PULSE_ASSET_MEMORY_LOAD_DESC_VERSION;
        desc.type_id = PULSE_TYPE_FONT;
        desc.path = path;
        desc.data = data;
        desc.size = size;
        desc.settings = &settings;
        return pulse_asset_system_load_from_memory(asset_system, &desc);
    }
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = PULSE_TYPE_FONT;
    desc.path = path;
    desc.settings = &settings;
    return pulse_asset_system_load(asset_system, &desc);
}

}

void register_font_type(PulseAssetSystemId asset_system, PulseAppId app) {
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_FONT;
    type_desc.size = sizeof(PulseFontAssetData);
    type_desc.align = alignof(PulseFontAssetData);
    type_desc.destroy = destroy_font_asset;
    type_desc.user_data = app;
    pulse_asset_system_register_type(asset_system, &type_desc);
}

void register_font_loaders(PulseAssetSystemId asset_system) {
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_FONT;
    ld.extensions = "ttf,otf,ttc";
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_font_loader;
    ld.loader_size = 0;
    ld.loader_align = 0;
    ld.settings_size = sizeof(PulseFontLoadSettings);
    ld.settings_align = alignof(PulseFontLoadSettings);
    ld.user_data = nullptr;
    pulse_asset_system_register_loader(asset_system, &ld);

    ld.extensions = "fnt";
    ld.loader_identifier = nullptr;
    ld.step = step_font_bitmap_loader;
    pulse_asset_system_register_loader(asset_system, &ld);

    ld.extensions = nullptr;
    ld.loader_identifier = kDefaultFontLoaderId;
    ld.step = step_font_default_loader;
    pulse_asset_system_register_loader(asset_system, &ld);
}

void font_build_default_font(pulse_font_plugin_state* state) {
    PulseAssetSystemId asset_system = pulse_get_asset_system(state->app);
    if (!asset_system || pulse_asset_handle_is_valid(state->default_font)) {
        return;
    }
    PulseAssetBuildDesc desc{};
    desc.struct_size = sizeof(PulseAssetBuildDesc);
    desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    desc.type_id = PULSE_TYPE_FONT;
    desc.loader_identifier = kDefaultFontLoaderId;
    desc.name = "default_font";
    const PulseAssetHandle handle = pulse_asset_system_build_sync(asset_system, &desc);
    if (!pulse_asset_handle_is_valid(handle)) {
        std::fprintf(stderr, "pulse_font: failed to build default font asset\n");
        return;
    }
    // 常驻引用：默认字体是插件生命周期资产，不随外部 release 消失。
    pulse_asset_system_retain(asset_system, handle, nullptr);
    state->default_font = handle;
}

const font_asset_impl* font_face_borrow(pulse_font_plugin_state* state, PulseAssetHandle handle) {
    if (!state || !state->asset_system || !pulse_asset_handle_is_valid(handle)) {
        return nullptr;
    }
    void* ptr = nullptr;
    if (!pulse_asset_system_borrow(state->asset_system, handle, &ptr, nullptr) || !ptr) {
        return nullptr;
    }
    const auto* data = static_cast<const PulseFontAssetData*>(ptr);
    return data->impl;
}

}

using namespace pulse_font_internal;

extern "C" {

PulseFontRequest pulse_font_load(PulseAppId app, const char* path, uint32_t face_index) {
    PulseFontRequest result{};
    PulseAssetSystemId asset_system = app ? pulse_get_asset_system(app) : nullptr;
    if (!asset_system || !path || !path[0]) {
        return result;
    }
    PulseAssetRequest request = pulse_font_internal::asset_load_with_face_index(asset_system, path, face_index, nullptr, 0);
    if (!pulse_asset_request_is_valid(request)) {
        return result;
    }
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

PulseFontRequest pulse_font_load_from_memory(PulseAppId app, const char* name, Pulse_Blob_Param(memory), uint32_t face_index) {
    PulseFontRequest result{};
    PulseAssetSystemId asset_system = app ? pulse_get_asset_system(app) : nullptr;
    if (!asset_system || !name || !name[0] || !p_memory || memory_size == 0) {
        return result;
    }
    PulseAssetRequest request = pulse_font_internal::asset_load_with_face_index(asset_system, name, face_index, p_memory, (uint64_t)memory_size);
    if (!pulse_asset_request_is_valid(request)) {
        return result;
    }
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

}
