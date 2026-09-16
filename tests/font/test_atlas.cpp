#include "test_common.h"

static bool same_rect(const PulseGlyph& a, const PulseGlyph& b) {
    return a.page == b.page && a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1 && a.u0 == b.u0 && a.v0 == b.v0 && a.u1 == b.u1 && a.v1 == b.v1;
}

int main() {
    PulseAppId app = make_font_app("t-font-atlas", nullptr);

    const uint32_t latin = register_latin(app);
    const uint32_t cjk = register_cjk(app);
    const uint32_t fonts[] = { latin, cjk };
    const uint32_t chain = make_chain(app, fonts, 2);

    const PulseGlyph glyph_48 = pulse_font_glyph(app, chain, 'A', 48.0f);
    assert(glyph_48.valid);
    assert(glyph_48.page == 0);
    assert(glyph_48.x1 > glyph_48.x0);
    assert(glyph_48.y1 > glyph_48.y0);
    assert(glyph_48.u1 > glyph_48.u0);
    assert(glyph_48.v1 > glyph_48.v0);
    assert(glyph_48.u0 >= 0.0f && glyph_48.u1 <= 1.0f);
    assert(glyph_48.v0 >= 0.0f && glyph_48.v1 <= 1.0f);
    assert(fabsf(glyph_48.advance - pulse_font_advance(app, chain, 'A', 48.0f)) < 1e-5f);

    const PulseGlyph glyph_48_again = pulse_font_glyph(app, chain, 'A', 48.0f);
    assert(same_rect(glyph_48, glyph_48_again));
    const PulseAtlasStats first_stats = pulse_font_atlas_stats(app);
    assert(first_stats.rasterize_count == 1);
    assert(first_stats.page_count == 1);
    assert(first_stats.glyph_count == 1);
    assert(first_stats.slot_count == 1024);

    const PulseGlyph glyph_24 = pulse_font_glyph(app, chain, 'A', 24.0f);
    assert(glyph_24.valid);
    assert(fabsf((glyph_24.x1 - glyph_24.x0) * 2.0f - (glyph_48.x1 - glyph_48.x0)) < 0.01f);
    assert(pulse_font_atlas_stats(app).rasterize_count == 1);

    const PulseGlyph glyph_96 = pulse_font_glyph(app, chain, 'A', 96.0f);
    assert(glyph_96.valid);
    assert(fabsf(glyph_96.advance - glyph_48.advance * 2.0f) < 1e-3f);
    assert((glyph_96.x1 - glyph_96.x0) > (glyph_48.x1 - glyph_48.x0));
    assert(pulse_font_atlas_stats(app).rasterize_count == 2);
    assert(pulse_font_atlas_stats(app).glyph_count == 2);
    assert(pulse_font_atlas_stats(app).page_count == 2);

    const PulseGlyph space = pulse_font_glyph(app, chain, ' ', 48.0f);
    assert(!space.valid);
    assert(space.advance > 0.0f);
    assert(pulse_font_atlas_stats(app).glyph_count == 2);

    const PulseGlyph missing = pulse_font_glyph(app, chain, 0x1FFFF, 48.0f);
    assert(missing.valid);
    assert(missing.page == 0);
    assert(fabsf(missing.advance - 48.0f * 0.8f) < 1e-4f);
    assert(pulse_font_atlas_stats(app).rasterize_count == 3);
    assert(pulse_font_atlas_stats(app).glyph_count == 3);

    pulse_font_prewarm(app, chain, "BCDEF", 48.0f);
    assert(pulse_font_atlas_stats(app).rasterize_count == 8);
    const PulseGlyph b = pulse_font_glyph(app, chain, 'B', 48.0f);
    assert(b.valid);
    assert(pulse_font_atlas_stats(app).rasterize_count == 8);

    pulse_font_prewarm(app, chain, "\xE4\xB8\xAD\xE6\x96\x87\xE5\x8D\xA1\xE7\x89\x8C", 48.0f);
    const PulseAtlasStats cjk_stats = pulse_font_atlas_stats(app);
    assert(cjk_stats.rasterize_count == 12);
    assert(cjk_stats.glyph_count == 12);
    assert(cjk_stats.eviction_count == 0);
    assert(cjk_stats.page_count == 2);

    pulse_destroy_app(app);

    PulseFontPluginDesc small = font_desc_for(256, 1);
    PulseAppId small_app = make_font_app("t-font-atlas-small", &small);
    const uint32_t small_latin = register_latin(small_app);
    const uint32_t small_chain = make_chain(small_app, &small_latin, 1);
    std::vector<uint32_t> codepoints;
    for (uint32_t c = 'A'; c <= 'Z'; ++c) {
        codepoints.push_back(c);
    }
    for (uint32_t c = 'a'; c <= 'z'; ++c) {
        codepoints.push_back(c);
    }
    for (uint32_t codepoint : codepoints) {
        const PulseGlyph glyph = pulse_font_glyph(small_app, small_chain, codepoint, 48.0f);
        assert(glyph.valid);
        assert(glyph.page == 0);
    }
    const PulseAtlasStats small_stats = pulse_font_atlas_stats(small_app);
    assert(small_stats.page_count == 1);
    assert(small_stats.slot_count == 16);
    assert(codepoints.size() == 52);
    assert(small_stats.rasterize_count == 52);
    assert(small_stats.eviction_count == 3);
    assert(small_stats.glyph_count == 4);
    assert(pulse_font_atlas_sample(small_app, 0, 0, 0) <= 255);
    assert(pulse_font_atlas_sample(small_app, 9, 0, 0) == 0);
    assert(pulse_font_atlas_sample(small_app, 0, 4096, 0) == 0);
    pulse_destroy_app(small_app);

    printf("font atlas tests passed\n");
    return 0;
}
