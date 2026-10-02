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

const font_chain_data* require_chain(pulse_font_plugin_state* state, PulseFontChainHandle chain) {
    if (chain.index == 0 || chain.generation == 0 || !state->asset_system) {
        return nullptr;
    }
    const PulseAssetHandle asset = pulse_font_chain_to_handle(chain);
    void* ptr = nullptr;
    if (!pulse_asset_system_borrow(state->asset_system, asset, &ptr, nullptr) || !ptr) {
        return nullptr;
    }
    return static_cast<const font_chain_data*>(ptr);
}

PulseAssetHandle resolve_in_chain(pulse_font_plugin_state* state, const font_chain_data& chain, uint32_t codepoint) {
    for (const PulseAssetHandle& font : chain.fonts) {
        const font_asset_impl* face = font_face_borrow(state, font);
        if (!face) {
            continue;
        }
        if (font_glyph_index(*face, codepoint) != 0) {
            return font;
        }
    }
    return PulseAssetHandle{};
}

PulseGlyph make_glyph(pulse_font_plugin_state* state, const glyph_entry* entry, float size, uint32_t tier) {
    PulseGlyph glyph{};
    if (!entry) {
        return glyph;
    }
    const float scale = size / kTierBaseSizes[tier];
    if (!pulse_asset_handle_is_valid(entry->key.font)) {
        glyph.advance = size * 0.8f;
    } else {
        const font_asset_impl* face = font_face_borrow(state, entry->key.font);
        glyph.advance = face ? font_advance_raw(*face, entry->key.codepoint, size) : size * 0.5f;
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

float font_scale_for_size(const font_asset_impl& face, float size) {
    if (face.kind == kFontKindBitmap) {
        return size / (float)face.bitmap->line_height;
    }
    return stbtt_ScaleForMappingEmToPixels(&face.info, size);
}

int32_t font_glyph_index(const font_asset_impl& face, uint32_t codepoint) {
    if (codepoint > 0x10FFFF) {
        return 0;
    }
    if (face.kind == kFontKindBitmap) {
        return bitmap_font_find_glyph(*face.bitmap, codepoint) != nullptr ? 1 : 0;
    }
    return stbtt_FindGlyphIndex(&face.info, (int)codepoint);
}

float font_advance_raw(const font_asset_impl& face, uint32_t codepoint, float size) {
    const float scale = font_scale_for_size(face, size);
    if (face.kind == kFontKindBitmap) {
        const bitmap_glyph* glyph = bitmap_font_find_glyph(*face.bitmap, codepoint);
        const bitmap_glyph* fallback = glyph && glyph->xadvance > 0 ? glyph : bitmap_font_find_glyph(*face.bitmap, 'n');
        if (!fallback || fallback->xadvance <= 0) {
            return size * 0.5f;
        }
        return (float)fallback->xadvance * scale;
    }
    const int32_t glyph = font_glyph_index(face, codepoint);
    int advance = 0;
    int bearing = 0;
    if (glyph != 0) {
        stbtt_GetGlyphHMetrics(&face.info, glyph, &advance, &bearing);
    }
    if (advance <= 0) {
        const int32_t fb = font_glyph_index(face, 'n');
        if (fb != 0) {
            stbtt_GetGlyphHMetrics(&face.info, fb, &advance, &bearing);
        }
    }
    if (advance <= 0) {
        return size * 0.5f;
    }
    return advance * scale;
}

float font_kerning_raw(const font_asset_impl& face, uint32_t first, uint32_t second, float size) {
    if (face.kind == kFontKindBitmap) {
        return (float)bitmap_font_kerning(*face.bitmap, first, second) * font_scale_for_size(face, size);
    }
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
    register_font_asset_reflection(world);
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult pulse_font_plugin_post_build(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_font_plugin_state*>(ctx);
    if (!state) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    PulseAssetSystemId asset_system = pulse_get_asset_system(app);
    if (asset_system) {
        state->asset_system = asset_system;
        register_font_type(asset_system, app);
        register_font_loaders(asset_system);
        register_font_chain_type(asset_system, app);
        register_font_chain_loaders(asset_system);
        font_build_default_font(state);
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
    PulseAssetSystemId asset_system = app ? pulse_get_asset_system(app) : nullptr;
    if (asset_system) {
        pulse_asset_system_force_unload_assets(asset_system, PULSE_TYPE_FONT_CHAIN);
        pulse_asset_system_force_unload_assets(asset_system, PULSE_TYPE_FONT);
    }
    render_shutdown(state);
    atlas_shutdown(state);
    ecs_world_t* world = app ? pulse_app_world(app) : nullptr;
    if (world && ecs_id(pulse_font_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_font_state_resource);
        if (ecs_is_alive(world, ecs_id(pulse_font_state_resource))) {
            ecs_delete(world, ecs_id(pulse_font_state_resource));
        }
        ecs_id(pulse_font_state_resource) = 0;
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

    const char* font_dependencies[] = { "pulse_asset" };
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

PulseFontHandle pulse_font_default(PulseAppId app) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state) {
        PulseFontHandle invalid{};
        return invalid;
    }
    return PulseFontHandle{ state->default_font.index, state->default_font.generation };
}

PulseFontChainHandle pulse_font_create_chain(PulseAppId app, Pulse_Array_Param(const PulseFontHandle, fonts)) {
    PulseFontChainHandle result{};
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || !state->asset_system || fonts_count == 0 || !p_fonts) {
        return result;
    }
    std::vector<PulseAssetHandle> assets;
    assets.reserve(fonts_count);
    for (size_t i = 0; i < fonts_count; ++i) {
        const PulseAssetHandle asset = pulse_font_to_handle(p_fonts[i]);
        if (!pulse_asset_handle_is_valid(asset)) {
            return result;
        }
        assets.push_back(asset);
    }
    pulse_font_internal::font_chain_append_default(state, assets);
    std::vector<PulseAssetDependency> dependencies;
    dependencies.reserve(assets.size());
    for (const PulseAssetHandle& asset : assets) {
        PulseAssetDependency dep{};
        dep.dep_ref = { asset.type_id, asset.index, asset.generation };
        dep.requirement = PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED;
        dependencies.push_back(dep);
    }
    PulseAssetBuildDesc desc{};
    desc.struct_size = sizeof(PulseAssetBuildDesc);
    desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    desc.type_id = PULSE_TYPE_FONT_CHAIN;
    desc.loader_identifier = pulse_font_internal::kChainLoaderId;
    desc.p_dependencies = dependencies.data();
    desc.dependencies_count = dependencies.size();
    const PulseAssetHandle handle = pulse_asset_system_build_sync(state->asset_system, &desc);
    if (!pulse_asset_handle_is_valid(handle)) {
        std::fprintf(stderr, "pulse_font: failed to build font chain asset\n");
        return result;
    }
    result.index = handle.index;
    result.generation = handle.generation;
    return result;
}

void pulse_font_destroy_chain(PulseAppId app, PulseFontChainHandle chain) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || !state->asset_system || chain.index == 0 || chain.generation == 0) {
        return;
    }
    pulse_asset_system_release(state->asset_system, pulse_font_chain_to_handle(chain), nullptr);
}

float pulse_font_advance(PulseAppId app, PulseFontChainHandle chain, uint32_t codepoint, float size) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || size <= 0.0f) {
        return 0.0f;
    }
    const font_chain_data* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot) {
        return 0.0f;
    }
    const PulseAssetHandle font = pulse_font_internal::resolve_in_chain(state, *slot, codepoint);
    const font_asset_impl* face = font_face_borrow(state, font);
    if (!face) {
        return size * 0.8f;
    }
    return font_advance_raw(*face, codepoint, size);
}

