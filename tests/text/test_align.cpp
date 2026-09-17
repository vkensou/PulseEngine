#include "test_common.h"

#include <algorithm>

static const float kSize = 24.0f;

static uint32_t make_block_with(PulseAppId app, uint32_t chain, EPulseTextAlignH align_h, EPulseTextAlignV align_v, float line_height) {
    PulseTextBlockDesc desc = block_desc_for(chain, kSize);
    desc.align_h = align_h;
    desc.align_v = align_v;
    desc.line_height = line_height;
    const uint32_t block = pulse_text_block_create(app, &desc);
    assert(block != PULSE_TEXT_BLOCK_ID_NONE);
    return block;
}

int main() {
    PulseAppId app = make_text_app("t-text-align");
    const PulseFontHandle cjk = register_cjk(app);
    const uint32_t chain = make_chain(app, &cjk, 1);
    const float la = line_advance(app, chain, kSize, 0.0f);

    const float w_ab = run_width(app, chain, "ab", kSize);
    const float w_cd = run_width(app, chain, "cd", kSize);
    const float box_w = run_width(app, chain, "ab ", kSize);
    const PulseGlyph glyph_a = pulse_font_glyph(app, chain, 'a', kSize);
    const PulseGlyph glyph_c = pulse_font_glyph(app, chain, 'c', kSize);

    const uint32_t block_center = make_block_with(app, chain, PULSE_TEXT_ALIGN_H_CENTER, PULSE_TEXT_ALIGN_V_TOP, 0.0f);
    PulseTextLayout* centered = pulse_text_block_layout(app, block_center, "ab cd", box_w, 0.0f);
    assert(centered->line_count == 2);
    assert(fabsf(centered->p_instances[0].x - ((box_w - w_ab) * 0.5f + glyph_a.x0)) < 1e-3f);
    assert(fabsf(centered->p_instances[2].x - ((box_w - w_cd) * 0.5f + glyph_c.x0)) < 1e-3f);
    pulse_text_layout_free(app, centered);

    const uint32_t block_right = make_block_with(app, chain, PULSE_TEXT_ALIGN_H_RIGHT, PULSE_TEXT_ALIGN_V_TOP, 0.0f);
    PulseTextLayout* righted = pulse_text_block_layout(app, block_right, "ab cd", box_w, 0.0f);
    assert(fabsf(righted->p_instances[0].x - (box_w - w_ab + glyph_a.x0)) < 1e-3f);
    assert(fabsf(righted->p_instances[2].x - (box_w - w_cd + glyph_c.x0)) < 1e-3f);
    pulse_text_layout_free(app, righted);

    const float no_wrap_max = std::max(w_ab, w_cd);
    PulseTextLayout* natural_centered = pulse_text_block_layout(app, block_center, "ab\ncd", 0.0f, 0.0f);
    assert(natural_centered->line_count == 2);
    assert(fabsf(natural_centered->p_instances[0].x - ((no_wrap_max - w_ab) * 0.5f + glyph_a.x0)) < 1e-3f);
    pulse_text_layout_free(app, natural_centered);
    pulse_text_block_destroy(app, block_center);
    pulse_text_block_destroy(app, block_right);

    const float ascent = pulse_font_vertical_metrics(app, chain, kSize).ascent;
    const uint32_t block_bottom = make_block_with(app, chain, PULSE_TEXT_ALIGN_H_LEFT, PULSE_TEXT_ALIGN_V_BOTTOM, 0.0f);
    PulseTextLayout* bottomed = pulse_text_block_layout(app, block_bottom, "ab", box_w, la * 2.0f);
    assert(bottomed->line_count == 1 && !bottomed->out_of_box);
    assert(fabsf(bottomed->p_instances[0].y - (la * 2.0f - la + ascent + glyph_a.y0)) < 1e-3f);
    pulse_text_layout_free(app, bottomed);
    pulse_text_block_destroy(app, block_bottom);

    const uint32_t block_middle = make_block_with(app, chain, PULSE_TEXT_ALIGN_H_LEFT, PULSE_TEXT_ALIGN_V_MIDDLE, 0.0f);
    PulseTextLayout* middled = pulse_text_block_layout(app, block_middle, "ab", box_w, la * 4.0f);
    assert(fabsf(middled->p_instances[0].y - ((la * 4.0f - la) * 0.5f + ascent + glyph_a.y0)) < 1e-3f);
    pulse_text_layout_free(app, middled);
    pulse_text_block_destroy(app, block_middle);

    const uint32_t block_top = make_block_with(app, chain, PULSE_TEXT_ALIGN_H_LEFT, PULSE_TEXT_ALIGN_V_TOP, 1.5f);
    const PulseTextMeasure scaled = pulse_text_block_measure(app, block_top, "a\nb", 0.0f);
    assert(scaled.line_count == 2);
    assert(fabsf(scaled.height - la * 3.0f) < 1e-3f);
    const float la15 = line_advance(app, chain, kSize, 1.5f);
    PulseTextLayout* scaled_layout = pulse_text_block_layout(app, block_top, "a\nb\nc", 0.0f, la15 * 2.5f);
    assert(scaled_layout->out_of_box);
    assert(scaled_layout->line_count == 2);
    assert(fabsf(scaled_layout->height - la15 * 2.0f) < 1e-3f);
    const uint32_t block_plain = make_block_with(app, chain, PULSE_TEXT_ALIGN_H_LEFT, PULSE_TEXT_ALIGN_V_TOP, 1.5f);
    pulse_text_layout_free(app, scaled_layout);
    PulseTextLayout* exact_fit = pulse_text_block_layout(app, block_plain, "a\nb\nc", 0.0f, la15 * 3.0f);
    assert(!exact_fit->out_of_box);
    assert(exact_fit->line_count == 3);
    pulse_text_layout_free(app, exact_fit);
    pulse_text_block_destroy(app, block_top);
    pulse_text_block_destroy(app, block_plain);

    const uint32_t block_one = make_block_with(app, chain, PULSE_TEXT_ALIGN_H_LEFT, PULSE_TEXT_ALIGN_V_TOP, 0.0f);
    PulseTextLayout* clipped = pulse_text_block_layout(app, block_one, "a\nb\nc", 0.0f, la * 1.5f);
    assert(clipped->out_of_box);
    assert(clipped->line_count == 1);
    const PulseTextMeasure unclipped = pulse_text_block_measure(app, block_one, "a\nb\nc", 0.0f);
    assert(unclipped.line_count == 3);
    assert(fabsf(unclipped.height - la * 3.0f) < 1e-3f);
    pulse_text_layout_free(app, clipped);
    pulse_text_block_destroy(app, block_one);

    pulse_font_destroy_chain(app, chain);
    pulse_destroy_app(app);
    printf("text align tests passed\n");
    return 0;
}
