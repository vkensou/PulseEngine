#include "test_common.h"

static const uint8_t kExpectedRow[64] = { 0, 16, 32, 48, 64, 80, 96, 112, 143, 159, 143, 112, 96, 80, 64, 48, 32, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 32, 48, 64, 80, 96, 112, 143, 159, 143, 112, 96, 80, 64, 48, 32, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static const uint8_t kExpectedColumn[64] = { 0, 16, 32, 48, 64, 80, 96, 112, 143, 159, 143, 112, 96, 80, 64, 48, 32, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 16, 32, 48, 64, 80, 96, 112, 143, 159, 143, 112, 96, 80, 64, 48, 32, 16, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

int main() {
    PulseAppId app = make_font_app("t-font-sdf", nullptr);

    const uint32_t latin = register_latin(app);
    const uint32_t chain = make_chain(app, &latin, 1);

    const PulseGlyph box = pulse_font_glyph(app, chain, 0x1FFFF, 48.0f);
    assert(box.valid);
    assert(box.page == 0);
    assert(fabsf(box.x0 - (-3.2f)) < 1e-4f);
    assert(fabsf(box.y0 - (-41.6f)) < 1e-4f);
    assert(fabsf(box.x1 - 60.8f) < 1e-4f);
    assert(fabsf(box.y1 - 22.4f) < 1e-4f);

    const uint32_t origin_x = slot_origin_x(box, 2048);
    const uint32_t origin_y = slot_origin_y(box, 2048);
    assert(origin_x == 0);
    assert(origin_y == 0);

    for (uint32_t i = 0; i < 64; ++i) {
        const uint8_t row = pulse_font_atlas_sample(app, box.page, origin_x + i, origin_y + 22);
        const uint8_t column = pulse_font_atlas_sample(app, box.page, origin_x + 22, origin_y + i);
        if (row != kExpectedRow[i] || column != kExpectedColumn[i]) {
            printf("sdf mismatch at %u: row=%u expected=%u column=%u expected=%u\n", i, (unsigned)row, (unsigned)kExpectedRow[i], (unsigned)column, (unsigned)kExpectedColumn[i]);
        }
        assert(row == kExpectedRow[i]);
        assert(column == kExpectedColumn[i]);
    }

    for (uint32_t i = 11; i < 18; ++i) {
        const int32_t outer = pulse_font_atlas_sample(app, box.page, origin_x + i, origin_y + 22);
        const int32_t inner = pulse_font_atlas_sample(app, box.page, origin_x + i + 1, origin_y + 22);
        assert(outer - inner == 16);
    }

    assert(pulse_font_atlas_sample(app, box.page, origin_x + 10, origin_y + 22) > 128);
    assert(pulse_font_atlas_sample(app, box.page, origin_x + 11, origin_y + 22) < 128);
    assert(pulse_font_atlas_sample(app, box.page, origin_x + 35, origin_y + 22) > 128);
    assert(pulse_font_atlas_sample(app, box.page, origin_x + 37, origin_y + 22) < 128);
    assert(pulse_font_atlas_sample(app, box.page, origin_x + 22, origin_y + 9) > 128);
    assert(pulse_font_atlas_sample(app, box.page, origin_x + 22, origin_y + 11) < 128);

    pulse_destroy_app(app);
    printf("font sdf tests passed\n");
    return 0;
}
