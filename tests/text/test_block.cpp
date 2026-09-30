#include "test_common.h"

static PulseFontChainHandle load_chain_ready(PulseAppId app, const char* path) {
    const PulseFontChainRequest request = pulse_font_load_chain(app, path);
    assert(request.index != 0 && request.generation != 0);
    while (!pulse_font_chain_is_ready(app, request) && pulse_font_chain_is_alive(app, request)) {
        assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    }
    const PulseFontChainHandle chain = pulse_font_chain_get_handle(app, request);
    assert(chain.index != 0 && chain.generation != 0);
    return chain;
}

int main() {
    PulseAppId app = make_text_app("t-text-block");
    const PulseFontHandle latin = register_latin(app);
    const PulseFontChainHandle chain = make_chain(app, &latin, 1);

    ecs_world_t* world = pulse_app_world(app);
    const ecs_entity_t desc_type = ecs_lookup(world, "PulseTextBlockDesc");
    assert(desc_type != 0);
    assert(ecs_struct_get_member(world, desc_type, "chain") != nullptr);
    assert(ecs_struct_get_member(world, desc_type, "size") != nullptr);
    assert(ecs_struct_get_member(world, desc_type, "align_h") != nullptr);
    assert(ecs_struct_get_member(world, desc_type, "align_v") != nullptr);
    assert(ecs_struct_get_member(world, desc_type, "line_height") != nullptr);
    const ecs_entity_t color_type = ecs_lookup(world, "PulseTextColor");
    assert(color_type != 0);
    assert(ecs_struct_get_member(world, color_type, "r") != nullptr);
    assert(ecs_struct_get_member(world, color_type, "a") != nullptr);
    assert(ecs_struct_get_member(world, desc_type, "color")->type == color_type);

    assert(pulse_text_layout(app, nullptr, "x", 100.0f, 100.0f) == nullptr);

    PulseTextBlockDesc desc = text_desc(chain, 24.0f);
    PulseFontChainHandle invalid_chain{};
    desc.chain = invalid_chain;
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

    const ecs_entity_t holder = ecs_new(world);
    const PulseFontChainHandle file_chain = load_chain_ready(app, "chain_latin.fontchain");
    assert(pulse_asset_handle_equals(pulse_font_to_handle(pulse_font_resolve_codepoint(app, file_chain, 'A')), pulse_font_to_handle(latin)));

    {
        void* stored = ecs_ensure_id(world, holder, desc_type, sizeof(PulseTextBlockDesc));
        assert(stored != nullptr);
        memcpy(stored, &desc, sizeof(desc));
        char* json = ecs_entity_to_json(world, holder, nullptr);
        assert(json != nullptr);
        assert(strstr(json, "\"chain\":\"\"") != nullptr);
        ecs_os_free(json);
        PulseTextBlockDesc serialized = desc;
        serialized.chain = file_chain;
        memcpy(stored, &serialized, sizeof(serialized));
        json = ecs_entity_to_json(world, holder, nullptr);
        assert(strstr(json, "\"chain\":\"chain_latin.fontchain\"") != nullptr);
        ecs_os_free(json);
    }

    {
        PulseTextBlockDesc restored = {};
        assert(ecs_ptr_from_json(world, desc_type, &restored, "{\"chain\":\"chain_latin.fontchain\"}", nullptr) != nullptr);
        assert(restored.chain.index == file_chain.index && restored.chain.generation == file_chain.generation);
    }

    {
        PulseTextBlockDesc absent = {};
        absent.size = 24.0f;
        assert(ecs_ptr_from_json(world, desc_type, &absent, "{\"chain\":\"text/absent\"}", nullptr) != nullptr);
        assert(absent.chain.index == 0 && absent.chain.generation == 0);
    }

    pulse_font_destroy_chain(app, file_chain);

    {
        char* json = ecs_entity_to_json(world, holder, nullptr);
        assert(json != nullptr);
        assert(strstr(json, "\"chain\":\"\"") != nullptr);
        ecs_os_free(json);
    }

    {
        PulseTextBlockDesc dead = {};
        assert(ecs_ptr_from_json(world, desc_type, &dead, "{\"chain\":\"chain_latin.fontchain\"}", nullptr) != nullptr);
        assert(dead.chain.index == 0 && dead.chain.generation == 0);
    }

    pulse_font_destroy_chain(app, chain);
    pulse_destroy_app(app);
    printf("text block tests passed\n");
    return 0;
}
