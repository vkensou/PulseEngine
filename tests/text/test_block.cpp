#include "test_common.h"

int main() {
    PulseAppId app = make_text_app("t-text-block");
    const PulseFontHandle latin = register_latin(app);
    const uint32_t chain = make_chain(app, &latin, 1);

    assert(pulse_text_block_create(app, nullptr) == PULSE_TEXT_BLOCK_ID_NONE);
    PulseTextBlockDesc desc = block_desc_for(chain, 24.0f);
    assert(pulse_text_block_create(app, &desc) != PULSE_TEXT_BLOCK_ID_NONE);
    desc.chain = PULSE_FONT_ID_NONE;
    assert(pulse_text_block_create(app, &desc) == PULSE_TEXT_BLOCK_ID_NONE);
    desc = block_desc_for(chain, 24.0f);
    desc.size = 0.0f;
    assert(pulse_text_block_create(app, &desc) == PULSE_TEXT_BLOCK_ID_NONE);
    desc = block_desc_for(chain, 24.0f);
    desc.align_h = PULSE_TEXT_ALIGN_H_COUNT;
    assert(pulse_text_block_create(app, &desc) == PULSE_TEXT_BLOCK_ID_NONE);

    const uint32_t block = make_block(app, chain, 24.0f);
    assert(pulse_text_block_create(app, &desc) == PULSE_TEXT_BLOCK_ID_NONE);
    desc.align_h = PULSE_TEXT_ALIGN_H_LEFT;
    const uint32_t second = pulse_text_block_create(app, &desc);
    assert(second != PULSE_TEXT_BLOCK_ID_NONE && second != block);

    assert(pulse_text_block_layout(app, 999, "x", 100.0f, 100.0f) == nullptr);
    assert(pulse_text_block_layout(app, block, nullptr, 100.0f, 100.0f) != nullptr);
    const PulseTextLayout* empty = pulse_text_block_layout(app, block, "", 100.0f, 100.0f);
    assert(empty->instances_count == 0 && empty->line_count == 0 && empty->height == 0.0f);
    pulse_text_layout_free(app, nullptr);
    pulse_text_layout_free(app, const_cast<PulseTextLayout*>(empty));

    const PulseTextMeasure zero = pulse_text_block_measure(app, 999, "x", 100.0f);
    assert(zero.width == 0.0f && zero.height == 0.0f && zero.line_count == 0);

    pulse_text_block_destroy(app, block);
    assert(pulse_text_block_layout(app, block, "x", 100.0f, 100.0f) == nullptr);
    pulse_text_block_destroy(app, block);
    const uint32_t reused = pulse_text_block_create(app, &desc);
    assert(reused == block);
    pulse_text_block_destroy(app, second);
    pulse_text_block_destroy(app, reused);

    pulse_font_destroy_chain(app, chain);
    pulse_destroy_app(app);
    printf("text block tests passed\n");
    return 0;
}
