#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <vector>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_font.h"
#include "pulse_graphics.h"
#include "pulse_input.h"
#include "pulse_text.h"
#include "pulse_vfs.h"
#include "pulse_window.h"

namespace {

constexpr int32_t kWindowWidth = 640;
constexpr int32_t kWindowHeight = 480;
constexpr int32_t kTextRecordPriority = 100;
constexpr int32_t kFramesBeforeDone = 30;
constexpr const char* kWindowTitle = "test-text-window";
constexpr const char* kDoneTitle = "text-window-rendered";

enum text_chain_kind { kChainMain, kChainBitmap, kChainDefault };

struct text_line_spec {
    text_chain_kind chain;
    float x;
    float baseline;
    float size;
    PulseTextColor color;
    const char* text;
    bool scissored;
    float scissor_x;
    float scissor_y;
    float scissor_width;
    float scissor_height;
};

struct text_line_box {
    uint32_t block = PULSE_TEXT_BLOCK_ID_NONE;
    float left = 0.0f;
    float top = 0.0f;
    PulseTextLayout* layout = nullptr;
    PulseScissor scissor{};
};

struct text_window_state {
    uint32_t chain = PULSE_FONT_ID_NONE;
    uint32_t bitmap_chain = PULSE_FONT_ID_NONE;
    uint32_t default_chain = PULSE_FONT_ID_NONE;
    PulseFontRequest latin_request{};
    PulseFontRequest cjk_request{};
    PulseFontRequest proggy_request{};
    PulseFontHandle latin_font{};
    PulseFontHandle cjk_font{};
    PulseFontHandle proggy_font{};
    std::vector<text_line_box> boxes;
    bool fonts_requested = false;
    bool fonts_ready = false;
    bool initialized = false;
    int32_t frames = 0;
    ecs_entity_t window = 0;
};

void clear_record_callback(PulseAppId app, PulseRenderGraphId graph, void* user_data);

const text_line_spec kLineSpecs[] = {
    { kChainMain, 24.0f, 70.0f, 64.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, "Pulse Font \xE6\x96\x87\xE5\xAD\x97\xE6\xB8\xB2\xE6\x9F\x93", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainMain, 24.0f, 110.0f, 12.0f, { 0.6f, 0.9f, 1.0f, 1.0f }, "12px: Sphinx of black quartz, judge my vow. AV To Wa", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainMain, 24.0f, 142.0f, 18.0f, { 0.7f, 0.95f, 0.7f, 1.0f }, "18px: Sphinx of black quartz, judge my vow. AV To Wa", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainMain, 24.0f, 180.0f, 24.0f, { 1.0f, 0.85f, 0.4f, 1.0f }, "24px: Sphinx of black quartz, judge my vow. AV To Wa", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainMain, 24.0f, 228.0f, 36.0f, { 1.0f, 0.6f, 0.55f, 1.0f }, "36px: quartz AV Wa", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainMain, 24.0f, 286.0f, 48.0f, { 0.85f, 0.7f, 1.0f, 1.0f }, "\xE5\x8D\xA1\xE7\x89\x8C\xE6\x96\x87\xE5\xAD\x97 card text", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainMain, 24.0f, 334.0f, 20.0f, { 0.9f, 0.9f, 0.9f, 1.0f }, "\xE6\x94\xBB\xE5\x87\xBB\xE5\x8A\x9B +2 \xE7\x94\x9F\xE5\x91\xBD\xE5\x80\xBC 30", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainMain, 24.0f, 388.0f, 48.0f, { 0.5f, 1.0f, 0.6f, 1.0f }, "\xE7\xBC\xBA\xE5\xAD\x97\xEF\xBC\x9A\xEE\x80\x80 tofu", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainBitmap, 24.0f, 424.0f, 28.0f, { 1.0f, 1.0f, 1.0f, 1.0f }, "Bitmap SDF: Proggy 0123", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainDefault, 330.0f, 424.0f, 16.0f, { 0.6f, 0.85f, 1.0f, 1.0f }, "default 16px: ABCabc 123", false, 0.0f, 0.0f, 0.0f, 0.0f },
    { kChainMain, 24.0f, 460.0f, 24.0f, { 0.5f, 1.0f, 0.6f, 1.0f }, "\xE8\xA2\xAB\xE8\xA3\x81\xE5\x89\xAA\xE7\x9A\x84\xE6\x96\x87\xE6\x9C\xAC clipped text clipped", true, 24.0f, 430.0f, 160.0f, 48.0f },
};

constexpr size_t kLineCount = sizeof(kLineSpecs) / sizeof(kLineSpecs[0]);

void init_system_run(ecs_iter_t* it) {
    text_window_state* state = static_cast<text_window_state*>(it->ctx);
    if (!state) {
        return;
    }
    PulseAppId app = pulse_get_app_from_world(it->world);
    if (!app) {
        return;
    }

    PulseRenderRecordCallbackDesc callback_desc{};
    callback_desc.callback = clear_record_callback;
    callback_desc.user_data = state;
    callback_desc.priority = kTextRecordPriority - 10;
    assert(pulse_add_render_record_callback(app, &callback_desc) == PULSE_RESULT_OK);

    state->window = ecs_lookup(it->world, kWindowTitle);
    assert(state->window != 0);
    state->initialized = true;
}

void build_text_boxes(PulseAppId app, text_window_state* state) {
    const uint32_t chains[] = { state->chain, state->bitmap_chain, state->default_chain };
    state->boxes.resize(kLineCount);
    for (size_t i = 0; i < kLineCount; ++i) {
        const text_line_spec& spec = kLineSpecs[i];
        PulseTextBlockDesc desc{};
        desc.chain = chains[spec.chain];
        desc.size = spec.size;
        desc.color = spec.color;
        desc.align_h = PULSE_TEXT_ALIGN_H_LEFT;
        desc.align_v = PULSE_TEXT_ALIGN_V_TOP;
        desc.line_height = 0.0f;
        text_line_box& box = state->boxes[i];
        box.block = pulse_text_block_create(app, &desc);
        assert(box.block != PULSE_TEXT_BLOCK_ID_NONE);
        const PulseVerticalMetrics metrics = pulse_font_vertical_metrics(app, chains[spec.chain], spec.size);
        box.left = spec.x;
        box.top = spec.baseline - metrics.ascent;
        box.layout = pulse_text_block_layout(app, box.block, spec.text, 0.0f, 0.0f);
        assert(box.layout != nullptr);
        assert(box.layout->instances_count > 0);
        if (spec.scissored) {
            box.scissor.x = spec.scissor_x - box.left;
            box.scissor.y = spec.scissor_y - box.top;
            box.scissor.width = spec.scissor_width;
            box.scissor.height = spec.scissor_height;
            box.scissor.enabled = true;
        }
    }

    const PulseTextMeasure clipped_measure = pulse_text_block_measure(app, state->boxes[kLineCount - 1].block, "\xE8\xA2\xAB\xE8\xA3\x81\xE5\x89\xAA\xE7\x9A\x84\xE6\x96\x87\xE6\x9C\xAC clipped text", 0.0f);
    assert(clipped_measure.width > 160.0f);
}

void prepare_fonts(PulseAppId app, text_window_state* state) {
    if (!state->fonts_requested) {
        state->latin_request = pulse_font_load(app, "latin.ttf", 0);
        state->cjk_request = pulse_font_load(app, "cjk.ttf", 0);
        state->proggy_request = pulse_font_load(app, "ProggyVector.fnt", 0);
        assert(pulse_asset_request_is_valid(pulse_font_request_to_asset_request(state->latin_request)));
        assert(pulse_asset_request_is_valid(pulse_font_request_to_asset_request(state->cjk_request)));
        assert(pulse_asset_request_is_valid(pulse_font_request_to_asset_request(state->proggy_request)));
        state->fonts_requested = true;
        return;
    }
    if (!pulse_font_is_ready(app, state->latin_request) || !pulse_font_is_ready(app, state->cjk_request) || !pulse_font_is_ready(app, state->proggy_request)) {
        assert(pulse_font_is_alive(app, state->latin_request));
        assert(pulse_font_is_alive(app, state->cjk_request));
        assert(pulse_font_is_alive(app, state->proggy_request));
        return;
    }
    state->latin_font = pulse_font_get_handle(app, state->latin_request);
    state->cjk_font = pulse_font_get_handle(app, state->cjk_request);
    state->proggy_font = pulse_font_get_handle(app, state->proggy_request);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(state->latin_font)));
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(state->cjk_font)));
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(state->proggy_font)));
    const PulseFontHandle default_font = pulse_font_default(app);
    assert(pulse_asset_handle_is_valid(pulse_font_to_handle(default_font)));
    assert(pulse_font_count(app) == 4);
    const PulseFontHandle fonts[] = { state->latin_font, state->cjk_font };
    state->chain = pulse_font_create_chain(app, fonts, 2);
    state->bitmap_chain = pulse_font_create_chain(app, &state->proggy_font, 1);
    state->default_chain = pulse_font_create_chain(app, &default_font, 1);
    assert(state->chain != PULSE_FONT_ID_NONE);
    assert(state->bitmap_chain != PULSE_FONT_ID_NONE);
    assert(state->default_chain != PULSE_FONT_ID_NONE);
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_resolve_codepoint(app, state->chain, 0x4E2D)), pulse_font_to_handle(state->cjk_font)));
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_resolve_codepoint(app, state->chain, 'A')), pulse_font_to_handle(state->latin_font)));
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_resolve_codepoint(app, state->default_chain, 'A')), pulse_font_to_handle(default_font)));

    build_text_boxes(app, state);

    const PulseAtlasStats stats = pulse_font_atlas_stats(app);
    assert(stats.rasterize_count > 0);
    assert(stats.glyph_count > 0);
    assert(stats.page_count >= 1);

    const PulseGlyph missing = pulse_font_glyph(app, state->chain, 0x1FFFF, 48.0f);
    assert(missing.valid);
    assert(fabsf(missing.advance - 48.0f * 0.8f) < 1e-4f);

    state->fonts_ready = true;
}

