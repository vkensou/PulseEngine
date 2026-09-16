#include "font_internal.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#include "pulse_graphics.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

namespace pulse_font_internal {

constexpr const char* kPluginName = "pulse_font";

ECS_COMPONENT_DECLARE(pulse_font_state_resource);

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

PulseFontPluginDesc normalize_plugin_desc(const PulseFontPluginDesc* desc) {
    PulseFontPluginDesc normalized = pulse_font_plugin_desc_default();
    if (desc) {
        normalized = *desc;
    }
    normalized.struct_size = sizeof(PulseFontPluginDesc);
    normalized.version = PULSE_FONT_PLUGIN_DESC_VERSION;
    if (normalized.atlas_width < 256) {
        normalized.atlas_width = 2048;
    }
    if (normalized.atlas_height < 256) {
        normalized.atlas_height = 2048;
    }
    if (normalized.max_atlas_count == 0) {
        normalized.max_atlas_count = 4;
    }
    if (normalized.max_atlas_count > 8) {
        normalized.max_atlas_count = 8;
    }
    if (normalized.sdf_padding == 0) {
        normalized.sdf_padding = 8;
    }
    if (normalized.sdf_padding > 64) {
        normalized.sdf_padding = 64;
    }
    return normalized;
}

bool validate_plugin_desc(const PulseFontPluginDesc* desc) {
    return !desc || (desc->struct_size == sizeof(PulseFontPluginDesc) && desc->version == PULSE_FONT_PLUGIN_DESC_VERSION);
}

pulse_font_plugin_state* require_state(PulseAppId app) {
    return app ? state_from_app(app) : nullptr;
}

const font_chain_slot* require_chain(pulse_font_plugin_state* state, uint32_t chain) {
    if (chain == 0 || chain > state->chains.size()) {
        return nullptr;
    }
    const font_chain_slot& slot = state->chains[chain - 1];
    return slot.alive ? &slot : nullptr;
}

uint32_t resolve_in_chain(pulse_font_plugin_state* state, const font_chain_slot& chain, uint32_t codepoint) {
    for (uint32_t font : chain.fonts) {
        if (font == 0 || font > state->fonts.size()) {
            continue;
        }
        if (font_glyph_index(state->fonts[font - 1], codepoint) != 0) {
            return font;
        }
    }
    return PULSE_FONT_ID_NONE;
}

void decode_utf8(const char* text, std::vector<uint32_t>& out) {
    out.clear();
    if (!text) {
        return;
    }
    const uint8_t* cursor = reinterpret_cast<const uint8_t*>(text);
    while (*cursor) {
        uint32_t code = *cursor;
        uint32_t extra = 0;
        if (code >= 0xF0) {
            code &= 0x07;
            extra = 3;
        } else if (code >= 0xE0) {
            code &= 0x0F;
            extra = 2;
        } else if (code >= 0xC0) {
            code &= 0x1F;
            extra = 1;
        }
        ++cursor;
        for (uint32_t i = 0; i < extra && (*cursor & 0xC0) == 0x80; ++i) {
            code = (code << 6) | (*cursor & 0x3F);
            ++cursor;
        }
        out.push_back(code);
    }
}

PulseGlyph make_glyph(pulse_font_plugin_state* state, const glyph_entry* entry, float size, uint32_t tier) {
    PulseGlyph glyph{};
    if (!entry) {
        return glyph;
    }
    const float scale = size / kTierBaseSizes[tier];
    if (entry->key.font == kMissingGlyphFont) {
        glyph.advance = size * 0.8f;
    } else {
        glyph.advance = font_advance_raw(state->fonts[entry->key.font - 1], entry->key.codepoint, size);
    }
    if (entry->page == kInvalidIndex) {
        return glyph;
    }
    const atlas_page& page = state->pages[entry->page];
    const uint32_t page_width = state->desc.atlas_width;
    const uint32_t page_height = state->desc.atlas_height;
    const uint32_t slot_x = (entry->slot % page.grid_x) * entry->slot_size;
    const uint32_t slot_y = (entry->slot / page.grid_x) * entry->slot_size;
    glyph.valid = true;
    glyph.page = entry->page;
    glyph.x0 = entry->x0 * scale;
    glyph.y0 = entry->y0 * scale;
    glyph.x1 = entry->x1 * scale;
    glyph.y1 = entry->y1 * scale;
    glyph.u0 = ((float)slot_x + 0.5f) / (float)page_width;
    glyph.v0 = ((float)slot_y + 0.5f) / (float)page_height;
    glyph.u1 = ((float)(slot_x + entry->slot_size) - 0.5f) / (float)page_width;
    glyph.v1 = ((float)(slot_y + entry->slot_size) - 0.5f) / (float)page_height;
    return glyph;
}

}

