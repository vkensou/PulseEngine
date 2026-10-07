#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"
#include "pulse_input.h"
#include "pulse_window.h"
#include "pulse_math.h"
#include "pulse_transform.h"
#include "pulse_graphics.h"
#include "pulse_renderer.h"
#include "pulse_prefab.h"
#include "pulse_font.h"
#include "pulse_text.h"
#include "pulse_text_render.h"

static void update_once(PulseAppId app) {
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
}

static bool wait_script_ready(PulseAppId app, PulsePrefabScriptRequest request) {
    for (int frame = 0; frame < 600; ++frame) {
        if (pulse_prefab_script_is_ready(app, request)) {
            return true;
        }
        if (!pulse_prefab_script_is_alive(app, request)) {
            return false;
        }
        update_once(app);
    }
    return false;
}

template <typename T>
static bool handle_is_zero(T handle) {
    return handle.index == 0 && handle.generation == 0;
}

static void assert_script_fails(PulseAppId app, const char* path, const char* expected_error) {
    PulsePrefabScriptRequest request = pulse_load_prefab_script(app, path);
    PulseAssetRequest asset_request = pulse_prefab_script_request_to_asset_request(request);
    assert(pulse_asset_request_is_valid(asset_request));
    assert(!wait_script_ready(app, request));
    assert(!pulse_prefab_script_is_alive(app, request));
    assert(pulse_asset_system_get_state(pulse_get_asset_system(app), asset_request) == PULSE_ASSET_STATE_FAILED);
    const char* error = pulse_asset_system_get_error(pulse_get_asset_system(app), asset_request);
    if (!error || !strstr(error, expected_error)) {
        fprintf(stderr, "load error for %s: %s\n", path, error ? error : "(null)");
    }
    assert(error != nullptr && strstr(error, expected_error) != nullptr);
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-prefab-script",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseWindowPluginDesc window_desc = pulse_window_plugin_desc_default();
    window_desc.flags = PULSE_WINDOW_PLUGIN_CREATE_PRIMARY;
    window_desc.primary_window.width = 320;
    window_desc.primary_window.height = 240;
    assert(pulse_add_window_plugin(app, &window_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_vfs_mount("tests/flecs_script/data", "/", false));
    assert(pulse_vfs_mount("tests/graphics/data", "/", false));
    assert(pulse_vfs_mount("tests/font/data", "/", false));
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(pulse_add_math_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseGraphicsPluginDesc graphic_desc = pulse_graphics_plugin_desc_default();
    assert(pulse_add_graphics_plugin(app, &graphic_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_renderer_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_prefab_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_font_plugin(app, nullptr) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_text_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_text_render_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    ecs_world_t* world = pulse_app_world(app);
    PulseAssetSystemId asset_system = pulse_get_asset_system(app);

    assert(handle_is_zero(pulse_asset_system_find_loaded(asset_system, PULSE_TYPE_MESH, "Quad.obj")));

    PulsePrefabScriptRequest script_request = pulse_load_prefab_script(app, "card.flecs");
    assert(pulse_asset_request_is_valid(pulse_prefab_script_request_to_asset_request(script_request)));
    if (!wait_script_ready(app, script_request)) {
        const char* error = pulse_prefab_script_get_error(app, script_request);
        fprintf(stderr, "card.flecs load error: %s\n", error ? error : "(null)");
        assert(false);
    }
    PulsePrefabScriptHandle script_handle = pulse_prefab_script_get_handle(app, script_request);
    assert(script_handle.index != 0 && script_handle.generation != 0);

    ecs_entity_t card = pulse_prefab_script_get_entity(app, script_handle, "Card");
    assert(card != 0 && ecs_is_alive(world, card));
    ecs_entity_t marker = pulse_prefab_script_get_entity(app, script_handle, "Marker");
    assert(marker != 0 && ecs_has_id(world, marker, EcsPrefab));
    assert(pulse_prefab_script_get_entity(app, script_handle, "Nope") == 0);

    assert(!handle_is_zero(pulse_asset_system_find_loaded(asset_system, PULSE_TYPE_MESH, "Quad.obj")));
    assert(!handle_is_zero(pulse_asset_system_find_loaded(asset_system, PULSE_TYPE_MATERIAL, "quad.material")));
    assert(!handle_is_zero(pulse_asset_system_find_loaded(asset_system, PULSE_TYPE_FONT_CHAIN, "chain_latin.fontchain")));

    PulseAssetHandle mesh_asset = pulse_asset_system_find_loaded(asset_system, PULSE_TYPE_MESH, "Quad.obj");
    PulseAssetHandle material_asset = pulse_asset_system_find_loaded(asset_system, PULSE_TYPE_MATERIAL, "quad.material");
    PulseAssetHandle chain_asset = pulse_asset_system_find_loaded(asset_system, PULSE_TYPE_FONT_CHAIN, "chain_latin.fontchain");

    {
        const char* names[] = { "title", "footer", "title_align", "frame_width", "face" };
        const char* values[] = { "火球术", "造成2点火焰伤害", "PULSE_TEXT_ALIGN_H_RIGHT", "300", "quad.material" };
        ecs_entity_t e = pulse_prefab_script_instantiate(app, card, 0, names, 5, values, 5);
        if (!e) {
            fprintf(stderr, "instantiate error: %s\n", pulse_prefab_last_error());
        }
        assert(e != 0 && ecs_is_alive(world, e));
        assert(!ecs_has_id(world, e, EcsPrefab));
        assert(pulse_prefab_script_instantiate(app, card, e, names, 5, values, 5) == e);

        ecs_entity_t frame = ecs_lookup_child(world, e, "Frame");
        assert(frame != 0);
        const PulseLocalTransform* frame_transform = ecs_get(world, frame, PulseLocalTransform);
        assert(frame_transform != nullptr);
        assert(frame_transform->scale.X == 300.0f && frame_transform->scale.Y == 320.0f);
        const PulseRenderable* frame_renderable = ecs_get(world, frame, PulseRenderable);
        assert(frame_renderable != nullptr);
        assert(frame_renderable->mesh.index == mesh_asset.index && frame_renderable->mesh.generation == mesh_asset.generation);
        assert(frame_renderable->material.index == material_asset.index && frame_renderable->material.generation == material_asset.generation);
        assert(frame_renderable->sorting_order == 1);

        ecs_entity_t face = ecs_lookup_child(world, e, "Face");
        assert(face != 0);
        const PulseRenderable* face_renderable = ecs_get(world, face, PulseRenderable);
        assert(face_renderable != nullptr);
        assert(face_renderable->material.index == material_asset.index && face_renderable->material.generation == material_asset.generation);

        ecs_entity_t title = ecs_lookup_child(world, e, "Title");
        assert(title != 0);
        const PulseText* title_text = ecs_get(world, title, PulseText);
        assert(title_text != nullptr);
        assert(strcmp(title_text->text, "火球术") == 0);
        assert(title_text->block.chain.index == chain_asset.index && title_text->block.chain.generation == chain_asset.generation);
        assert(title_text->block.size == 55.0f);
        assert(title_text->block.align_h == PULSE_TEXT_ALIGN_H_RIGHT);
        assert(title_text->block.align_v == PULSE_TEXT_ALIGN_V_MIDDLE);
        assert(title_text->block.auto_size == true);
        assert(title_text->block.color.r == 0.0f && title_text->block.color.a == 1.0f);
        assert(title_text->sorting_order == 1000);

        ecs_entity_t footer = ecs_lookup_child(world, e, "Footer");
        assert(footer != 0);
        const PulseText* footer_text = ecs_get(world, footer, PulseText);
        assert(footer_text != nullptr);
        assert(strcmp(footer_text->text, "造成2点火焰伤害") == 0);
        assert(footer_text->block.size == 25.0f);
        assert(footer_text->block.align_h == PULSE_TEXT_ALIGN_H_CENTER);
        assert(footer_text->block.align_v == PULSE_TEXT_ALIGN_V_TOP);
        assert(footer_text->block.line_height == 1.25f);
        assert(footer_text->block.auto_size == false);
        assert(footer_text->block.color.r == 0.25f && footer_text->block.color.b == 0.75f && footer_text->block.color.a == 0.5f);
        assert(footer_text->sorting_order == 7);
    }

    {
        ecs_entity_t e = pulse_prefab_script_instantiate(app, card, 0, nullptr, 0, nullptr, 0);
        assert(e != 0);
        assert(ecs_lookup_child(world, e, "Frame") != 0);
        assert(ecs_lookup_child(world, e, "Face") != 0);
        assert(ecs_lookup_child(world, e, "Title") != 0);
        assert(ecs_lookup_child(world, e, "Footer") == 0);

        const PulseLocalTransform* frame_transform = ecs_get(world, ecs_lookup_child(world, e, "Frame"), PulseLocalTransform);
        assert(frame_transform != nullptr && frame_transform->scale.X == 318.0f);

        const PulseText* title_text = ecs_get(world, ecs_lookup_child(world, e, "Title"), PulseText);
        assert(title_text != nullptr);
        assert(strcmp(title_text->text, "精") == 0);
        assert(title_text->block.align_h == PULSE_TEXT_ALIGN_H_LEFT);

        const PulseRenderable* face_renderable = ecs_get(world, ecs_lookup_child(world, e, "Face"), PulseRenderable);
        assert(face_renderable != nullptr);
        assert(handle_is_zero(face_renderable->material));
    }

    {
        const char* names[] = { "title" };
        const char* values[] = { "第一版" };
        ecs_entity_t e = pulse_prefab_script_instantiate(app, card, 0, names, 1, values, 1);
        assert(e != 0);
        ecs_entity_t old_title = ecs_lookup_child(world, e, "Title");
        assert(old_title != 0);

        PulseLocalTransform game_transform{};
        game_transform.translation.Y = 42.0f;
        game_transform.rotation.W = 1.0f;
        ecs_set_id(world, e, ecs_id(PulseLocalTransform), sizeof(PulseLocalTransform), &game_transform);

        const char* new_names[] = { "title" };
        const char* new_values[] = { "第二版" };
        assert(pulse_prefab_script_instantiate(app, card, e, new_names, 1, new_values, 1) == e);

        const PulseLocalTransform* transform = ecs_get(world, e, PulseLocalTransform);
        assert(transform != nullptr && transform->translation.Y == 42.0f);

        ecs_entity_t new_title = ecs_lookup_child(world, e, "Title");
        assert(new_title != 0 && new_title != old_title);
        const PulseText* title_text = ecs_get(world, new_title, PulseText);
        assert(title_text != nullptr);
        assert(strcmp(title_text->text, "第二版") == 0);
    }

    {
        ecs_entity_t e = 0;
        ecs_readonly_begin(world, false);
        const char* names[] = { "title", "footer" };
        const char* values[] = { "异步卡", "命令缓冲" };
        e = pulse_prefab_script_instantiate(app, card, 0, names, 2, values, 2);
        ecs_readonly_end(world);

        assert(e != 0 && ecs_is_alive(world, e));
        ecs_entity_t title = ecs_lookup_child(world, e, "Title");
        assert(title != 0);
        const PulseText* title_text = ecs_get(world, title, PulseText);
        assert(title_text != nullptr);
        assert(strcmp(title_text->text, "异步卡") == 0);
        ecs_entity_t footer = ecs_lookup_child(world, e, "Footer");
        assert(footer != 0);
        assert(strcmp(ecs_get(world, footer, PulseText)->text, "命令缓冲") == 0);
    }

    {
        ecs_entity_t instance = pulse_prefab_script_instantiate(app, marker, 0, nullptr, 0, nullptr, 0);
        assert(instance != 0 && ecs_has_pair(world, instance, EcsIsA, marker));
        assert(!ecs_has_id(world, instance, EcsPrefab));
        const PulseRenderable* renderable = ecs_get(world, instance, PulseRenderable);
        assert(renderable != nullptr);
        assert(renderable->mesh.index == mesh_asset.index && renderable->mesh.generation == mesh_asset.generation);
        assert(renderable->material.index == material_asset.index && renderable->material.generation == material_asset.generation);
        const PulseLocalTransform* transform = ecs_get(world, instance, PulseLocalTransform);
        assert(transform != nullptr && transform->translation.X == 5.0f);
    }

    {
        const char* names[] = { "nope" };
        const char* values[] = { "1" };
        assert(pulse_prefab_script_instantiate(app, card, 0, names, 1, values, 1) == 0);
        assert(strstr(pulse_prefab_last_error(), "unknown prop 'nope'") != nullptr);

        const char* face_names[] = { "face" };
        const char* face_values[] = { "missing.material" };
        assert(pulse_prefab_script_instantiate(app, card, 0, face_names, 1, face_values, 1) == 0);
        assert(strstr(pulse_prefab_last_error(), "missing.material") != nullptr);

        assert(pulse_prefab_script_instantiate(app, card, 0, names, 1, values, 0) == 0);
        assert(strstr(pulse_prefab_last_error(), "equal length") != nullptr);

        assert(pulse_prefab_script_instantiate(app, marker, 0, names, 1, values, 1) == 0);
        assert(strstr(pulse_prefab_last_error(), "takes no props") != nullptr);

        ecs_entity_t plain = ecs_new(world);
        assert(pulse_prefab_script_instantiate(app, plain, 0, nullptr, 0, nullptr, 0) == 0);
        assert(strstr(pulse_prefab_last_error(), "not a template or prefab") != nullptr);
        ecs_delete(world, plain);

        assert(pulse_prefab_script_instantiate(app, 0, 0, nullptr, 0, nullptr, 0) == 0);
        assert(pulse_prefab_script_instantiate(app, card, 12345678, nullptr, 0, nullptr, 0) == 0);
        assert(strstr(pulse_prefab_last_error(), "not alive") != nullptr);
    }

    {
        char value[32];
        for (int i = 0; i < 100; ++i) {
            snprintf(value, sizeof(value), "card_%d", i);
            const char* names[] = { "title" };
            const char* values[] = { value };
            ecs_entity_t e = pulse_prefab_script_instantiate(app, card, 0, names, 1, values, 1);
            assert(e != 0);
            const PulseText* title_text = ecs_get(world, ecs_lookup_child(world, e, "Title"), PulseText);
            assert(title_text != nullptr);
            assert(strcmp(title_text->text, value) == 0);
        }
    }

    assert_script_fails(app, "bad_syntax.flecs", "bad_syntax.flecs");
    assert_script_fails(app, "bad_eval.flecs", "bad_eval.flecs");
    assert_script_fails(app, "bad_eval.flecs", "NoSuchComponent");
    assert_script_fails(app, "empty.flecs", "declares no entity");

    {
        pulse_asset_system_force_unload_assets(asset_system, PULSE_TYPE_PREFAB_SCRIPT);
        assert(!pulse_prefab_script_is_alive(app, script_request));
        assert(!ecs_is_alive(world, card));
        assert(!ecs_is_alive(world, marker));

        PulsePrefabScriptRequest reloaded = pulse_load_prefab_script(app, "card.flecs");
        assert(wait_script_ready(app, reloaded));
        PulsePrefabScriptHandle reloaded_handle = pulse_prefab_script_get_handle(app, reloaded);
        ecs_entity_t reloaded_card = pulse_prefab_script_get_entity(app, reloaded_handle, "Card");
        assert(reloaded_card != 0 && reloaded_card != card);
        const char* names[] = { "title" };
        const char* values[] = { "重新加载" };
        ecs_entity_t e = pulse_prefab_script_instantiate(app, reloaded_card, 0, names, 1, values, 1);
        assert(e != 0);
        assert(strcmp(ecs_get(world, ecs_lookup_child(world, e, "Title"), PulseText)->text, "重新加载") == 0);
    }

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("prefab script test passed\n");
    return 0;
}
