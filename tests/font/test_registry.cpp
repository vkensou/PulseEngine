#include "test_common.h"

static int kerned_pair_count(PulseAppId app, PulseFontChainHandle chain, float size) {
    const char* pairs[] = { "AV", "To", "Wa", "Yo", "LT", "PA", "F,", "ry" };
    int count = 0;
    for (const char* pair : pairs) {
        if (pulse_font_kerning(app, chain, (uint32_t)pair[0], (uint32_t)pair[1], size) < -0.0001f) {
            ++count;
        }
    }
    return count;
}

static bool is_missing_advance(PulseAppId app, PulseFontChainHandle chain, uint32_t codepoint, float size) {
    return fabsf(pulse_font_advance(app, chain, codepoint, size) - size * 0.8f) < 1e-4f;
}

int main() {
    PulseAppId app = make_font_app("t-font-registry", nullptr);

    const PulseFontHandle default_font = pulse_font_default(app);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(default_font)));

    const PulseFontHandle latin = register_latin(app);
    const PulseFontHandle cjk = register_cjk(app);
    assert(latin.index != cjk.index);

    const std::vector<uint8_t> latin_bytes = read_test_file("tests/font/data/latin.ttf");
    const PulseFontRequest dup_request = pulse_font_load_from_memory(app, "latin.ttf", latin_bytes.data(), latin_bytes.size(), 0);
    pump_font_request(app, dup_request);
    assert(pulse_font_is_ready(app, dup_request));
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_get_handle(app, dup_request)), pulse_font_to_handle(latin)));

    uint8_t junk[32];
    memset(junk, 0x5A, sizeof(junk));
    const PulseFontRequest junk_request = pulse_font_load_from_memory(app, "junk.ttf", junk, sizeof(junk), 0);
    pump_font_request(app, junk_request);
    assert(!pulse_font_is_ready(app, junk_request));
    assert(pulse_font_get_error(app, junk_request) != nullptr);
    assert(!pulse_asset_handle_is_valid(pulse_font_to_handle(pulse_font_get_handle(app, junk_request))));

    const PulseFontHandle latin_first[] = { latin, cjk };
    const PulseFontHandle cjk_first[] = { cjk, latin };
    const PulseFontChainHandle latin_chain = make_chain(app, latin_first, 2);
    const PulseFontChainHandle cjk_chain = make_chain(app, cjk_first, 2);
    assert(latin_chain.index != cjk_chain.index);
    const PulseFontHandle stale_font{ latin.index + 100, latin.generation };
    const PulseFontHandle stale_chain_font[] = { latin, stale_font };
    assert(pulse_font_create_chain(app, stale_chain_font, 2).index == 0);
    const PulseFontChainHandle bogus_chain{ 99, 1 };

    const uint32_t han = 0x4E2D;
    const PulseFontChainHandle latin_ref = make_chain(app, &latin, 1);
    const PulseFontChainHandle cjk_ref = make_chain(app, &cjk, 1);
    assert(fabsf(pulse_font_advance(app, latin_ref, 'A', 48.0f) - pulse_font_advance(app, cjk_ref, 'A', 48.0f)) > 1e-3f);
    assert(fabsf(pulse_font_advance(app, latin_chain, 'A', 48.0f) - pulse_font_advance(app, latin_ref, 'A', 48.0f)) < 1e-5f);
    assert(fabsf(pulse_font_advance(app, cjk_chain, 'A', 48.0f) - pulse_font_advance(app, cjk_ref, 'A', 48.0f)) < 1e-5f);
    assert(!is_missing_advance(app, cjk_ref, han, 48.0f));
    assert(is_missing_advance(app, latin_ref, han, 48.0f));
    assert(fabsf(pulse_font_advance(app, latin_chain, han, 48.0f) - pulse_font_advance(app, cjk_ref, han, 48.0f)) < 1e-5f);
    assert(fabsf(pulse_font_advance(app, cjk_chain, han, 48.0f) - pulse_font_advance(app, cjk_ref, han, 48.0f)) < 1e-5f);
    assert(pulse_font_advance(app, bogus_chain, 'A', 48.0f) == 0.0f);

    const float advance_48 = pulse_font_advance(app, latin_chain, 'A', 48.0f);
    const float advance_96 = pulse_font_advance(app, latin_chain, 'A', 96.0f);
    const float space_48 = pulse_font_advance(app, latin_chain, ' ', 48.0f);
    assert(advance_48 > 0.0f);
    assert(space_48 > 0.0f);
    assert(fabsf(advance_96 - advance_48 * 2.0f) < 1e-3f);
    assert(pulse_font_advance(app, latin_chain, han, 48.0f) > 0.0f);
    assert(fabsf(pulse_font_advance(app, latin_chain, 0x1FFFF, 48.0f) - 48.0f * 0.8f) < 1e-4f);

    const PulseFontHandle single_font[] = { latin };
    const PulseFontChainHandle single_chain = make_chain(app, single_font, 1);
    assert(single_chain.index != latin_chain.index);
    assert(fabsf(pulse_font_advance(app, single_chain, 'A', 48.0f) - advance_48) < 1e-4f);

    assert(kerned_pair_count(app, latin_chain, 48.0f) > 0);
    assert(pulse_font_kerning(app, cjk_chain, (uint32_t)'A', (uint32_t)'V', 48.0f) <= 0.0f);
    assert(pulse_font_kerning(app, latin_chain, han, (uint32_t)'A', 48.0f) == 0.0f);
    assert(pulse_font_kerning(app, bogus_chain, (uint32_t)'A', (uint32_t)'V', 48.0f) == 0.0f);

    const PulseVerticalMetrics metrics = pulse_font_vertical_metrics(app, latin_chain, 48.0f);
    assert(metrics.ascent > 0.0f);
    assert(metrics.descent < 0.0f);
    assert(metrics.height > 0.0f);
    assert(fabsf(metrics.height - (metrics.ascent - metrics.descent + metrics.line_gap)) < 1e-4f);
    assert(metrics.height > 48.0f * 0.8f && metrics.height < 48.0f * 2.0f);
    const PulseVerticalMetrics metrics_96 = pulse_font_vertical_metrics(app, latin_chain, 96.0f);
    assert(fabsf(metrics_96.ascent - metrics.ascent * 2.0f) < 1e-3f);
    const PulseVerticalMetrics empty_metrics = pulse_font_vertical_metrics(app, bogus_chain, 48.0f);
    assert(empty_metrics.height == 0.0f);

    pulse_font_destroy_chain(app, latin_chain);
    assert(pulse_font_advance(app, latin_chain, 'A', 48.0f) == 0.0f);
    const PulseFontChainHandle recreated_chain = make_chain(app, latin_first, 2);
    assert(fabsf(pulse_font_advance(app, recreated_chain, 'A', 48.0f) - advance_48) < 1e-4f);

    pulse_font_destroy_chain(app, recreated_chain);
    pulse_font_destroy_chain(app, single_chain);
    pulse_font_destroy_chain(app, cjk_chain);
    pulse_font_destroy_chain(app, latin_ref);
    pulse_font_destroy_chain(app, cjk_ref);
    unload_font(app, latin);
    const PulseFontHandle latin_only[] = { latin };
    assert(pulse_font_create_chain(app, latin_only, 1).index == 0);
    unload_font(app, cjk);
    assert(!pulse_asset_handle_is_valid(pulse_font_to_handle(pulse_font_get_handle(app, dup_request))));

    const PulseFontHandle reloaded_latin = register_latin(app);
    const PulseFontHandle reloaded_cjk = register_cjk(app);
    assert(!pulse_asset_handle_equals(pulse_font_to_handle(reloaded_latin), pulse_font_to_handle(latin)));
    const PulseFontChainHandle rechain = make_chain(app, &reloaded_latin, 1);
    assert(fabsf(pulse_font_advance(app, rechain, 'A', 48.0f) - advance_48) < 1e-4f);
    assert(is_missing_advance(app, rechain, han, 48.0f));
    pulse_font_destroy_chain(app, rechain);

    const PulseFontChainHandle cjk_only_chain = make_chain(app, &reloaded_cjk, 1);
    assert(!is_missing_advance(app, cjk_only_chain, 'A', 48.0f));
    assert(!is_missing_advance(app, cjk_only_chain, 'n', 48.0f));
    assert(!is_missing_advance(app, cjk_only_chain, han, 48.0f));
    pulse_font_destroy_chain(app, cjk_only_chain);

    const PulseFontChainHandle default_chain = make_chain(app, &default_font, 1);
    assert(pulse_font_glyph(app, default_chain, 'A', 24.0f).valid);
    assert(pulse_font_glyph(app, default_chain, 'z', 48.0f).valid);
    assert(pulse_font_glyph(app, default_chain, '0', 96.0f).valid);
    assert(!is_missing_advance(app, default_chain, 'A', 48.0f));
    assert(is_missing_advance(app, default_chain, han, 48.0f));
    const PulseGlyph default_tofu = pulse_font_glyph(app, default_chain, han, 48.0f);
    assert(default_tofu.valid);
    assert(fabsf(default_tofu.advance - 48.0f * 0.8f) < 1e-4f);
    pulse_font_destroy_chain(app, default_chain);

    pulse_destroy_app(app);
    printf("font registry tests passed\n");
    return 0;
}