float font_scale_for_size(const font_face& face, float size) {
    return stbtt_ScaleForMappingEmToPixels(&face.info, size);
}

int32_t font_glyph_index(const font_face& face, uint32_t codepoint) {
    if (codepoint > 0x10FFFF) {
        return 0;
    }
    return stbtt_FindGlyphIndex(&face.info, (int)codepoint);
}

float font_advance_raw(const font_face& face, uint32_t codepoint, float size) {
    const float scale = font_scale_for_size(face, size);
    const int32_t glyph = font_glyph_index(face, codepoint);
    int advance = 0;
    int bearing = 0;
    if (glyph != 0) {
        stbtt_GetGlyphHMetrics(&face.info, glyph, &advance, &bearing);
    }
    if (advance <= 0) {
        const int32_t fallback = font_glyph_index(face, 'n');
        if (fallback != 0) {
            stbtt_GetGlyphHMetrics(&face.info, fallback, &advance, &bearing);
        }
    }
    if (advance <= 0) {
        return size * 0.5f;
    }
    return advance * scale;
}

float font_kerning_raw(const font_face& face, uint32_t first, uint32_t second, float size) {
    const int32_t a = font_glyph_index(face, first);
    const int32_t b = font_glyph_index(face, second);
    if (a == 0 || b == 0) {
        return 0.0f;
    }
    const float scale = font_scale_for_size(face, size);
    return stbtt_GetGlyphKernAdvance(&face.info, a, b) * scale;
}

uint32_t font_tier_for_size(float size) {
    for (uint32_t i = 0; i < kTierCount; ++i) {
        if (size <= kTierBaseSizes[i]) {
            return i;
        }
    }
    return kTierCount - 1;
}

pulse_font_plugin_state* state_from_world(ecs_world_t* world) {
    if (!world || ecs_id(pulse_font_state_resource) == 0) {
        return nullptr;
    }
    const pulse_font_state_resource* resource = ecs_singleton_get(world, pulse_font_state_resource);
    return resource ? resource->state : nullptr;
}

pulse_font_plugin_state* state_from_app(PulseAppId app) {
    return state_from_world(pulse_app_world(app));
}

EPulsePluginBuildResult pulse_font_plugin_build(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_font_plugin_state*>(ctx);
    if (!state || !app) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    state->app = app;
    ecs_id(pulse_font_state_resource) = flecs::_::type<pulse_font_state_resource>::id(world);
    pulse_font_state_resource resource{};
    resource.state = state;
    ecs_singleton_set_ptr(world, pulse_font_state_resource, &resource);
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult pulse_font_plugin_post_build(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_font_plugin_state*>(ctx);
    if (!state) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (!pulse_get_renderer(app)) {
        return PULSE_PLUGIN_BUILD_RESULT_OK;
    }
    const EPulseResult result = render_init(state);
    return result == PULSE_RESULT_OK ? PULSE_PLUGIN_BUILD_RESULT_OK : PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
}

void pulse_font_plugin_shutdown(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_font_plugin_state*>(ctx);
    if (!state) {
        return;
    }
    render_shutdown(state);
    atlas_shutdown(state);
    ecs_world_t* world = app ? pulse_app_world(app) : nullptr;
    if (world && ecs_id(pulse_font_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_font_state_resource);
    }
    delete state;
}

}

