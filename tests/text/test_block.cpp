#include "test_common.h"

int main() {
    PulseAppId app = make_text_app("t-text-block");
    const PulseFontHandle latin = register_latin(app);
    const uint32_t chain = make_chain(app, &latin, 1);

    assert(pulse_text_layout(app, nullptr, "x", 100.0f, 100.0f) == nullptr);

    PulseTextBlockDesc desc = text_desc(chain, 24.0f);
    desc.chain = PULSE_FONT_ID_NONE;
    assert(pulse_text_layout(app, &desc, "x", 100.0f, 100.0f) == nullptr);
    assert(pulse_text_measure(app, &desc, "x", 100.0f).line_count == 0);

    desc = text_desc(chain, 24.0f);
    desc.size = 0.0f;
    assert(pulse_text_layout(app, &desc, "x", 100.0f, 100.0f) == nullptr);
    assert(pulse_text_measure(app, &desc, "x", 100.0f).line_count == 0);

    desc = text_desc(chain, 24.0f);
    desc.align_h = PULSE_TEXT_ALIGN_H_COUNT;
    assert(pulse_text_layout(app, &desc, "x", 100.0f, 100.0f) == nullptr);
    assert(pulse_text_measure(app, &desc, "x", 100.0f).line_count == 0);

    desc = text_desc(chain, 24.0f);
    desc.align_v = PULSE_TEXT_ALIGN_V_COUNT;
    assert(pulse_text_layout(app, &desc, "x", 100.0f, 100.0f) == nullptr);
    assert(pulse_text_measure(app, &desc, "x", 100.0f).line_count == 0);

    desc = text_desc(chain, 24.0f);
    const PulseTextLayout* valid = pulse_text_layout(app, &desc, "x", 100.0f, 100.0f);
    assert(valid != nullptr);
    pulse_text_layout_free(app, const_cast<PulseTextLayout*>(valid));

    PulseTextLayout* layout = pulse_text_layout(app, &desc, nullptr, 100.0f, 100.0f);
    assert(layout != nullptr);
    assert(layout->instances_count == 0 && layout->line_count == 0 && layout->height == 0.0f);
    pulse_text_layout_free(app, layout);

    layout = pulse_text_layout(app, &desc, "", 100.0f, 100.0f);
    assert(layout->instances_count == 0 && layout->line_count == 0 && layout->height == 0.0f);
    pulse_text_layout_free(app, nullptr);
    pulse_text_layout_free(app, layout);

    const PulseTextMeasure measure = pulse_text_measure(app, &desc, "x", 100.0f);
    assert(measure.line_count == 1 && measure.width > 0.0f && measure.height > 0.0f);

    pulse_font_destroy_chain(app, chain);
    pulse_destroy_app(app);
    printf("text block tests passed\n");
    return 0;
}