void submit_system_run(ecs_iter_t* it) {
    text_window_state* state = static_cast<text_window_state*>(it->ctx);
    if (!state || !state->initialized) {
        return;
    }
    PulseAppId app = pulse_get_app_from_world(it->world);
    if (!app) {
        return;
    }
    if (!state->fonts_ready) {
        prepare_fonts(app, state);
        if (!state->fonts_ready) {
            return;
        }
    }
    for (const text_line_box& box : state->boxes) {
        PulseDrawDesc desc{};
        desc.p_instances = box.layout->p_instances;
        desc.instances_count = box.layout->instances_count;
        desc.transform.scale_x = 2.0f / (float)kWindowWidth;
        desc.transform.scale_y = -2.0f / (float)kWindowHeight;
        desc.transform.translate_x = -1.0f + 2.0f * box.left / (float)kWindowWidth;
        desc.transform.translate_y = 1.0f - 2.0f * box.top / (float)kWindowHeight;
        desc.scissor = box.scissor;
        assert(pulse_font_submit(app, &desc) == PULSE_RESULT_OK);
    }

    ++state->frames;
    if (state->frames == kFramesBeforeDone && state->window) {
        assert(pulse_window_set_title(app, state->window, kDoneTitle) == PULSE_RESULT_OK);
    }
}