using namespace pulse_font_internal;

extern "C" {

PulseFontPluginDesc pulse_font_plugin_desc_default(void) {
    PulseFontPluginDesc desc{};
    desc.struct_size = sizeof(PulseFontPluginDesc);
    desc.version = PULSE_FONT_PLUGIN_DESC_VERSION;
    desc.atlas_width = 2048;
    desc.atlas_height = 2048;
    desc.max_atlas_count = 4;
    desc.sdf_padding = 8;
    desc.record_priority = 100;
    return desc;
}

EPulseAppAddPluginResult pulse_add_font_plugin(PulseAppId app, const PulseFontPluginDesc* desc) {
    if (!app || !validate_plugin_desc(desc)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, pulse_font_internal::kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    auto* state = new pulse_font_plugin_state();
    state->desc = pulse_font_internal::normalize_plugin_desc(desc);

    const char* font_dependencies[] = { "pulse_vfs" };
    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_FONT_PLUGIN_DESC_VERSION,
        .name = pulse_font_internal::kPluginName,
        .ctx = state,
        .build = pulse_font_plugin_build,
        .post_build = pulse_font_plugin_post_build,
        .shutdown = pulse_font_plugin_shutdown,
        .dependency_count = 1,
        .dependencies = font_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, pulse_font_internal::kPluginName)) {
        delete state;
    }
    return result;
}

uint32_t pulse_font_register(PulseAppId app, Pulse_Blob_Param(memory), uint32_t face_index) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || !p_memory || memory_size == 0) {
        return PULSE_FONT_ID_NONE;
    }
    if (state->fonts.size() >= PULSE_FONT_MAX_COUNT) {
        std::fprintf(stderr, "pulse_font: font count limit %u reached\n", (unsigned)PULSE_FONT_MAX_COUNT);
        return PULSE_FONT_ID_NONE;
    }
    const auto* bytes = static_cast<const uint8_t*>(p_memory);
    const int face_count = stbtt_GetNumberOfFonts(bytes);
    if (face_count <= 0) {
        std::fprintf(stderr, "pulse_font: not a TrueType font\n");
        return PULSE_FONT_ID_NONE;
    }
    if (face_index >= (uint32_t)face_count) {
        std::fprintf(stderr, "pulse_font: face index %u out of range (%d)\n", (unsigned)face_index, face_count);
        return PULSE_FONT_ID_NONE;
    }
    const int offset = stbtt_GetFontOffsetForIndex(bytes, (int)face_index);
    if (offset < 0) {
        std::fprintf(stderr, "pulse_font: face offset missing for index %u\n", (unsigned)face_index);
        return PULSE_FONT_ID_NONE;
    }

    font_face face{};
    face.data.assign(bytes, bytes + memory_size);
    face.face_index = face_index;
    if (!stbtt_InitFont(&face.info, face.data.data(), offset)) {
        std::fprintf(stderr, "pulse_font: failed to initialize font face\n");
        return PULSE_FONT_ID_NONE;
    }
    face.family = pulse_font_internal::read_family_name(face.info);
    state->fonts.push_back(std::move(face));
    return (uint32_t)state->fonts.size();
}

