#include "test_common.h"

static PulseFontChainHandle pump_chain(PulseAppId app, PulseFontChainRequest request) {
    while (!pulse_font_chain_is_ready(app, request) && pulse_font_chain_is_alive(app, request)) {
        assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    }
    return pulse_font_chain_get_handle(app, request);
}

int main() {
    PulseAppId app = make_font_app("t-font-chain-file", nullptr);

    const PulseFontHandle latin = register_latin(app);
    const PulseFontHandle cjk = register_cjk(app);

    const PulseFontChainRequest request = pulse_font_load_chain(app, "chain_latin_cjk.fontchain");
    assert(request.index != 0 && request.generation != 0);
    const PulseFontChainHandle chain = pump_chain(app, request);
    assert(chain.index != 0 && chain.generation != 0);

    const uint32_t han = 0x4E2D;
    const PulseFontChainHandle latin_ref = make_chain(app, &latin, 1);
    const PulseFontChainHandle cjk_ref = make_chain(app, &cjk, 1);
    assert(fabsf(pulse_font_advance(app, chain, 'A', 48.0f) - pulse_font_advance(app, latin_ref, 'A', 48.0f)) < 1e-5f);
    assert(fabsf(pulse_font_advance(app, chain, han, 48.0f) - pulse_font_advance(app, cjk_ref, han, 48.0f)) < 1e-5f);
    assert(pulse_font_advance(app, chain, 'A', 48.0f) > 0.0f);
    assert(pulse_font_vertical_metrics(app, chain, 48.0f).ascent > 0.0f);
    assert(pulse_font_glyph(app, chain, 'A', 48.0f).valid);

    const PulseFontChainRequest again_request = pulse_font_load_chain(app, "chain_latin_cjk.fontchain");
    assert(again_request.index == request.index && again_request.generation == request.generation);
    const PulseFontChainHandle again = pulse_font_chain_get_handle(app, again_request);
    assert(again.index == chain.index && again.generation == chain.generation);

    const PulseFontChainRequest bad_request = pulse_font_load_chain(app, "chain_bad.fontchain");
    assert(bad_request.index != 0 && bad_request.generation != 0);
    const PulseFontChainHandle bad_chain = pump_chain(app, bad_request);
    assert(bad_chain.index == 0 && bad_chain.generation == 0);
    assert(pulse_font_chain_get_error(app, bad_request) != nullptr);
    assert(pulse_font_advance(app, bad_chain, 'A', 48.0f) == 0.0f);

    pulse_font_destroy_chain(app, latin_ref);
    pulse_font_destroy_chain(app, cjk_ref);
    pulse_font_destroy_chain(app, chain);
    assert(pulse_font_advance(app, chain, 'A', 48.0f) == 0.0f);

    unload_font(app, latin);
    unload_font(app, cjk);

    pulse_destroy_app(app);

    PulseAppId lazy_app = make_font_app("t-font-chain-file-lazy", nullptr);
    const PulseFontChainRequest lazy_request = pulse_font_load_chain(lazy_app, "chain_latin.fontchain");
    assert(lazy_request.index != 0 && lazy_request.generation != 0);
    const PulseFontChainHandle lazy_chain = pump_chain(lazy_app, lazy_request);
    assert(lazy_chain.index != 0 && lazy_chain.generation != 0);
    assert(pulse_font_advance(lazy_app, lazy_chain, 'A', 48.0f) > 0.0f);
    pulse_font_destroy_chain(lazy_app, lazy_chain);
    pulse_destroy_app(lazy_app);

    printf("font chain file tests passed\n");
    return 0;
}
