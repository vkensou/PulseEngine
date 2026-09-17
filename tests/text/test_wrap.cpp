#include "test_common.h"

#include <algorithm>

static const float kSize = 24.0f;

int main() {
    PulseAppId app = make_text_app("t-text-wrap");
    const PulseFontHandle cjk = register_cjk(app);
    const uint32_t chain = make_chain(app, &cjk, 1);
    const uint32_t block = make_block(app, chain, kSize);
    const float la = line_advance(app, chain, kSize, 0.0f);

    const PulseTextMeasure natural = pulse_text_block_measure(app, block, "Hello world", 0.0f);
    assert(natural.line_count == 1);
    assert(fabsf(natural.width - run_width(app, chain, "Hello world", kSize)) < 1e-3f);
    assert(fabsf(natural.height - la) < 1e-3f);

    const float hello_space = run_width(app, chain, "Hello ", kSize);
    const float box_w = run_width(app, chain, "world", kSize) * 1.01f;
    assert(box_w > hello_space);

    const PulseTextMeasure wrapped = pulse_text_block_measure(app, block, "Hello world", box_w);
    assert(wrapped.line_count == 2);
    assert(fabsf(wrapped.width - run_width(app, chain, "world", kSize)) < 1e-3f);
    assert(fabsf(wrapped.height - la * 2.0f) < 1e-3f);

    PulseTextLayout* layout = pulse_text_block_layout(app, block, "Hello world", box_w, 0.0f);
    assert(layout->line_count == 2);
    assert(layout->instances_count == 10);
    assert(!layout->out_of_box);
    assert(fabsf(layout->width - wrapped.width) < 1e-3f);
    const PulseGlyph first_w = pulse_font_glyph(app, chain, 'w', kSize);
    assert(fabsf(layout->p_instances[5].x - first_w.x0) < 1e-3f);
    const float baseline1 = pulse_font_vertical_metrics(app, chain, kSize).ascent + la;
    assert(fabsf(layout->p_instances[5].y - (baseline1 + first_w.y0)) < 1e-3f);
    pulse_text_layout_free(app, layout);

    const float aaa_w = run_width(app, chain, "aaa", kSize);
    const PulseTextMeasure overlong = pulse_text_block_measure(app, block, "aaaaaaaaaa", aaa_w);
    assert(overlong.line_count == 4);
    assert(fabsf(overlong.width - aaa_w) < 1e-3f);

    const float cjk3 = run_width(app, chain, "中中中", kSize);
    const PulseTextMeasure cjk_wrap = pulse_text_block_measure(app, block, "中中中中中中", cjk3 * 1.1f);
    assert(cjk_wrap.line_count == 2);
    assert(fabsf(cjk_wrap.width - cjk3) < 1e-3f);
    const PulseTextLayout* cjk_layout = pulse_text_block_layout(app, block, "中中中中中中", cjk3 * 1.1f, 0.0f);
    assert(cjk_layout->instances_count == 6);
    assert(cjk_layout->line_count == 2);
    pulse_text_layout_free(app, const_cast<PulseTextLayout*>(cjk_layout));

    const PulseTextMeasure explicit_break = pulse_text_block_measure(app, block, "ab\ncd", 0.0f);
    assert(explicit_break.line_count == 2);
    assert(fabsf(explicit_break.width - std::max(run_width(app, chain, "ab", kSize), run_width(app, chain, "cd", kSize))) < 1e-3f);
    const PulseTextMeasure crlf = pulse_text_block_measure(app, block, "a\r\nb", 0.0f);
    assert(crlf.line_count == 2);
    const PulseTextMeasure trailing_newline = pulse_text_block_measure(app, block, "ab\n", 0.0f);
    assert(trailing_newline.line_count == 2);
    const PulseTextMeasure empty = pulse_text_block_measure(app, block, "", 100.0f);
    assert(empty.line_count == 0 && empty.width == 0.0f && empty.height == 0.0f);

    const float ab_w = run_width(app, chain, "ab", kSize);
    const PulseTextLayout* trimmed = pulse_text_block_layout(app, block, "ab ab", run_width(app, chain, "ab ", kSize), 0.0f);
    assert(trimmed->line_count == 2);
    assert(trimmed->instances_count == 4);
    assert(fabsf(trimmed->width - ab_w) < 1e-3f);
    pulse_text_layout_free(app, const_cast<PulseTextLayout*>(trimmed));

    const PulseTextMeasure kerned = pulse_text_block_measure(app, block, "AV", 0.0f);
    assert(fabsf(kerned.width - run_width(app, chain, "AV", kSize)) < 1e-3f);

    const PulseTextMeasure fallback = pulse_text_block_measure(app, block, "中A", 0.0f);
    assert(fabsf(fallback.width - run_width(app, chain, "中A", kSize)) < 1e-3f);
    const PulseTextLayout* emoji_layout = pulse_text_block_layout(app, block, "\xF0\x9F\x98\x80!", 0.0f, 0.0f);
    assert(emoji_layout->line_count == 1);
    assert(emoji_layout->instances_count == 2);
    pulse_text_layout_free(app, const_cast<PulseTextLayout*>(emoji_layout));

    pulse_text_block_destroy(app, block);
    pulse_font_destroy_chain(app, chain);
    pulse_destroy_app(app);
    printf("text wrap tests passed\n");
    return 0;
}