uint32_t pulse_font_register_file(PulseAppId app, const char* path, uint32_t face_index) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || !path) {
        return PULSE_FONT_ID_NONE;
    }
    PulseVfsFileId file = pulse_vfs_open_read(path);
    if (!file) {
        std::fprintf(stderr, "pulse_font: cannot open font file '%s'\n", path);
        return PULSE_FONT_ID_NONE;
    }
    const int64_t length = pulse_vfs_file_length(file);
    if (length <= 0) {
        std::fprintf(stderr, "pulse_font: font file '%s' is empty\n", path);
        pulse_vfs_close(file);
        return PULSE_FONT_ID_NONE;
    }
    std::vector<uint8_t> buffer((size_t)length);
    const int64_t read = pulse_vfs_read_bytes(file, buffer.data(), (uint64_t)length);
    pulse_vfs_close(file);
    if (read != length) {
        std::fprintf(stderr, "pulse_font: short read on font file '%s'\n", path);
        return PULSE_FONT_ID_NONE;
    }
    return pulse_font_register(app, buffer.data(), buffer.size(), face_index);
}

uint32_t pulse_font_face_count(PulseAppId app, Pulse_Blob_Param(memory)) {
    (void)app;
    if (!p_memory || memory_size == 0) {
        return 0;
    }
    const int count = stbtt_GetNumberOfFonts(static_cast<const uint8_t*>(p_memory));
    return count > 0 ? (uint32_t)count : 0;
}

uint32_t pulse_font_count(PulseAppId app) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    return state ? (uint32_t)state->fonts.size() : 0;
}

const char* pulse_font_family_name(PulseAppId app, uint32_t font) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || font == 0 || font > state->fonts.size()) {
        return nullptr;
    }
    return state->fonts[font - 1].family.c_str();
}

uint32_t pulse_font_find_family(PulseAppId app, const char* family) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || !family) {
        return PULSE_FONT_ID_NONE;
    }
    for (uint32_t i = 0; i < state->fonts.size(); ++i) {
        if (state->fonts[i].family == family) {
            return i + 1;
        }
    }
    return PULSE_FONT_ID_NONE;
}

uint32_t pulse_font_create_chain(PulseAppId app, Pulse_Array_Param(const uint32_t, fonts)) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || fonts_count == 0 || !p_fonts) {
        return PULSE_FONT_ID_NONE;
    }
    for (size_t i = 0; i < fonts_count; ++i) {
        if (p_fonts[i] == PULSE_FONT_ID_NONE || p_fonts[i] > state->fonts.size()) {
            return PULSE_FONT_ID_NONE;
        }
    }
    if (!state->free_chains.empty()) {
        const uint32_t index = state->free_chains.back();
        state->free_chains.pop_back();
        state->chains[index].fonts.assign(p_fonts, p_fonts + fonts_count);
        state->chains[index].alive = true;
        return index + 1;
    }
    if (state->chains.size() >= kMaxChains) {
        std::fprintf(stderr, "pulse_font: chain count limit %u reached\n", (unsigned)kMaxChains);
        return PULSE_FONT_ID_NONE;
    }
    font_chain_slot slot{};
    slot.fonts.assign(p_fonts, p_fonts + fonts_count);
    slot.alive = true;
    state->chains.push_back(std::move(slot));
    return (uint32_t)state->chains.size();
}

void pulse_font_destroy_chain(PulseAppId app, uint32_t chain) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || chain == 0 || chain > state->chains.size()) {
        return;
    }
    font_chain_slot& slot = state->chains[chain - 1];
    if (!slot.alive) {
        return;
    }
    slot.alive = false;
    slot.fonts.clear();
    state->free_chains.push_back(chain - 1);
}

uint32_t pulse_font_resolve_codepoint(PulseAppId app, uint32_t chain, uint32_t codepoint) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state) {
        return PULSE_FONT_ID_NONE;
    }
    const font_chain_slot* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot) {
        return PULSE_FONT_ID_NONE;
    }
    return pulse_font_internal::resolve_in_chain(state, *slot, codepoint);
}

float pulse_font_advance(PulseAppId app, uint32_t chain, uint32_t codepoint, float size) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || size <= 0.0f) {
        return 0.0f;
    }
    const font_chain_slot* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot) {
        return 0.0f;
    }
    const uint32_t font = pulse_font_internal::resolve_in_chain(state, *slot, codepoint);
    if (font == PULSE_FONT_ID_NONE) {
        return size * 0.8f;
    }
    return font_advance_raw(state->fonts[font - 1], codepoint, size);
}

