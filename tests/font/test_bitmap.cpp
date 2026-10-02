#include "test_common.h"

int main() {
    PulseAppId app = make_font_app("t-font-bitmap", nullptr);
    const std::vector<uint8_t> fnt_bytes = read_test_file("tests/font/data/ProggyVector.fnt");

    const PulseFontRequest bad_face_request = pulse_font_load_from_memory(app, "badface.fnt", fnt_bytes.data(), fnt_bytes.size(), 1);
    pump_font_request(app, bad_face_request);
    assert(!pulse_font_is_ready(app, bad_face_request));
    assert(pulse_font_get_error(app, bad_face_request) != nullptr);

    const PulseFontRequest request = pulse_font_load(app, "ProggyVector.fnt", 0);
    pump_font_request(app, request);
    if (!pulse_font_is_ready(app, request)) {
        printf("bitmap load error: %s\n", pulse_font_get_error(app, request));
    }
    assert(pulse_font_is_ready(app, request));
    const PulseFontHandle bitmap = pulse_font_get_handle(app, request);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(bitmap)));

    uint8_t junk[16];
    memset(junk, 'x', sizeof(junk));
    const PulseFontRequest junk_request = pulse_font_load_from_memory(app, "junk.fnt", junk, sizeof(junk), 0);
    pump_font_request(app, junk_request);
    assert(!pulse_font_is_ready(app, junk_request));
    assert(pulse_font_get_error(app, junk_request) != nullptr);

    const PulseFontRequest mem_request = pulse_font_load_from_memory(app, "ProggyVector.fnt", fnt_bytes.data(), fnt_bytes.size(), 0);
    pump_font_request(app, mem_request);
    assert(pulse_font_is_ready(app, mem_request));
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_get_handle(app, mem_request)), pulse_font_to_handle(bitmap)));

    const PulseFontChainHandle chain = make_chain(app, &bitmap, 1);

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

    assert(pulse_font_page_count(app) == 0);
    const PulseGlyph glyph = pulse_font_glyph(app, chain, 'A', 48.0f);
    assert(glyph.valid);
    assert(pulse_font_page_count(app) == 1);
    assert(fabsf(glyph.x1 - glyph.x0 - 64.0f) < 1e-4f);
    assert(glyph.y0 < 0.0f);
    assert(slot_origin_x(glyph, 2048) == 0);
    assert(slot_origin_y(glyph, 2048) == 0);

    const PulseGlyph glyph_small = pulse_font_glyph(app, chain, 'A', 24.0f);
    assert(glyph_small.valid);
    assert(fabsf(glyph_small.advance - adv_48 * 0.5f) < 1e-4f);

    const PulseGlyph glyph_96 = pulse_font_glyph(app, chain, 'A', 96.0f);
    assert(glyph_96.valid);
    assert(fabsf((glyph_96.x1 - glyph_96.x0) - 112.0f) < 1e-3f);
    assert(glyph_96.page != glyph.page);
    assert(pulse_font_page_count(app) == 2);

    const PulseFontHandle def = pulse_font_default(app);
    assert(!pulse_asset_handle_equals(pulse_font_to_handle(def), pulse_font_to_handle(bitmap)));
    const PulseFontChainHandle han_chain = make_chain(app, &bitmap, 1);
    const PulseGlyph tofu = pulse_font_glyph(app, han_chain, 0x4E2D, 48.0f);
    assert(tofu.valid);
    assert(fabsf(tofu.advance - 48.0f * 0.8f) < 1e-4f);

    pulse_font_destroy_chain(app, chain);
    pulse_font_destroy_chain(app, han_chain);
    const uint64_t generation_before_unload = pulse_font_atlas_generation(app);
    unload_font(app, bitmap);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_font_atlas_generation(app) == generation_before_unload + 1);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(pulse_font_default(app))));
    const PulseFontChainHandle after_chain = make_chain(app, &def, 1);
    assert(pulse_font_glyph(app, after_chain, 'x', 32.0f).valid);
    pulse_font_destroy_chain(app, after_chain);

    pulse_destroy_app(app);
    printf("font bitmap tests passed\n");
    return 0;
}
