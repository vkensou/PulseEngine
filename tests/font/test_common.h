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
#include "pulse_vfs.h"

static PulseFontPluginDesc font_desc_for(uint32_t atlas, uint32_t max_pages) {
    PulseFontPluginDesc desc = pulse_font_plugin_desc_default();
    desc.atlas_width = atlas;
    desc.atlas_height = atlas;
    desc.max_atlas_count = max_pages;
    return desc;
}

static PulseAppId make_font_app(const char* name, const PulseFontPluginDesc* desc) {
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
    assert(pulse_font_plugin_desc_default().struct_size == sizeof(PulseFontPluginDesc));
    assert(pulse_add_font_plugin(app, desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
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
    assert(pulse_asset_request_is_valid(pulse_font_request_to_asset_request(request)));
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

static void unload_font(PulseAppId app, PulseFontHandle font) {
    pulse_asset_system_release(pulse_get_asset_system(app), pulse_font_to_handle(font), nullptr);
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

static uint32_t slot_origin_x(const PulseGlyph& glyph, uint32_t atlas_width) {
    return (uint32_t)lroundf(glyph.u0 * (float)atlas_width - 0.5f);
}

static uint32_t slot_origin_y(const PulseGlyph& glyph, uint32_t atlas_height) {
    return (uint32_t)lroundf(glyph.v0 * (float)atlas_height - 0.5f);
}
