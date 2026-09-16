#include "test_common.h"

static int kerned_pair_count(PulseAppId app, uint32_t chain, float size) {
    const char* pairs[] = { "AV", "To", "Wa", "Yo", "LT", "PA", "F,", "ry" };
    int count = 0;
    for (const char* pair : pairs) {
        if (pulse_font_kerning(app, chain, (uint32_t)pair[0], (uint32_t)pair[1], size) < -0.0001f) {
            ++count;
        }
    }
    return count;
}

int main() {
    PulseAppId app = make_font_app("t-font-registry", nullptr);

    const uint32_t latin = register_latin(app);
    const uint32_t cjk = register_cjk(app);
    assert(latin == 1);
    assert(cjk == 2);
    assert(pulse_font_count(app) == 2);

    const char* latin_family = pulse_font_family_name(app, latin);
    const char* cjk_family = pulse_font_family_name(app, cjk);
    assert(latin_family && latin_family[0]);
    assert(cjk_family && cjk_family[0]);
    assert(strcmp(latin_family, cjk_family) != 0);
    assert(pulse_font_find_family(app, latin_family) == latin);
    assert(pulse_font_find_family(app, cjk_family) == cjk);
    assert(pulse_font_find_family(app, "NoSuchFamilyName") == PULSE_FONT_ID_NONE);
    assert(pulse_font_family_name(app, 42) == nullptr);

    const std::vector<uint8_t> latin_bytes = read_test_file("tests/font/data/latin.ttf");
    assert(pulse_font_face_count(app, latin_bytes.data(), latin_bytes.size()) == 1);
    uint8_t junk[32];
    memset(junk, 0x5A, sizeof(junk));
    assert(pulse_font_face_count(app, junk, sizeof(junk)) == 0);
    assert(pulse_font_register(app, junk, sizeof(junk), 0) == PULSE_FONT_ID_NONE);
    assert(pulse_font_count(app) == 2);

    const uint32_t from_file = pulse_font_register_file(app, "latin.ttf", 0);
    assert(from_file == 3);
    assert(pulse_font_register_file(app, "latin.ttf", 1) == PULSE_FONT_ID_NONE);
    assert(pulse_font_register_file(app, "missing.ttf", 0) == PULSE_FONT_ID_NONE);
    assert(pulse_font_count(app) == 3);

    const uint32_t latin_first[] = { latin, cjk };
    const uint32_t cjk_first[] = { cjk, latin };
    const uint32_t latin_chain = make_chain(app, latin_first, 2);
    const uint32_t cjk_chain = make_chain(app, cjk_first, 2);
    assert(latin_chain == 1);
    assert(cjk_chain == 2);

    const uint32_t han = 0x4E2D;
    assert(pulse_font_resolve_codepoint(app, latin_chain, 'A') == latin);
    assert(pulse_font_resolve_codepoint(app, cjk_chain, 'A') == cjk);
    assert(pulse_font_resolve_codepoint(app, latin_chain, han) == cjk);
    assert(pulse_font_resolve_codepoint(app, cjk_chain, han) == cjk);
    assert(pulse_font_resolve_codepoint(app, latin_chain, 0x1FFFF) == PULSE_FONT_ID_NONE);
    assert(pulse_font_resolve_codepoint(app, 99, 'A') == PULSE_FONT_ID_NONE);

    const float advance_48 = pulse_font_advance(app, latin_chain, 'A', 48.0f);
    const float advance_96 = pulse_font_advance(app, latin_chain, 'A', 96.0f);
    const float space_48 = pulse_font_advance(app, latin_chain, ' ', 48.0f);
    assert(advance_48 > 0.0f);
    assert(space_48 > 0.0f);
    assert(fabsf(advance_96 - advance_48 * 2.0f) < 1e-3f);
    assert(pulse_font_advance(app, latin_chain, han, 48.0f) > 0.0f);
    assert(fabsf(pulse_font_advance(app, latin_chain, 0x1FFFF, 48.0f) - 48.0f * 0.8f) < 1e-4f);

    const uint32_t single_chain = make_chain(app, &from_file, 1);
    assert(single_chain == 3);
    assert(fabsf(pulse_font_advance(app, single_chain, 'A', 48.0f) - advance_48) < 1e-4f);

    assert(kerned_pair_count(app, latin_chain, 48.0f) > 0);
    assert(pulse_font_kerning(app, cjk_chain, (uint32_t)'A', (uint32_t)'V', 48.0f) <= 0.0f);
    assert(pulse_font_kerning(app, latin_chain, han, (uint32_t)'A', 48.0f) == 0.0f);
    assert(pulse_font_kerning(app, 99, (uint32_t)'A', (uint32_t)'V', 48.0f) == 0.0f);

    const PulseVerticalMetrics metrics = pulse_font_vertical_metrics(app, latin_chain, 48.0f);
    assert(metrics.ascent > 0.0f);
    assert(metrics.descent < 0.0f);
    assert(metrics.height > 0.0f);
    assert(fabsf(metrics.height - (metrics.ascent - metrics.descent + metrics.line_gap)) < 1e-4f);
    assert(metrics.height > 48.0f * 0.8f && metrics.height < 48.0f * 2.0f);
    const PulseVerticalMetrics metrics_96 = pulse_font_vertical_metrics(app, latin_chain, 96.0f);
    assert(fabsf(metrics_96.ascent - metrics.ascent * 2.0f) < 1e-3f);
    const PulseVerticalMetrics empty_metrics = pulse_font_vertical_metrics(app, 99, 48.0f);
    assert(empty_metrics.height == 0.0f);

    pulse_font_destroy_chain(app, latin_chain);
    assert(pulse_font_resolve_codepoint(app, latin_chain, 'A') == PULSE_FONT_ID_NONE);
    const uint32_t reused_chain = make_chain(app, latin_first, 2);
    assert(reused_chain == latin_chain);

    pulse_destroy_app(app);
    printf("font registry tests passed\n");
    return 0;
}
