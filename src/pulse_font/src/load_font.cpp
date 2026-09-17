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

void destroy_font_asset(void* ptr, void* user_data) {
    auto* data = static_cast<PulseFontAssetData*>(ptr);
    delete data->impl;
    data->impl = nullptr;
    pulse_font_plugin_state* state = state_from_app(static_cast<PulseAppId>(user_data));
    if (state) {
        font_registry_release(state, data->self);
    }
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
    auto* data = static_cast<PulseFontAssetData*>(ctx->out_asset);
    data->impl = impl;
    data->self = { ctx->request.type_id, ctx->request.index, ctx->request.generation };
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

void register_font_load_loader(PulseAssetSystemId asset_system) {
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
}

uint32_t font_slot_of(pulse_font_plugin_state* state, PulseFontHandle handle) {
    if (handle.index == 0) {
        return PULSE_FONT_ID_NONE;
    }
    for (uint32_t i = 0; i < state->fonts.size(); ++i) {
        if (state->fonts[i].occupied && state->fonts[i].asset.index == handle.index && state->fonts[i].asset.generation == handle.generation) {
            return i + 1;
        }
    }
    return PULSE_FONT_ID_NONE;
}

PulseFontHandle font_handle_of(pulse_font_plugin_state* state, uint32_t font) {
    if (font == PULSE_FONT_ID_NONE || font > state->fonts.size() || !state->fonts[font - 1].occupied) {
        PulseFontHandle invalid{};
        return invalid;
    }
    const PulseAssetHandle& asset = state->fonts[font - 1].asset;
    return PulseFontHandle{ asset.index, asset.generation };
}

void font_registry_release(pulse_font_plugin_state* state, PulseAssetHandle handle) {
    for (uint32_t i = 0; i < state->fonts.size(); ++i) {
        font_face& face = state->fonts[i];
        if (!face.occupied || !pulse_asset_handle_equals(face.asset, handle)) {
            continue;
        }
        atlas_purge_font(state, i + 1);
        face = font_face{};
        state->free_fonts.push_back(i);
        return;
    }
}

PulseFontHandle font_get_handle_impl(PulseAppId app, PulseFontRequest request) {
    PulseFontHandle invalid{};
    pulse_font_plugin_state* state = state_from_app(app);
    PulseAssetSystemId asset_system = app ? pulse_get_asset_system(app) : nullptr;
    if (!state || !asset_system) {
        return invalid;
    }
    PulseAssetHandle handle = pulse_asset_system_get_handle(asset_system, pulse_font_request_to_asset_request(request));
    if (!pulse_asset_handle_is_valid(handle)) {
        return invalid;
    }
    PulseFontHandle result{ handle.index, handle.generation };
    if (font_slot_of(state, result) != PULSE_FONT_ID_NONE) {
        return result;
    }
    void* ptr = nullptr;
    if (!pulse_asset_system_borrow(asset_system, handle, &ptr, nullptr)) {
        return invalid;
    }
    auto* data = static_cast<PulseFontAssetData*>(ptr);
    if (!data->impl) {
        return invalid;
    }
    if (font_occupied_count(state) >= PULSE_FONT_MAX_COUNT) {
        std::fprintf(stderr, "pulse_font: font count limit %u reached\n", (unsigned)PULSE_FONT_MAX_COUNT);
        return invalid;
    }
    uint32_t index = kInvalidIndex;
    if (!state->free_fonts.empty()) {
        index = state->free_fonts.back();
        state->free_fonts.pop_back();
    } else {
        state->fonts.push_back(font_face{});
        index = (uint32_t)state->fonts.size() - 1;
    }
    font_face& face = state->fonts[index];
    face = font_face{};
    face.info = data->impl->info;
    face.family = data->impl->family;
    face.asset = handle;
    face.occupied = true;
    return result;
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

bool pulse_font_is_ready(PulseAppId app, PulseFontRequest request) {
    PulseAssetSystemId asset_system = app ? pulse_get_asset_system(app) : nullptr;
    return asset_system && pulse_asset_system_is_ready(asset_system, pulse_font_request_to_asset_request(request));
}

bool pulse_font_is_alive(PulseAppId app, PulseFontRequest request) {
    PulseAssetSystemId asset_system = app ? pulse_get_asset_system(app) : nullptr;
    return asset_system && pulse_asset_system_is_alive(asset_system, pulse_font_request_to_asset_request(request));
}

const char* pulse_font_get_error(PulseAppId app, PulseFontRequest request) {
    PulseAssetSystemId asset_system = app ? pulse_get_asset_system(app) : nullptr;
    if (!asset_system) {
        return nullptr;
    }
    return pulse_asset_system_get_error(asset_system, pulse_font_request_to_asset_request(request));
}

PulseFontHandle pulse_font_get_handle(PulseAppId app, PulseFontRequest request) {
    return pulse_font_internal::font_get_handle_impl(app, request);
}

}