float pulse_font_kerning(PulseAppId app, uint32_t chain, uint32_t first, uint32_t second, float size) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || size <= 0.0f) {
        return 0.0f;
    }
    const font_chain_slot* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot) {
        return 0.0f;
    }
    const uint32_t font = pulse_font_internal::resolve_in_chain(state, *slot, first);
    if (font == PULSE_FONT_ID_NONE || font != pulse_font_internal::resolve_in_chain(state, *slot, second)) {
        return 0.0f;
    }
    return font_kerning_raw(state->fonts[font - 1], first, second, size);
}

PulseVerticalMetrics pulse_font_vertical_metrics(PulseAppId app, uint32_t chain, float size) {
    PulseVerticalMetrics metrics{};
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || size <= 0.0f) {
        return metrics;
    }
    const font_chain_slot* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot || slot->fonts.empty()) {
        return metrics;
    }
    const font_face& face = state->fonts[slot->fonts[0] - 1];
    const float scale = font_scale_for_size(face, size);
    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&face.info, &ascent, &descent, &line_gap);
    metrics.ascent = (float)ascent * scale;
    metrics.descent = (float)descent * scale;
    metrics.line_gap = (float)line_gap * scale;
    metrics.height = metrics.ascent - metrics.descent + metrics.line_gap;
    return metrics;
}

PulseGlyph pulse_font_glyph(PulseAppId app, uint32_t chain, uint32_t codepoint, float size) {
    PulseGlyph glyph{};
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || size <= 0.0f) {
        return glyph;
    }
    const font_chain_slot* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot) {
        return glyph;
    }
    const uint32_t tier = font_tier_for_size(size);
    uint32_t font = pulse_font_internal::resolve_in_chain(state, *slot, codepoint);
    uint32_t key_codepoint = codepoint;
    if (font == PULSE_FONT_ID_NONE) {
        font = kMissingGlyphFont;
        key_codepoint = kMissingGlyphCodepoint;
    }
    glyph_key key{};
    key.font = font;
    key.codepoint = key_codepoint;
    key.tier = tier;
    const glyph_entry* entry = atlas_acquire_glyph(state, key);
    return pulse_font_internal::make_glyph(state, entry, size, tier);
}

void pulse_font_prewarm(PulseAppId app, uint32_t chain, const char* text, float size) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || !text || size <= 0.0f) {
        return;
    }
    const font_chain_slot* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot) {
        return;
    }
    std::vector<uint32_t> codepoints;
    pulse_font_internal::decode_utf8(text, codepoints);
    for (uint32_t codepoint : codepoints) {
        if (codepoint == '\n' || codepoint == '\r' || codepoint == '\t') {
            continue;
        }
        pulse_font_glyph(app, chain, codepoint, size);
    }
}

PulseAtlasStats pulse_font_atlas_stats(PulseAppId app) {
    PulseAtlasStats stats{};
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state) {
        return stats;
    }
    for (const glyph_entry& entry : state->entries) {
        if (entry.occupied && entry.page != kInvalidIndex) {
            ++stats.glyph_count;
        }
    }
    for (const atlas_page& page : state->pages) {
        stats.slot_count += page.grid_x * page.grid_y;
    }
    stats.page_count = (uint32_t)state->pages.size();
    stats.eviction_count = state->eviction_count;
    stats.rasterize_count = state->rasterize_count;
    return stats;
}

uint8_t pulse_font_atlas_sample(PulseAppId app, uint32_t page, uint32_t x, uint32_t y) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || page >= state->pages.size()) {
        return 0;
    }
    const atlas_page& target = state->pages[page];
    if (x >= state->desc.atlas_width || y >= state->desc.atlas_height) {
        return 0;
    }
    return target.pixels[(size_t)y * state->desc.atlas_width + x];
}

}
