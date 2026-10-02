#include "test_common.h"

static const float kSize = 48.0f;

static void rasterize(PulseAppId app, PulseFontChainHandle chain, const char* text) {
    std::vector<uint32_t> codepoints;
    decode_utf8(text, codepoints);
    for (uint32_t codepoint : codepoints) {
        assert(pulse_font_glyph(app, chain, codepoint, kSize).valid);
    }
}

static std::vector<uint32_t> codepoints_of(const char* text) {
    std::vector<uint32_t> out;
    decode_utf8(text, out);
    return out;
}

static bool layout_matches_atlas(PulseAppId app, PulseFontChainHandle chain, const char* text, const PulseTextLayout* layout) {
    const std::vector<uint32_t> codepoints = codepoints_of(text);
    if (layout->instances_count != codepoints.size()) {
        return false;
    }
    for (size_t i = 0; i < codepoints.size(); ++i) {
        const PulseGlyph glyph = pulse_font_glyph(app, chain, codepoints[i], kSize);
        const PulseGlyphInstance& instance = layout->p_instances[i];
        if (!glyph.valid) {
            return false;
        }
        if (glyph.page != instance.page || glyph.u0 != instance.u0 || glyph.v0 != instance.v0 || glyph.u1 != instance.u1 || glyph.v1 != instance.v1) {
            return false;
        }
    }
    return true;
}

int main() {
    PulseFontPluginDesc desc = font_desc_for(256, 1);
    PulseAppId app = make_text_app("t-text-atlas-generation", &desc);
    const PulseFontHandle latin = register_latin(app);
    const PulseFontChainHandle chain = make_chain(app, &latin, 1);
    const PulseTextBlockDesc block = text_desc(chain, kSize);

    PulseTextLayout* layout = pulse_text_layout(app, &block, "ABCDE", 0.0f, 0.0f);
    assert(layout->instances_count == 5);
    assert(pulse_font_atlas_generation(app) == 0);
    assert(layout_matches_atlas(app, chain, "ABCDE", layout));

    rasterize(app, chain, "FGHIJKLMNOP");
    assert(pulse_font_page_count(app) == 1);
    assert(pulse_font_atlas_generation(app) == 0);

    rasterize(app, chain, "Q");
    const uint64_t evicted = pulse_font_atlas_generation(app);
    assert(evicted == 1);

    assert(!layout_matches_atlas(app, chain, "ABCDE", layout));
    assert(pulse_font_atlas_generation(app) == evicted);

    PulseTextLayout* relaid = pulse_text_layout(app, &block, "ABCDE", 0.0f, 0.0f);
    assert(relaid->instances_count == 5);
    assert(layout_matches_atlas(app, chain, "ABCDE", relaid));
    assert(pulse_font_atlas_generation(app) == evicted);

    rasterize(app, chain, "R");
    assert(pulse_font_atlas_generation(app) == evicted);

    pulse_text_layout_free(app, layout);
    pulse_text_layout_free(app, relaid);
    pulse_font_destroy_chain(app, chain);
    pulse_destroy_app(app);
    printf("text atlas generation tests passed\n");
    return 0;
}
