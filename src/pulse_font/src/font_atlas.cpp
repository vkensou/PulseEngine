#include "font_internal.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace pulse_font_internal {

namespace {

constexpr uint32_t kTableEmpty = 0u;
constexpr uint32_t kTableTombstone = 0xFFFFFFFFu;
constexpr float kEdtInfinity = 1.0e20f;

uint32_t hash_key(const glyph_key& key) {
    uint32_t h = key.font * 0x9E3779B1u;
    h ^= key.codepoint * 0x85EBCA77u;
    h ^= key.tier * 0xC2B2AE3Du;
    h ^= h >> 15;
    h *= 0x2545F491u;
    h ^= h >> 13;
    return h;
}

bool key_equal(const glyph_key& a, const glyph_key& b) {
    return a.font == b.font && a.codepoint == b.codepoint && a.tier == b.tier;
}

void table_rehash(pulse_font_plugin_state* state, uint32_t new_size) {
    std::vector<uint32_t> next(new_size, kTableEmpty);
    const uint32_t mask = new_size - 1;
    uint32_t used = 0;
    for (uint32_t i = 0; i < state->entries.size(); ++i) {
        if (!state->entries[i].occupied) {
            continue;
        }
        uint32_t slot = hash_key(state->entries[i].key) & mask;
        while (next[slot] != kTableEmpty) {
            slot = (slot + 1) & mask;
        }
        next[slot] = i + 1;
        ++used;
    }
    state->table = std::move(next);
    state->table_used = used;
    state->table_occupied = used;
}

void table_reserve(pulse_font_plugin_state* state) {
    if (state->table.empty()) {
        table_rehash(state, 1024);
        return;
    }
    if ((state->table_occupied + 1) * 4 < state->table.size() * 3) {
        return;
    }
    table_rehash(state, state->table_used * 4 >= state->table.size() * 3 ? state->table.size() * 2 : state->table.size());
}

void table_insert(pulse_font_plugin_state* state, uint32_t entry_index) {
    table_reserve(state);
    const uint32_t mask = (uint32_t)state->table.size() - 1;
    uint32_t slot = hash_key(state->entries[entry_index].key) & mask;
    uint32_t tombstone = kTableEmpty;
    while (state->table[slot] != kTableEmpty) {
        if (state->table[slot] == kTableTombstone) {
            if (tombstone == kTableEmpty) {
                tombstone = slot;
            }
        } else if (state->table[slot] == entry_index + 1) {
            return;
        }
        slot = (slot + 1) & mask;
    }
    if (tombstone != kTableEmpty) {
        state->table[tombstone] = entry_index + 1;
    } else {
        state->table[slot] = entry_index + 1;
        ++state->table_occupied;
    }
    ++state->table_used;
}

uint32_t table_find(pulse_font_plugin_state* state, const glyph_key& key) {
    if (state->table.empty()) {
        return kInvalidIndex;
    }
    const uint32_t mask = (uint32_t)state->table.size() - 1;
    uint32_t slot = hash_key(key) & mask;
    while (state->table[slot] != kTableEmpty) {
        if (state->table[slot] != kTableTombstone) {
            const glyph_entry& entry = state->entries[state->table[slot] - 1];
            if (entry.occupied && key_equal(entry.key, key)) {
                return state->table[slot] - 1;
            }
        }
        slot = (slot + 1) & mask;
    }
    return kInvalidIndex;
}

void table_remove(pulse_font_plugin_state* state, const glyph_key& key) {
    if (state->table.empty()) {
        return;
    }
    const uint32_t mask = (uint32_t)state->table.size() - 1;
    uint32_t slot = hash_key(key) & mask;
    while (state->table[slot] != kTableEmpty) {
        if (state->table[slot] != kTableTombstone) {
            const glyph_entry& entry = state->entries[state->table[slot] - 1];
            if (entry.occupied && key_equal(entry.key, key)) {
                state->table[slot] = kTableTombstone;
                --state->table_used;
                return;
            }
        }
        slot = (slot + 1) & mask;
    }
}

uint32_t alloc_entry(pulse_font_plugin_state* state) {
    if (!state->free_entries.empty()) {
        const uint32_t index = state->free_entries.back();
        state->free_entries.pop_back();
        state->entries[index] = glyph_entry{};
        return index;
    }
    state->entries.push_back(glyph_entry{});
    return (uint32_t)state->entries.size() - 1;
}

void free_entry(pulse_font_plugin_state* state, uint32_t entry_index) {
    state->entries[entry_index] = glyph_entry{};
    state->free_entries.push_back(entry_index);
}

void edt_1d(const float* f, uint32_t n, float* d, int32_t* v, float* z) {
    int32_t k = 0;
    v[0] = 0;
    z[0] = -kEdtInfinity;
    z[1] = kEdtInfinity;
    for (uint32_t q = 1; q < n; ++q) {
        float s = ((f[q] + (float)q * (float)q) - (f[v[k]] + (float)v[k] * (float)v[k])) / (float)(2 * (int32_t)q - 2 * v[k]);
        while (s <= z[k]) {
            --k;
            s = ((f[q] + (float)q * (float)q) - (f[v[k]] + (float)v[k] * (float)v[k])) / (float)(2 * (int32_t)q - 2 * v[k]);
        }
        ++k;
        v[k] = (int32_t)q;
        z[k] = s;
        z[k + 1] = kEdtInfinity;
    }
    k = 0;
    for (uint32_t q = 0; q < n; ++q) {
        while (z[k + 1] < (float)q) {
            ++k;
        }
        const float delta = (float)q - (float)v[k];
        d[q] = delta * delta + f[v[k]];
    }
}

void edt_2d(pulse_font_plugin_state* state, const float* src, float* dst, uint32_t w, uint32_t h) {
    const uint32_t longest = std::max(w, h);
    state->edt_parabola.resize(longest);
    state->edt_boundary.resize(longest + 1);
    state->edt_column_in.resize(longest);
    state->edt_column_out.resize(longest);
    int32_t* v = state->edt_parabola.data();
    float* z = state->edt_boundary.data();
    float* col = state->edt_column_in.data();
    float* out = state->edt_column_out.data();
    for (uint32_t y = 0; y < h; ++y) {
        edt_1d(src + (size_t)y * w, w, dst + (size_t)y * w, v, z);
    }
    for (uint32_t x = 0; x < w; ++x) {
        for (uint32_t y = 0; y < h; ++y) {
            col[y] = dst[(size_t)y * w + x];
        }
        edt_1d(col, h, out, v, z);
        for (uint32_t y = 0; y < h; ++y) {
            dst[(size_t)y * w + x] = out[y];
        }
    }
}

void build_sdf(pulse_font_plugin_state* state, const uint8_t* mask, uint8_t* out, uint32_t size, uint32_t padding) {
    const uint32_t count = size * size;
    state->edt_source.resize(count);
    state->edt_distance.resize(count);
    state->edt_outside.resize(count);
    for (uint32_t i = 0; i < count; ++i) {
        state->edt_source[i] = mask[i] > 127 ? 0.0f : kEdtInfinity;
    }
    edt_2d(state, state->edt_source.data(), state->edt_distance.data(), size, size);
    for (uint32_t i = 0; i < count; ++i) {
        state->edt_source[i] = mask[i] > 127 ? kEdtInfinity : 0.0f;
    }
    edt_2d(state, state->edt_source.data(), state->edt_outside.data(), size, size);
    const float span = 2.0f * (float)padding;
    for (uint32_t i = 0; i < count; ++i) {
        const float inside = std::sqrt(state->edt_distance[i]);
        const float outside = std::sqrt(state->edt_outside[i]);
        const float value = 0.5f - (inside - outside) / span;
        out[i] = (uint8_t)(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
    }
}

void build_box_mask(pulse_font_plugin_state* state, uint32_t size, uint32_t padding) {
    state->raster_mask.assign((size_t)size * size, 0);
    const float base = (float)(size - padding * 2);
    const float pen_x0 = 0.1f * base;
    const float pen_y0 = -0.7f * base;
    const float pen_x1 = 0.7f * base;
    const float pen_y1 = -0.1f * base;
    const float stroke = std::max(1.0f, std::round(base / 16.0f));
    const float origin_x = pen_x0 - (float)padding;
    const float origin_y = pen_y0 - (float)padding;
    for (uint32_t v = 0; v < size; ++v) {
        for (uint32_t u = 0; u < size; ++u) {
            const float px = (float)u + origin_x;
            const float py = (float)v + origin_y;
            const bool outer = px >= pen_x0 && px <= pen_x1 && py >= pen_y0 && py <= pen_y1;
            const bool inner = px >= pen_x0 + stroke && px <= pen_x1 - stroke && py >= pen_y0 + stroke && py <= pen_y1 - stroke;
            if (outer && !inner) {
                state->raster_mask[(size_t)v * size + u] = 255;
            }
        }
    }
}

void rasterize_glyph_bitmap_font(pulse_font_plugin_state* state, glyph_entry& entry, const font_face& face, uint8_t* out, uint32_t size, uint32_t padding) {
    const bitmap_font_data& bm = *face.bitmap;
    const bitmap_glyph* glyph = bitmap_font_find_glyph(bm, entry.key.codepoint);
    const float base = (float)(size - padding * 2);
    const int32_t limit = (int32_t)base;
    float scale = base / (float)bm.line_height;
    int32_t width = (int32_t)((float)glyph->width * scale + 0.5f);
    int32_t height = (int32_t)((float)glyph->height * scale + 0.5f);
    float fit = 1.0f;
    if (width > limit || height > limit) {
        fit = std::min((float)limit / (float)width, (float)limit / (float)height);
        scale *= fit;
        width = (int32_t)((float)glyph->width * scale + 0.5f);
        height = (int32_t)((float)glyph->height * scale + 0.5f);
    }
    if (width < 1) {
        width = 1;
    }
    if (height < 1) {
        height = 1;
    }
    state->raster_mask.assign((size_t)size * size, 0);
    uint8_t* dest = state->raster_mask.data() + (size_t)padding * size + (size_t)padding;
    for (int32_t dy = 0; dy < height; ++dy) {
        const int32_t sy = (int32_t)((int64_t)dy * glyph->height / height);
        for (int32_t dx = 0; dx < width; ++dx) {
            const int32_t sx = (int32_t)((int64_t)dx * glyph->width / width);
            dest[(size_t)dy * size + dx] = bm.pixels[(size_t)(glyph->y + sy) * bm.scale_width + (glyph->x + sx)];
        }
    }
    build_sdf(state, state->raster_mask.data(), out, size, padding);
    const int32_t ix0 = (int32_t)std::lround((float)glyph->xoffset * scale);
    const int32_t iy0 = (int32_t)std::lround((float)(glyph->yoffset - bm.base) * scale);
    const float inverse = 1.0f / fit;
    entry.slot_size = size;
    entry.x0 = ((float)ix0 - (float)padding) * inverse;
    entry.y0 = ((float)iy0 - (float)padding) * inverse;
    entry.x1 = entry.x0 + (float)size * inverse;
    entry.y1 = entry.y0 + (float)size * inverse;
    entry.advance = font_advance_raw(face, entry.key.codepoint, base);
}

void rasterize_glyph_bitmap(pulse_font_plugin_state* state, glyph_entry& entry, uint8_t* out, uint32_t size, uint32_t padding) {
    const glyph_key& key = entry.key;
    const float base = (float)(size - padding * 2);
    if (key.font == kMissingGlyphFont) {
        build_box_mask(state, size, padding);
        build_sdf(state, state->raster_mask.data(), out, size, padding);
        entry.slot_size = size;
        entry.x0 = 0.1f * base - (float)padding;
        entry.y0 = -0.7f * base - (float)padding;
        entry.x1 = entry.x0 + (float)size;
        entry.y1 = entry.y0 + (float)size;
        entry.advance = 0.8f * base;
        return;
    }
    const font_face& face = state->fonts[key.font - 1];
    if (face.kind == kFontKindBitmap) {
        rasterize_glyph_bitmap_font(state, entry, face, out, size, padding);
        return;
    }
    const int32_t glyph = font_glyph_index(face, key.codepoint);
    float scale = stbtt_ScaleForMappingEmToPixels(&face.info, base);
    int32_t ix0 = 0;
    int32_t iy0 = 0;
    int32_t ix1 = 0;
    int32_t iy1 = 0;
    stbtt_GetGlyphBitmapBoxSubpixel(&face.info, glyph, scale, scale, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);
    float fit = 1.0f;
    const int32_t limit = (int32_t)base;
    if (ix1 - ix0 > limit || iy1 - iy0 > limit) {
        fit = std::min((float)limit / (float)(ix1 - ix0), (float)limit / (float)(iy1 - iy0));
        scale *= fit;
        stbtt_GetGlyphBitmapBoxSubpixel(&face.info, glyph, scale, scale, 0.0f, 0.0f, &ix0, &iy0, &ix1, &iy1);
    }
    const int32_t width = ix1 - ix0;
    const int32_t height = iy1 - iy0;
    state->raster_mask.assign((size_t)size * size, 0);
    if (width > 0 && height > 0) {
        uint8_t* dest = state->raster_mask.data() + (size_t)padding * size + (size_t)padding;
        stbtt_MakeGlyphBitmapSubpixel(&face.info, dest, width, height, (int)size, scale, scale, 0.0f, 0.0f, glyph);
    }
    build_sdf(state, state->raster_mask.data(), out, size, padding);
    const float inverse = 1.0f / fit;
    entry.slot_size = size;
    entry.x0 = ((float)ix0 - (float)padding) * inverse;
    entry.y0 = ((float)iy0 - (float)padding) * inverse;
    entry.x1 = entry.x0 + (float)size * inverse;
    entry.y1 = entry.y0 + (float)size * inverse;
    entry.advance = font_advance_raw(face, key.codepoint, base);
}

void reset_page_slots(pulse_font_plugin_state* state, atlas_page& page, uint32_t tier) {
    const uint32_t slot_size = (uint32_t)kTierBaseSizes[tier] + state->desc.sdf_padding * 2;
    page.tier = tier;
    page.slot_size = slot_size;
    page.grid_x = state->desc.atlas_width / slot_size;
    page.grid_y = state->desc.atlas_height / slot_size;
    std::fill(page.pixels.begin(), page.pixels.end(), 0);
    ++page.version;
    const uint32_t slot_count = page.grid_x * page.grid_y;
    page.slot_entries.assign(slot_count, kInvalidIndex);
    page.free_slots.clear();
    page.free_slots.reserve(slot_count);
    for (uint32_t i = slot_count; i > 0; --i) {
        page.free_slots.push_back(i - 1);
    }
}

uint32_t page_create(pulse_font_plugin_state* state, uint32_t tier) {
    const uint32_t slot_size = (uint32_t)kTierBaseSizes[tier] + state->desc.sdf_padding * 2;
    if (slot_size > state->desc.atlas_width || slot_size > state->desc.atlas_height) {
        return kInvalidIndex;
    }
    atlas_page page{};
    page.pixels.assign((size_t)state->desc.atlas_width * state->desc.atlas_height, 0);
    reset_page_slots(state, page, tier);
    state->pages.push_back(std::move(page));
    return (uint32_t)state->pages.size() - 1;
}

uint32_t page_evict_lru(pulse_font_plugin_state* state, uint32_t tier) {
    uint32_t victim = kInvalidIndex;
    uint64_t oldest = UINT64_MAX;
    for (uint32_t i = 0; i < state->pages.size(); ++i) {
        if (state->pages[i].last_used < oldest) {
            oldest = state->pages[i].last_used;
            victim = i;
        }
    }
    if (victim == kInvalidIndex) {
        return kInvalidIndex;
    }
    atlas_page& page = state->pages[victim];
    for (uint32_t entry_index : page.slot_entries) {
        if (entry_index == kInvalidIndex) {
            continue;
        }
        table_remove(state, state->entries[entry_index].key);
        free_entry(state, entry_index);
    }
    reset_page_slots(state, page, tier);
    page.last_used = ++state->tick;
    ++state->eviction_count;
    return victim;
}

}

const glyph_entry* atlas_find_glyph(pulse_font_plugin_state* state, const glyph_key& key) {
    const uint32_t index = table_find(state, key);
    if (index == kInvalidIndex) {
        return nullptr;
    }
    glyph_entry& entry = state->entries[index];
    entry.last_used = ++state->tick;
    if (entry.page != kInvalidIndex) {
        state->pages[entry.page].last_used = state->tick;
    }
    return &entry;
}

const glyph_entry* atlas_acquire_glyph(pulse_font_plugin_state* state, const glyph_key& key) {
    if (const glyph_entry* found = atlas_find_glyph(state, key)) {
        return found;
    }
    if (key.font != kMissingGlyphFont) {
        const font_face& face = state->fonts[key.font - 1];
        bool empty = false;
        if (face.kind == kFontKindBitmap) {
            const bitmap_glyph* glyph = bitmap_font_find_glyph(*face.bitmap, key.codepoint);
            if (!glyph) {
                return nullptr;
            }
            empty = glyph->width <= 0 || glyph->height <= 0;
        } else {
            const int32_t glyph = font_glyph_index(face, key.codepoint);
            if (glyph == 0) {
                return nullptr;
            }
            empty = stbtt_IsGlyphEmpty(&face.info, glyph) != 0;
        }
        if (empty) {
            const uint32_t empty_index = alloc_entry(state);
            glyph_entry& empty_entry = state->entries[empty_index];
            empty_entry.key = key;
            empty_entry.occupied = true;
            empty_entry.last_used = ++state->tick;
            empty_entry.advance = font_advance_raw(face, key.codepoint, (float)((uint32_t)kTierBaseSizes[key.tier]));
            table_insert(state, empty_index);
            return &empty_entry;
        }
    }
    const uint32_t entry_index = alloc_entry(state);
    uint32_t page_index = kInvalidIndex;
    for (uint32_t i = 0; i < state->pages.size(); ++i) {
        if (state->pages[i].tier == key.tier && !state->pages[i].free_slots.empty()) {
            page_index = i;
            break;
        }
    }
    if (page_index == kInvalidIndex) {
        if (state->pages.size() < state->desc.max_atlas_count) {
            page_index = page_create(state, key.tier);
        } else {
            page_index = page_evict_lru(state, key.tier);
        }
    }
    if (page_index == kInvalidIndex) {
        free_entry(state, entry_index);
        return nullptr;
    }
    atlas_page& page = state->pages[page_index];
    const uint32_t slot_index = page.free_slots.back();
    page.free_slots.pop_back();
    glyph_entry& entry = state->entries[entry_index];
    entry.key = key;
    entry.occupied = true;
    entry.last_used = ++state->tick;
    entry.page = page_index;
    entry.slot = slot_index;
    const uint32_t slot_size = page.slot_size;
    const uint32_t padding = state->desc.sdf_padding;
    const uint32_t slot_x = (slot_index % page.grid_x) * slot_size;
    const uint32_t slot_y = (slot_index / page.grid_x) * slot_size;
    page.slot_entries[slot_index] = entry_index;
    page.last_used = state->tick;
    ++state->rasterize_count;
    std::vector<uint8_t>& slot_pixels = state->slot_pixels;
    slot_pixels.assign((size_t)slot_size * slot_size, 0);
    rasterize_glyph_bitmap(state, entry, slot_pixels.data(), slot_size, padding);
    const uint32_t page_width = state->desc.atlas_width;
    for (uint32_t y = 0; y < slot_size; ++y) {
        uint8_t* dest = page.pixels.data() + (size_t)(slot_y + y) * page_width + slot_x;
        std::memcpy(dest, slot_pixels.data() + (size_t)y * slot_size, slot_size);
    }
    ++page.version;
    table_insert(state, entry_index);
    return &entry;
}

void atlas_purge_font(pulse_font_plugin_state* state, uint32_t font) {
    for (uint32_t i = 0; i < state->entries.size(); ++i) {
        glyph_entry& entry = state->entries[i];
        if (!entry.occupied || entry.key.font != font) {
            continue;
        }
        table_remove(state, entry.key);
        if (entry.page != kInvalidIndex) {
            atlas_page& page = state->pages[entry.page];
            page.slot_entries[entry.slot] = kInvalidIndex;
            page.free_slots.push_back(entry.slot);
        }
        free_entry(state, i);
    }
}

void atlas_shutdown(pulse_font_plugin_state* state) {
    state->entries.clear();
    state->free_entries.clear();
    state->table.clear();
    state->table_used = 0;
    state->table_occupied = 0;
    state->pages.clear();
}

}
