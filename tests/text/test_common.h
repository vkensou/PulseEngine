#pragma once

#undef NDEBUG
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <vector>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_font.h"
#include "pulse_text.h"
#include "pulse_vfs.h"

static PulseAppId make_text_app(const char* name) {
    PulseAppDesc app_desc = {
        .name = name,
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_vfs_mount("tests/font/data", "/", false));
    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_font_plugin(app, nullptr) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_text_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    return app;
}

static std::vector<uint8_t> read_test_file(const char* path) {
    FILE* file = fopen(path, "rb");
    assert(file);
    fseek(file, 0, SEEK_END);
    const long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert(size > 0);
    std::vector<uint8_t> data((size_t)size);
    assert(fread(data.data(), 1, (size_t)size, file) == (size_t)size);
    fclose(file);
    return data;
}

static void pump_font_request(PulseAppId app, PulseFontRequest request) {
    while (!pulse_font_is_ready(app, request) && pulse_font_is_alive(app, request)) {
        assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    }
}

static PulseFontHandle load_font_memory(PulseAppId app, const char* name, const std::vector<uint8_t>& bytes) {
    const PulseFontRequest request = pulse_font_load_from_memory(app, name, bytes.data(), bytes.size(), 0);
    pump_font_request(app, request);
    assert(pulse_font_is_ready(app, request));
    const PulseFontHandle font = pulse_font_get_handle(app, request);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(font)));
    return font;
}

static PulseFontHandle register_latin(PulseAppId app) {
    return load_font_memory(app, "latin.ttf", read_test_file("tests/font/data/latin.ttf"));
}

static PulseFontHandle register_cjk(PulseAppId app) {
    return load_font_memory(app, "cjk.ttf", read_test_file("tests/font/data/cjk.ttf"));
}

static uint32_t make_chain(PulseAppId app, const PulseFontHandle* fonts, size_t count) {
    const uint32_t chain = pulse_font_create_chain(app, fonts, count);
    assert(chain != PULSE_FONT_ID_NONE);
    return chain;
}

static void decode_utf8(const char* text, std::vector<uint32_t>& out) {
    out.clear();
    const uint8_t* cursor = reinterpret_cast<const uint8_t*>(text);
    while (*cursor) {
        uint32_t code = *cursor;
        uint32_t extra = 0;
        if (code >= 0xF0) {
            code &= 0x07;
            extra = 3;
        } else if (code >= 0xE0) {
            code &= 0x0F;
            extra = 2;
        } else if (code >= 0xC0) {
            code &= 0x1F;
            extra = 1;
        }
        ++cursor;
        for (uint32_t i = 0; i < extra && (*cursor & 0xC0) == 0x80; ++i) {
            code = (code << 6) | (*cursor & 0x3F);
            ++cursor;
        }
        out.push_back(code);
    }
}

static float run_width(PulseAppId app, uint32_t chain, const char* utf8, float size) {
    std::vector<uint32_t> cps;
    decode_utf8(utf8, cps);
    float width = 0.0f;
    for (size_t i = 0; i < cps.size(); ++i) {
        if (i > 0) {
            width += pulse_font_kerning(app, chain, cps[i - 1], cps[i], size);
        }
        width += pulse_font_advance(app, chain, cps[i], size);
    }
    return width;
}

static PulseTextBlockDesc block_desc_for(uint32_t chain, float size) {
    PulseTextBlockDesc desc{};
    desc.chain = chain;
    desc.size = size;
    desc.color.r = 1.0f;
    desc.color.g = 1.0f;
    desc.color.b = 1.0f;
    desc.color.a = 1.0f;
    desc.align_h = PULSE_TEXT_ALIGN_H_LEFT;
    desc.align_v = PULSE_TEXT_ALIGN_V_TOP;
    desc.line_height = 0.0f;
    return desc;
}

static uint32_t make_block(PulseAppId app, uint32_t chain, float size) {
    const PulseTextBlockDesc desc = block_desc_for(chain, size);
    const uint32_t block = pulse_text_block_create(app, &desc);
    assert(block != PULSE_TEXT_BLOCK_ID_NONE);
    return block;
}

static float line_advance(PulseAppId app, uint32_t chain, float size, float scale) {
    const PulseVerticalMetrics metrics = pulse_font_vertical_metrics(app, chain, size);
    const float natural = metrics.ascent - metrics.descent + metrics.line_gap;
    return scale > 0.0f ? natural * scale : natural;
}
