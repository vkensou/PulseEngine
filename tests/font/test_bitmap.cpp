#include "test_common.h"

int main() {
    PulseAppId app = make_font_app("t-font-bitmap", nullptr);
    const std::vector<uint8_t> fnt_bytes = read_test_file("tests/font/data/ProggyVector.fnt");

    const PulseFontRequest bad_face_request = pulse_font_load_from_memory(app, "badface.fnt", fnt_bytes.data(), fnt_bytes.size(), 1);
    pump_font_request(app, bad_face_request);
    assert(!pulse_font_is_ready(app, bad_face_request));
    assert(pulse_font_get_error(app, bad_face_request) != nullptr);
    assert(pulse_font_count(app) == 1);

    const PulseFontRequest request = pulse_font_load(app, "ProggyVector.fnt", 0);
    pump_font_request(app, request);
    if (!pulse_font_is_ready(app, request)) {
        printf("bitmap load error: %s\n", pulse_font_get_error(app, request));
    }
    assert(pulse_font_is_ready(app, request));
    const PulseFontHandle bitmap = pulse_font_get_handle(app, request);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(bitmap)));
    assert(pulse_font_count(app) == 2);
    const char* family = pulse_font_family_name(app, bitmap);
    assert(family && family[0]);

    uint8_t junk[16];
    memset(junk, 'x', sizeof(junk));
    const PulseFontRequest junk_request = pulse_font_load_from_memory(app, "junk.fnt", junk, sizeof(junk), 0);
    pump_font_request(app, junk_request);
    assert(!pulse_font_is_ready(app, junk_request));
    assert(pulse_font_get_error(app, junk_request) != nullptr);
    assert(pulse_font_count(app) == 2);

    const PulseFontRequest mem_request = pulse_font_load_from_memory(app, "ProggyVector.fnt", fnt_bytes.data(), fnt_bytes.size(), 0);
    pump_font_request(app, mem_request);
    assert(pulse_font_is_ready(app, mem_request));
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_get_handle(app, mem_request)), pulse_font_to_handle(bitmap)));
    assert(pulse_font_count(app) == 2);

    const uint32_t chain = make_chain(app, &bitmap, 1);

    const float adv_48 = pulse_font_advance(app, chain, 'A', 48.0f);
    const float adv_96 = pulse_font_advance(app, chain, 'A', 96.0f);
    assert(adv_48 > 0.0f);
    assert(fabsf(adv_48 - 48.0f * 8.0f / 14.0f) < 1e-3f);
    assert(fabsf(adv_96 - adv_48 * 2.0f) < 1e-3f);

    assert(pulse_font_kerning(app, chain, 'T', 'a', 48.0f) < -1.0f);
    assert(pulse_font_kerning(app, chain, 'a', 'T', 48.0f) == 0.0f);
    assert(pulse_font_kerning(app, chain, 'Y', '.', 48.0f) < -1.0f);

    const PulseVerticalMetrics metrics = pulse_font_vertical_metrics(app, chain, 48.0f);
    assert(fabsf(metrics.ascent - 48.0f * 11.0f / 14.0f) < 1e-3f);
    assert(fabsf(metrics.descent + 48.0f * 3.0f / 14.0f) < 1e-3f);
    assert(metrics.line_gap == 0.0f);
    assert(fabsf(metrics.height - 48.0f) < 1e-3f);

    const PulseAtlasStats before = pulse_font_atlas_stats(app);
    const PulseGlyph glyph = pulse_font_glyph(app, chain, 'A', 48.0f);
    assert(glyph.valid);
    assert(pulse_font_atlas_stats(app).rasterize_count == before.rasterize_count + 1);
    assert(fabsf(glyph.x1 - glyph.x0 - 64.0f) < 1e-4f);
    assert(glyph.y0 < 0.0f);
    const uint32_t origin_x = slot_origin_x(glyph, 2048);
    const uint32_t origin_y = slot_origin_y(glyph, 2048);
    uint32_t covered = 0;
    for (uint32_t y = 0; y < 64; ++y) {
        for (uint32_t x = 0; x < 64; ++x) {
            if (pulse_font_atlas_sample(app, glyph.page, origin_x + x, origin_y + y) > 128) {
                ++covered;
            }
        }
    }
    assert(covered > 32);
    assert(pulse_font_atlas_sample(app, glyph.page, origin_x, origin_y) < 128);
    assert(pulse_font_atlas_sample(app, glyph.page, origin_x + 63, origin_y + 63) < 128);

    const PulseGlyph glyph_small = pulse_font_glyph(app, chain, 'A', 24.0f);
    assert(glyph_small.valid);
    assert(fabsf(glyph_small.advance - adv_48 * 0.5f) < 1e-4f);

    const PulseGlyph glyph_96 = pulse_font_glyph(app, chain, 'A', 96.0f);
    assert(glyph_96.valid);
    assert(pulse_font_atlas_stats(app).rasterize_count == before.rasterize_count + 2);
    assert(fabsf((glyph_96.x1 - glyph_96.x0) - 112.0f) < 1e-3f);
    assert(glyph_96.page != glyph.page);

    const PulseFontHandle def = pulse_font_default(app);
    assert(!pulse_asset_handle_equals(pulse_font_to_handle(def), pulse_font_to_handle(bitmap)));
    const uint32_t han_chain = make_chain(app, &bitmap, 1);
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_resolve_codepoint(app, han_chain, 'A')), pulse_font_to_handle(bitmap)));
    assert(!pulse_asset_handle_is_valid(pulse_font_to_handle(pulse_font_resolve_codepoint(app, han_chain, 0x4E2D))));
    const PulseGlyph tofu = pulse_font_glyph(app, han_chain, 0x4E2D, 48.0f);
    assert(tofu.valid);
    assert(fabsf(tofu.advance - 48.0f * 0.8f) < 1e-4f);

    pulse_font_destroy_chain(app, chain);
    pulse_font_destroy_chain(app, han_chain);
    unload_font(app, bitmap);
    assert(pulse_font_count(app) == 1);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(pulse_font_default(app))));
    const uint32_t after_chain = make_chain(app, &def, 1);
    assert(pulse_font_glyph(app, after_chain, 'x', 32.0f).valid);
    pulse_font_destroy_chain(app, after_chain);

    pulse_destroy_app(app);
    printf("font bitmap tests passed\n");
    return 0;
}
