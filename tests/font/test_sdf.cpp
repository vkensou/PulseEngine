#include "test_common.h"

static const uint32_t kAtlas = 2048;
static const uint32_t kPadding = 8;

static uint32_t slot_size(uint32_t tier_base) {
    return tier_base + kPadding * 2;
}

static void assert_slot_uv(const PulseGlyph& glyph, uint32_t tier_base) {
    const uint32_t slot = slot_size(tier_base);
    assert(fabsf(glyph.u1 - glyph.u0 - (float)(slot - 1) / (float)kAtlas) < 1e-6f);
    assert(fabsf(glyph.v1 - glyph.v0 - (float)(slot - 1) / (float)kAtlas) < 1e-6f);
    assert(slot_origin_x(glyph, kAtlas) % slot == 0);
    assert(slot_origin_y(glyph, kAtlas) % slot == 0);
}

static bool same_slot(const PulseGlyph& a, const PulseGlyph& b) {
    return a.page == b.page && slot_origin_x(a, kAtlas) == slot_origin_x(b, kAtlas) && slot_origin_y(a, kAtlas) == slot_origin_y(b, kAtlas);
}

int main() {
    PulseAppId app = make_font_app("t-font-sdf", nullptr);

    const PulseFontHandle latin = register_latin(app);
    const PulseFontChainHandle chain = make_chain(app, &latin, 1);

    const PulseGlyph box = pulse_font_glyph(app, chain, 0x1FFFF, 48.0f);
    assert(box.valid);
    assert(box.page == 0);
    assert(fabsf(box.x0 - (-3.2f)) < 1e-4f);
    assert(fabsf(box.y0 - (-41.6f)) < 1e-4f);
    assert(fabsf(box.x1 - 60.8f) < 1e-4f);
    assert(fabsf(box.y1 - 22.4f) < 1e-4f);
    assert(fabsf(box.x1 - box.x0 - (float)slot_size(48)) < 1e-4f);
    assert(fabsf(box.y1 - box.y0 - (float)slot_size(48)) < 1e-4f);
    assert(fabsf(box.advance - 48.0f * 0.8f) < 1e-4f);
    assert_slot_uv(box, 48);
    assert(slot_origin_x(box, kAtlas) == 0);
    assert(slot_origin_y(box, kAtlas) == 0);

    const PulseGlyph latin_a = pulse_font_glyph(app, chain, 'A', 48.0f);
    assert(latin_a.valid);
    assert(latin_a.page == 0);
    assert_slot_uv(latin_a, 48);
    assert(!same_slot(latin_a, box));
    assert(slot_origin_x(latin_a, kAtlas) == slot_size(48));
    assert(slot_origin_y(latin_a, kAtlas) == 0);

    const PulseGlyph box_again = pulse_font_glyph(app, chain, 0x1FFFF, 48.0f);
    assert(box_again.valid);
    assert(same_slot(box_again, box));

    const PulseGlyph box_96 = pulse_font_glyph(app, chain, 0x1FFFF, 96.0f);
    assert(box_96.valid);
    assert(box_96.page == 1);
    assert(fabsf(box_96.x0 - 1.6f) < 1e-4f);
    assert(fabsf(box_96.y0 - (-75.2f)) < 1e-4f);
    assert(fabsf(box_96.x1 - 113.6f) < 1e-4f);
    assert(fabsf(box_96.y1 - 36.8f) < 1e-4f);
    assert(fabsf(box_96.advance - 96.0f * 0.8f) < 1e-4f);
    assert_slot_uv(box_96, 96);
    assert(slot_origin_x(box_96, kAtlas) == 0);
    assert(slot_origin_y(box_96, kAtlas) == 0);

    pulse_font_destroy_chain(app, chain);
    pulse_destroy_app(app);
    printf("font sdf tests passed\n");
    return 0;
}
