#include "test_common.h"

int main() {
    PulseAppId app = make_font_app("t-font-asset", nullptr);

    const PulseFontRequest latin_request = pulse_font_load(app, "latin.ttf", 0);
    pump_font_request(app, latin_request);
    assert(pulse_font_is_ready(app, latin_request));
    const PulseFontHandle latin = pulse_font_get_handle(app, latin_request);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(latin)));
    assert(pulse_font_count(app) == 2);
    assert(pulse_font_family_name(app, latin) != nullptr);

    const PulseFontRequest again_request = pulse_font_load(app, "latin.ttf", 0);
    pump_font_request(app, again_request);
    assert(pulse_asset_request_equals(pulse_font_request_to_asset_request(again_request), pulse_font_request_to_asset_request(latin_request)));
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_get_handle(app, again_request)), pulse_font_to_handle(latin)));
    assert(pulse_font_count(app) == 2);

    const uint32_t chain = make_chain(app, &latin, 1);
    const PulseGlyph glyph = pulse_font_glyph(app, chain, 'A', 48.0f);
    assert(glyph.valid);

    const PulseFontRequest bad_face_request = pulse_font_load(app, "cjk.ttf", 3);
    pump_font_request(app, bad_face_request);
    assert(!pulse_font_is_ready(app, bad_face_request));
    assert(pulse_font_get_error(app, bad_face_request) != nullptr);
    assert(!pulse_asset_handle_is_valid(pulse_font_to_handle(pulse_font_get_handle(app, bad_face_request))));
    assert(pulse_font_count(app) == 2);

    const PulseFontRequest missing_request = pulse_font_load(app, "missing.ttf", 0);
    assert(pulse_asset_request_is_valid(pulse_font_request_to_asset_request(missing_request)));
    pump_font_request(app, missing_request);
    assert(!pulse_font_is_ready(app, missing_request));
    assert(pulse_font_get_error(app, missing_request) != nullptr);

    unload_font(app, latin);
    assert(pulse_font_count(app) == 1);
    assert(pulse_font_family_name(app, latin) == nullptr);
    assert(!pulse_asset_handle_is_valid(pulse_font_to_handle(pulse_font_get_handle(app, latin_request))));

    const uint32_t reloaded_chain = pulse_font_create_chain(app, &latin, 1);
    assert(reloaded_chain == PULSE_FONT_ID_NONE);
    const PulseFontRequest reload_request = pulse_font_load(app, "latin.ttf", 0);
    pump_font_request(app, reload_request);
    assert(pulse_font_is_ready(app, reload_request));
    const PulseFontHandle reloaded = pulse_font_get_handle(app, reload_request);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(reloaded)));
    assert(pulse_font_count(app) == 2);
    const uint32_t chain2 = make_chain(app, &reloaded, 1);
    const PulseGlyph reloaded_glyph = pulse_font_glyph(app, chain2, 'A', 48.0f);
    assert(reloaded_glyph.valid);
    pulse_font_destroy_chain(app, chain2);
    unload_font(app, reloaded);
    assert(pulse_font_count(app) == 1);

    const std::vector<uint8_t> cjk_bytes = read_test_file("tests/font/data/cjk.ttf");
    const PulseFontHandle cjk = load_font_memory(app, "cjk_mem.ttf", cjk_bytes);
    const uint32_t cjk_chain = make_chain(app, &cjk, 1);
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_resolve_codepoint(app, cjk_chain, 0x4E2D)), pulse_font_to_handle(cjk)));
    pulse_font_destroy_chain(app, cjk_chain);
    unload_font(app, cjk);
    assert(pulse_font_count(app) == 1);

    pulse_destroy_app(app);

    PulseAppDesc bare_desc = {
        .name = "t-font-asset-bare",
    };
    PulseAppId bare_app = pulse_create_app(&bare_desc);
    assert(bare_app);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(bare_app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_font_plugin(bare_app, nullptr) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_prepare(bare_app) == PULSE_APP_PREPARE_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY);
    pulse_destroy_app(bare_app);

    PulseAppId nofont_app = pulse_create_app(&bare_desc);
    assert(nofont_app);
    assert(pulse_add_vfs_plugin(nofont_app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    PulseAssetPluginDesc nofont_asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_add_asset_plugin(nofont_app, &nofont_asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_prepare(nofont_app) == PULSE_APP_PREPARE_RESULT_OK);
    const PulseFontRequest nofont_request = pulse_font_load(nofont_app, "latin.ttf", 0);
    assert(!pulse_asset_request_is_valid(pulse_font_request_to_asset_request(nofont_request)));
    assert(!pulse_font_is_ready(nofont_app, nofont_request));
    assert(!pulse_font_is_alive(nofont_app, nofont_request));
    assert(pulse_font_get_error(nofont_app, nofont_request) == nullptr);
    assert(!pulse_asset_handle_is_valid(pulse_font_to_handle(pulse_font_get_handle(nofont_app, nofont_request))));
    assert(pulse_font_count(nofont_app) == 0);
    pulse_destroy_app(nofont_app);

    printf("font asset load tests passed\n");
    return 0;
}
