#include "test_common.h"

static const float kSize = 24.0f;

static bool color_eq(const PulseGlyphInstance& inst, float r, float g, float b, float a) {
    return fabsf(inst.r - r) < 1e-6f && fabsf(inst.g - g) < 1e-6f && fabsf(inst.b - b) < 1e-6f && fabsf(inst.a - a) < 1e-6f;
}

int main() {
    PulseAppId app = make_text_app("t-text-rich");
    const PulseFontHandle cjk = register_cjk(app);
    const uint32_t chain = make_chain(app, &cjk, 1);

    PulseTextBlockDesc desc = block_desc_for(chain, kSize);
    desc.color.a = 0.5f;
    const uint32_t block = pulse_text_block_create(app, &desc);
    assert(block != PULSE_TEXT_BLOCK_ID_NONE);

    PulseTextLayout* layout = pulse_text_block_layout(app, block, "[ff0000]red[/]black", 0.0f, 0.0f);
    assert(layout->line_count == 1);
    assert(layout->instances_count == 8);
    assert(color_eq(layout->p_instances[0], 1.0f, 0.0f, 0.0f, 0.5f));
    assert(color_eq(layout->p_instances[2], 1.0f, 0.0f, 0.0f, 0.5f));
    assert(color_eq(layout->p_instances[3], 1.0f, 1.0f, 1.0f, 0.5f));
    assert(color_eq(layout->p_instances[7], 1.0f, 1.0f, 1.0f, 0.5f));
    pulse_text_layout_free(app, layout);

    layout = pulse_text_block_layout(app, block, "[00FF00]ab[/]", 0.0f, 0.0f);
    assert(layout->instances_count == 2);
    assert(color_eq(layout->p_instances[0], 0.0f, 1.0f, 0.0f, 0.5f));
    assert(color_eq(layout->p_instances[1], 0.0f, 1.0f, 0.0f, 0.5f));
    pulse_text_layout_free(app, layout);

    layout = pulse_text_block_layout(app, block, "a[b]c", 0.0f, 0.0f);
    assert(layout->instances_count == 5);
    pulse_text_layout_free(app, layout);

    layout = pulse_text_block_layout(app, block, "[12g456]x", 0.0f, 0.0f);
    assert(layout->instances_count == 9);
    assert(color_eq(layout->p_instances[0], 1.0f, 1.0f, 1.0f, 0.5f));
    pulse_text_layout_free(app, layout);

    layout = pulse_text_block_layout(app, block, "[ff0000]", 0.0f, 0.0f);
    assert(layout->instances_count == 0);
    pulse_text_layout_free(app, layout);

    const PulseTextMeasure tagged = pulse_text_block_measure(app, block, "[ff0000]ab[/]", 0.0f);
    const PulseTextMeasure plain = pulse_text_block_measure(app, block, "ab", 0.0f);
    assert(fabsf(tagged.width - plain.width) < 1e-6f);
    assert(tagged.line_count == plain.line_count);

    pulse_text_block_destroy(app, block);
    pulse_font_destroy_chain(app, chain);
    pulse_destroy_app(app);
    printf("text rich text tests passed\n");
    return 0;
}