float pulse_font_kerning(PulseAppId app, PulseFontChainHandle chain, uint32_t first, uint32_t second, float size) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || size <= 0.0f) {
        return 0.0f;
    }
    const font_chain_data* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot) {
        return 0.0f;
    }
    const PulseAssetHandle font = pulse_font_internal::resolve_in_chain(state, *slot, first);
    if (!pulse_asset_handle_is_valid(font) || !pulse_asset_handle_equals(font, pulse_font_internal::resolve_in_chain(state, *slot, second))) {
        return 0.0f;
    }
    const font_asset_impl* face = font_face_borrow(state, font);
    return face ? font_kerning_raw(*face, first, second, size) : 0.0f;
}

PulseVerticalMetrics pulse_font_vertical_metrics(PulseAppId app, PulseFontChainHandle chain, float size) {
    PulseVerticalMetrics metrics{};
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || size <= 0.0f) {
        return metrics;
    }
    const font_chain_data* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot || slot->fonts.empty()) {
        return metrics;
    }
    const font_asset_impl* face = font_face_borrow(state, slot->fonts[0]);
    if (!face) {
        return metrics;
    }
    const float scale = font_scale_for_size(*face, size);
    if (face->kind == kFontKindBitmap) {
        metrics.ascent = (float)face->bitmap->base * scale;
        metrics.descent = ((float)face->bitmap->base - (float)face->bitmap->line_height) * scale;
        metrics.line_gap = 0.0f;
        metrics.height = metrics.ascent - metrics.descent;
        return metrics;
    }
    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&face->info, &ascent, &descent, &line_gap);
    metrics.ascent = (float)ascent * scale;
    metrics.descent = (float)descent * scale;
    metrics.line_gap = (float)line_gap * scale;
    metrics.height = metrics.ascent - metrics.descent + metrics.line_gap;
    return metrics;
}

PulseGlyph pulse_font_glyph(PulseAppId app, PulseFontChainHandle chain, uint32_t codepoint, float size) {
    PulseGlyph glyph{};
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || size <= 0.0f) {
        return glyph;
    }
    const font_chain_data* slot = pulse_font_internal::require_chain(state, chain);
    if (!slot) {
        return glyph;
    }
    const uint32_t tier = font_tier_for_size(size);
    PulseAssetHandle font = pulse_font_internal::resolve_in_chain(state, *slot, codepoint);
    uint32_t key_codepoint = codepoint;
    if (!pulse_asset_handle_is_valid(font)) {
        font = PulseAssetHandle{};
        key_codepoint = kMissingGlyphCodepoint;
    }
    glyph_key key{};
    key.font = font;
    key.codepoint = key_codepoint;
    key.tier = tier;
    const glyph_entry* entry = atlas_acquire_glyph(state, key);
    return pulse_font_internal::make_glyph(state, entry, size, tier);
}

uint32_t pulse_font_page_count(PulseAppId app) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    return state ? (uint32_t)state->pages.size() : 0;
}

PulseFontPagePixels pulse_font_page_pixels(PulseAppId app, uint32_t page) {
    PulseFontPagePixels view{};
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || page >= state->pages.size()) {
        return view;
    }
    const std::vector<uint8_t>& pixels = state->pages[page].pixels;
    view.p_pixels = pixels.data();
    view.pixels_size = pixels.size();
    return view;
}

uint64_t pulse_font_page_version(PulseAppId app, uint32_t page) {
    pulse_font_plugin_state* state = pulse_font_internal::require_state(app);
    if (!state || page >= state->pages.size()) {
        return 0;
    }
    return state->pages[page].version;
}

}
