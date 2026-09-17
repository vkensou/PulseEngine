#pragma once

#include "pulse_font.h"
#include "pulse_graphics.h"

#include "stb_truetype.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace pulse_font_internal {

constexpr uint32_t kTierCount = 2;
constexpr float kTierBaseSizes[kTierCount] = { 48.0f, 96.0f };
constexpr uint32_t kMaxChains = 256;
constexpr uint32_t kMaxPendingDraws = 256;
constexpr uint32_t kInvalidIndex = 0xFFFFFFFFu;
constexpr uint32_t kMissingGlyphFont = 0;
constexpr uint32_t kMissingGlyphCodepoint = 0xFFFFFFFFu;

enum font_kind : uint32_t { kFontKindTtf = 0, kFontKindBitmap = 1 };

struct bitmap_glyph {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t xoffset = 0;
    int32_t yoffset = 0;
    int32_t xadvance = 0;
};

struct bitmap_kerning {
    uint64_t pair = 0;
    int32_t amount = 0;
};

struct bitmap_font_data {
    std::string family;
    uint32_t line_height = 0;
    int32_t base = 0;
    uint32_t scale_width = 0;
    uint32_t scale_height = 0;
    std::vector<uint8_t> pixels;
    std::vector<uint32_t> codepoints;
    std::vector<bitmap_glyph> glyphs;
    std::vector<bitmap_kerning> kernings;
};

const bitmap_glyph* bitmap_font_find_glyph(const bitmap_font_data& data, uint32_t codepoint);
int32_t bitmap_font_kerning(const bitmap_font_data& data, uint32_t first, uint32_t second);

struct bitmap_font_parse_result {
    bitmap_font_data data;
    std::string page_file;
};

bool bitmap_font_parse(const uint8_t* fnt, size_t fnt_size, bitmap_font_parse_result& out, const char** out_error);
bool bitmap_font_attach_page(bitmap_font_data& data, const uint8_t* png, size_t png_size, const char** out_error);
bool bitmap_font_read_file(const char* path, std::vector<uint8_t>& out);
std::string bitmap_font_page_path(const char* fnt_path, const std::string& page_file);

const std::vector<uint8_t>& default_font_fnt_bytes();
const std::vector<uint8_t>& default_font_png_bytes();

struct font_face {
    font_kind kind = kFontKindTtf;
    stbtt_fontinfo info{};
    const bitmap_font_data* bitmap = nullptr;
    std::string family;
    PulseAssetHandle asset{};
    bool occupied = false;
};

struct font_asset_impl {
    font_kind kind = kFontKindTtf;
    std::vector<uint8_t> bytes;
    stbtt_fontinfo info{};
    std::unique_ptr<bitmap_font_data> bitmap;
    std::string family;
};

struct PulseFontAssetData {
    font_asset_impl* impl = nullptr;
    PulseAssetHandle self{};
};

struct PulseFontLoadSettings {
    uint32_t face_index = 0;
};

struct font_chain_slot {
    std::vector<uint32_t> fonts;
    bool alive = false;
};

struct glyph_key {
    uint32_t font = 0;
    uint32_t codepoint = 0;
    uint32_t tier = 0;
};

struct glyph_entry {
    glyph_key key{};
    uint32_t page = kInvalidIndex;
    uint32_t slot = kInvalidIndex;
    uint32_t slot_size = 0;
    float x0 = 0.0f;
    float y0 = 0.0f;
    float x1 = 0.0f;
    float y1 = 0.0f;
    float advance = 0.0f;
    bool occupied = false;
    uint64_t last_used = 0;
};

struct atlas_page {
    uint32_t tier = 0;
    uint32_t grid_x = 0;
    uint32_t grid_y = 0;
    uint32_t slot_size = 0;
    uint64_t last_used = 0;
    uint64_t version = 0;
    std::vector<uint8_t> pixels;
    std::vector<uint32_t> free_slots;
    std::vector<uint32_t> slot_entries;
};

struct font_page_gpu {
    PulseTextureRequest request{};
    PulseTextureHandle handle{};
    bool requested = false;
    bool ready = false;
    uint64_t last_uploaded_version = 0;
};

