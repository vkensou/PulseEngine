#include "test_common.h"

static bool same_rect(const PulseGlyph& a, const PulseGlyph& b) {
    return a.page == b.page && a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1 && a.u0 == b.u0 && a.v0 == b.v0 && a.u1 == b.u1 && a.v1 == b.v1;
}

static bool same_slot(const PulseGlyph& a, const PulseGlyph& b) {
    return a.page == b.page && a.u0 == b.u0 && a.v0 == b.v0;
}

static void rasterize(PulseAppId app, PulseFontChainHandle chain, const char* text, float size) {
    for (const char* cursor = text; *cursor; ++cursor) {
        const PulseGlyph glyph = pulse_font_glyph(app, chain, (uint8_t)*cursor, size);
        assert(glyph.valid);
    }
}

int main() {
    PulseAppId app = make_font_app("t-font-atlas", nullptr);

    const PulseFontHandle latin = register_latin(app);
    const PulseFontHandle cjk = register_cjk(app);
    const PulseFontHandle fonts[] = { latin, cjk };
    const PulseFontChainHandle chain = make_chain(app, fonts, 2);

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
    assert(pulse_font_page_count(app) == 1);

    const PulseGlyph glyph_24 = pulse_font_glyph(app, chain, 'A', 24.0f);
    assert(glyph_24.valid);
    assert(same_slot(glyph_24, glyph_48));
    assert(fabsf((glyph_24.x1 - glyph_24.x0) * 2.0f - (glyph_48.x1 - glyph_48.x0)) < 0.01f);

    const PulseGlyph glyph_96 = pulse_font_glyph(app, chain, 'A', 96.0f);
    assert(glyph_96.valid);
    assert(fabsf(glyph_96.advance - glyph_48.advance * 2.0f) < 1e-3f);
    assert((glyph_96.x1 - glyph_96.x0) > (glyph_48.x1 - glyph_48.x0));
    assert(pulse_font_page_count(app) == 2);
    assert(glyph_96.page == 1);

    const PulseGlyph space = pulse_font_glyph(app, chain, ' ', 48.0f);
    assert(!space.valid);
    assert(space.advance > 0.0f);

    const PulseGlyph missing = pulse_font_glyph(app, chain, 0x1FFFF, 48.0f);
    assert(missing.valid);
    assert(missing.page == 0);
    assert(fabsf(missing.advance - 48.0f * 0.8f) < 1e-4f);
    assert(!same_slot(missing, glyph_48));

    rasterize(app, chain, "BCDEF", 48.0f);
    const PulseGlyph b = pulse_font_glyph(app, chain, 'B', 48.0f);
    assert(b.valid);
    assert(!same_slot(b, glyph_48));
    assert(same_rect(b, pulse_font_glyph(app, chain, 'B', 48.0f)));

    const uint32_t cjk_text[] = { 0x4E2D, 0x6587, 0x5361, 0x7247 };
    for (uint32_t codepoint : cjk_text) {
        const PulseGlyph glyph = pulse_font_glyph(app, chain, codepoint, 48.0f);
        assert(glyph.valid);
        assert(!same_slot(glyph, glyph_48));
    }
    assert(pulse_font_page_count(app) == 2);

    assert(pulse_font_page_count(app) == 2);
    rasterize(app, chain, "G", 48.0f);
    assert(same_rect(glyph_48, pulse_font_glyph(app, chain, 'A', 48.0f)));
    assert(pulse_font_atlas_generation(app) == 0);

    pulse_destroy_app(app);

    PulseFontPluginDesc small = font_desc_for(256, 1);
    PulseAppId small_app = make_font_app("t-font-atlas-small", &small);
    const PulseFontHandle small_latin = register_latin(small_app);
    const PulseFontChainHandle small_chain = make_chain(small_app, &small_latin, 1);
    std::vector<uint32_t> codepoints;
    for (uint32_t c = 'A'; c <= 'Z'; ++c) {
        codepoints.push_back(c);
    }
    for (uint32_t c = 'a'; c <= 'z'; ++c) {
        codepoints.push_back(c);
    }
    const uint32_t slot_count = (256 / (48 + 8 * 2)) * (256 / (48 + 8 * 2));
    assert(slot_count == 16);
    const PulseGlyph first = pulse_font_glyph(small_app, small_chain, codepoints[0], 48.0f);
    assert(first.valid);
    assert(first.page == 0);
    for (uint32_t i = 0; i < slot_count; ++i) {
        const PulseGlyph glyph = pulse_font_glyph(small_app, small_chain, codepoints[i], 48.0f);
        assert(glyph.valid);
        assert(glyph.page == 0);
        assert(pulse_font_atlas_generation(small_app) == 0);
    }
    for (uint32_t i = slot_count; i < codepoints.size(); ++i) {
        const PulseGlyph glyph = pulse_font_glyph(small_app, small_chain, codepoints[i], 48.0f);
        assert(glyph.valid);
        assert(glyph.page == 0);
        assert(pulse_font_atlas_generation(small_app) == i / slot_count);
    }
    assert(pulse_font_page_count(small_app) == 1);
    assert(codepoints.size() == 52);
    assert(pulse_font_atlas_generation(small_app) == 3);

    const uint64_t generation = pulse_font_atlas_generation(small_app);
    const PulseGlyph resident = pulse_font_glyph(small_app, small_chain, codepoints.back(), 48.0f);
    assert(resident.valid);
    assert(same_rect(resident, pulse_font_glyph(small_app, small_chain, codepoints.back(), 48.0f)));
    assert(pulse_font_atlas_generation(small_app) == generation);

    const PulseGlyph rerun = pulse_font_glyph(small_app, small_chain, codepoints[0], 48.0f);
    assert(rerun.valid);
    assert(!same_slot(rerun, first));
    assert(pulse_font_atlas_generation(small_app) == generation);

    pulse_destroy_app(small_app);

    printf("font atlas tests passed\n");
    return 0;
}