void clear_record_callback(PulseAppId app, PulseRenderGraphId graph, void* user_data) {
    text_window_state* state = static_cast<text_window_state*>(user_data);
    if (!state || !state->window) {
        return;
    }
    const PulseRGTextureHandle backbuffer = pulse_import_window_backbuffer(app, graph, state->window);
    if (!pulse_rgtexture_handle_is_valid(backbuffer)) {
        return;
    }
    PulseRenderPassBuilder pass = pulse_render_graph_add_render_pass(graph, "PulseTextTestClear");
    pulse_render_pass_builder_add_color_attachment(&pass, backbuffer, CGPU_LOAD_ACTION_CLEAR, 0xff2a2f38, CGPU_STORE_ACTION_STORE);
}

}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-text-window",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_vfs_mount("tests/font/data", "/", false));

    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseWindowPluginDesc window_desc = pulse_window_plugin_desc_default();
    window_desc.primary_window.title = kWindowTitle;
    window_desc.primary_window.width = kWindowWidth;
    window_desc.primary_window.height = kWindowHeight;
    assert(pulse_add_window_plugin(app, &window_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseGraphicsPluginDesc graphics_desc = pulse_graphics_plugin_desc_default();
    graphics_desc.enable_debug_layer = true;
    graphics_desc.enable_gpu_based_validation = true;
    assert(pulse_add_graphics_plugin(app, &graphics_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(pulse_add_font_plugin(app, nullptr) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_text_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    text_window_state state{};

    ecs_entity_desc_t init_entity_desc{};
    init_entity_desc.name = "text_window_init";
    ecs_system_desc_t init_system_desc{};
    init_system_desc.entity = ecs_entity_init(pulse_app_world(app), &init_entity_desc);
    init_system_desc.phase = EcsOnStart;
    init_system_desc.ctx = &state;
    init_system_desc.callback = init_system_run;
    ecs_system_init(pulse_app_world(app), &init_system_desc);

    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "text_window_submit";
    ecs_system_desc_t system_desc{};
    system_desc.entity = ecs_entity_init(pulse_app_world(app), &entity_desc);
    system_desc.phase = EcsOnUpdate;
    system_desc.ctx = &state;
    system_desc.callback = submit_system_run;
    ecs_system_init(pulse_app_world(app), &system_desc);

    const EPulseAppRunResult result = pulse_app_run(app);
    if (result != PULSE_APP_RUN_RESULT_OK) {
        const char* error = pulse_app_last_error(app);
        fprintf(stderr, "run failed: result=%d error=%s\n", (int)result, error ? error : "(none)");
        fflush(stderr);
        return 1;
    }

    if (state.fonts_ready) {
        for (text_line_box& box : state.boxes) {
            pulse_text_layout_free(app, box.layout);
            pulse_text_block_destroy(app, box.block);
        }
        pulse_font_destroy_chain(app, state.chain);
        pulse_font_destroy_chain(app, state.bitmap_chain);
        pulse_font_destroy_chain(app, state.default_chain);
        pulse_asset_system_release(pulse_get_asset_system(app), pulse_font_to_handle(state.latin_font), nullptr);
        pulse_asset_system_release(pulse_get_asset_system(app), pulse_font_to_handle(state.cjk_font), nullptr);
        pulse_asset_system_release(pulse_get_asset_system(app), pulse_font_to_handle(state.proggy_font), nullptr);
    }

    pulse_destroy_app(app);
    printf("text window test finished\n");
    return 0;
}