struct font_draw_record {
    PulseTransform transform{};
    PulseScissor scissor{};
    uint32_t first = 0;
    uint32_t count = 0;
};

struct font_pass_group {
    uint32_t page;
    uint32_t first;
    uint32_t count;
};

struct font_record_groups {
    uint32_t first;
    uint32_t count;
};

struct font_render_state {
    bool initialized = false;
    PulseShaderHandle shader{};
    PulseMaterialHandle material{};
    PulseMeshHandle mesh{};
    PulseSamplerHandle sampler{};
    ecs_query_t* window_query = nullptr;
    std::vector<font_page_gpu> pages;
    std::vector<PulseGlyphInstance> instances;
    std::vector<font_draw_record> records;
    std::vector<font_pass_group> groups;
    std::vector<font_record_groups> record_groups;
    std::vector<uint8_t> gpu_instances;
    bool overflow_reported = false;
};

struct pulse_font_plugin_state {
    PulseAppId app = nullptr;
    PulseFontPluginDesc desc{};
    std::vector<font_face> fonts;
    std::vector<uint32_t> free_fonts;
    uint32_t default_font = PULSE_FONT_ID_NONE;
    std::vector<font_chain_slot> chains;
    std::vector<uint32_t> free_chains;
    std::vector<glyph_entry> entries;
    std::vector<uint32_t> free_entries;
    std::vector<uint32_t> table;
    uint32_t table_used = 0;
    uint32_t table_occupied = 0;
    std::vector<atlas_page> pages;
    std::vector<float> edt_distance;
    std::vector<float> edt_outside;
    std::vector<float> edt_source;
    std::vector<float> edt_column_in;
    std::vector<float> edt_column_out;
    std::vector<int32_t> edt_parabola;
    std::vector<float> edt_boundary;
    std::vector<uint8_t> raster_mask;
    std::vector<uint8_t> slot_pixels;
    uint64_t tick = 0;
    uint64_t eviction_count = 0;
    uint64_t rasterize_count = 0;
    font_render_state render;
};

struct pulse_font_state_resource {
    pulse_font_plugin_state* state;
};

extern ECS_COMPONENT_DECLARE(pulse_font_state_resource);

pulse_font_plugin_state* state_from_app(PulseAppId app);
pulse_font_plugin_state* state_from_world(ecs_world_t* world);

EPulsePluginBuildResult pulse_font_plugin_build(PulseAppId app, void* ctx);
EPulsePluginBuildResult pulse_font_plugin_post_build(PulseAppId app, void* ctx);
void pulse_font_plugin_shutdown(PulseAppId app, void* ctx);

float font_scale_for_size(const font_face& face, float size);
int32_t font_glyph_index(const font_face& face, uint32_t codepoint);
float font_advance_raw(const font_face& face, uint32_t codepoint, float size);
float font_kerning_raw(const font_face& face, uint32_t first, uint32_t second, float size);
uint32_t font_tier_for_size(float size);
uint32_t font_occupied_count(pulse_font_plugin_state* state);
uint32_t font_slot_of(pulse_font_plugin_state* state, PulseFontHandle handle);
PulseFontHandle font_handle_of(pulse_font_plugin_state* state, uint32_t font);

void register_font_type(PulseAssetSystemId asset_system, PulseAppId app);
void register_font_loaders(PulseAssetSystemId asset_system);
void font_build_default_font(pulse_font_plugin_state* state);
uint32_t font_register_asset(pulse_font_plugin_state* state, PulseAssetHandle handle);
void font_registry_release(pulse_font_plugin_state* state, PulseAssetHandle handle);

const glyph_entry* atlas_find_glyph(pulse_font_plugin_state* state, const glyph_key& key);
const glyph_entry* atlas_acquire_glyph(pulse_font_plugin_state* state, const glyph_key& key);
void atlas_purge_font(pulse_font_plugin_state* state, uint32_t font);
void atlas_shutdown(pulse_font_plugin_state* state);

void render_ensure_page(pulse_font_plugin_state* state, uint32_t page);
EPulseResult render_init(pulse_font_plugin_state* state);
void render_shutdown(pulse_font_plugin_state* state);

}
