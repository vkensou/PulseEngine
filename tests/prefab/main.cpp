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

struct TestNote {
    ecs_string_t value;
};

struct TestNoteHolder {
    TestNote note;
    PulseMeshHandle mesh;
};

struct TestTag {};

ECS_COMPONENT_DECLARE(TestNoteHolder);

static const char kRenderableJson[] = "{\"components\":{\"PulseRenderable\":{\"mesh\":\"Quad.obj\", \"material\":\"quad.material\"}}}";
static const char kOpaqueJson[] = "{\"components\":{\"TestNoteHolder\":{\"note\":\"not_an_asset.obj\", \"mesh\":\"Quad.obj\"}}}";
static const char kTagNullJson[] = "{\"components\":{\"TestTag\":null}}";
static const char kTagEmptyJson[] = "{\"components\":{\"TestTag\":{}}}";
static const char kTagArrayJson[] = "{\"tags\":[\"TestTag\"]}";
static const char kTagPairJson[] = "{\"components\":{\"(TestPairRel,TestPairTarget)\":null}}";
static const char kTagPairMapJson[] = "{\"pairs\":{\"TestPairRel\":\"TestPairTarget\"}}";

static void assign_test_note(TestNote* dst, ecs_world_t* world, const char* value) {
    (void)world;
    ecs_os_free(dst->value);
    dst->value = ecs_os_strdup(value);
}

static int serialize_test_note(const flecs::serializer* ser, const TestNote* note) {
    const char* value = note->value ? note->value : "";
    return ser->value(ecs_id(ecs_string_t), &value);
}

static void update_once(PulseAppId app) {
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
}

static bool wait_prefab_ready(PulseAppId app, PulsePrefabRequest request) {
    for (int frame = 0; frame < 600; ++frame) {
        if (pulse_prefab_is_ready(app, request)) {
            return true;
        }
        if (!pulse_prefab_is_alive(app, request)) {
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

static void assert_prefab_fails(PulseAppId app, const char* path, const char* expected_error) {
    PulsePrefabRequest request = pulse_load_prefab(app, path);
    PulseAssetRequest asset_request = pulse_prefab_request_to_asset_request(request);
    assert(pulse_asset_request_is_valid(asset_request));
    assert(!wait_prefab_ready(app, request));
    assert(!pulse_prefab_is_alive(app, request));
    assert(pulse_asset_system_get_state(pulse_get_asset_system(app), asset_request) == PULSE_ASSET_STATE_FAILED);
    const char* error = pulse_asset_system_get_error(pulse_get_asset_system(app), asset_request);
    assert(error != nullptr && strstr(error, expected_error) != nullptr);
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-prefab",
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
    assert(pulse_vfs_mount("tests/prefab/data", "/", false));
    assert(pulse_vfs_mount("tests/graphics/data", "/", false));
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(pulse_add_math_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseGraphicsPluginDesc graphic_desc = pulse_graphics_plugin_desc_default();
    assert(pulse_add_graphics_plugin(app, &graphic_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_renderer_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_prefab_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    ecs_world_t* world = pulse_app_world(app);

    flecs::opaque<TestNote>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_test_note).assign_string(assign_test_note);
    flecs::component<TestNoteHolder> note_holder_comp(world, "TestNoteHolder");
    ecs_id(TestNoteHolder) = note_holder_comp.id();
    note_holder_comp.member("note", &TestNoteHolder::note);
    note_holder_comp.member("mesh", &TestNoteHolder::mesh);
    flecs::component<TestTag> test_tag_comp(world, "TestTag");
    ecs_entity_desc_t pair_rel_desc = {};
    pair_rel_desc.name = "TestPairRel";
    ecs_entity_t test_pair_rel = ecs_entity_init(world, &pair_rel_desc);
    ecs_entity_desc_t pair_target_desc = {};
    pair_target_desc.name = "TestPairTarget";
    ecs_entity_t test_pair_target = ecs_entity_init(world, &pair_target_desc);

    {
        ecs_entity_t entity = ecs_new(world);
        assert(ecs_entity_from_json(world, entity, kRenderableJson, nullptr) != nullptr);
        const PulseRenderable* renderable = ecs_get(world, entity, PulseRenderable);
        assert(renderable != nullptr);
        assert(handle_is_zero(renderable->mesh));
        assert(handle_is_zero(renderable->material));
        ecs_delete(world, entity);
    }

    {
        ecs_entity_t entity = ecs_new(world);
        assert(ecs_entity_from_json(world, entity, kOpaqueJson, nullptr) != nullptr);
        const TestNoteHolder* holder = ecs_get(world, entity, TestNoteHolder);
        assert(holder != nullptr);
        assert(strcmp(holder->note.value, "not_an_asset.obj") == 0);
        assert(handle_is_zero(holder->mesh));
        ecs_delete(world, entity);
    }

    {
        ecs_entity_t entity = ecs_new(world);
        assert(ecs_entity_from_json(world, entity, kTagNullJson, nullptr) != nullptr);
        assert(ecs_has_id(world, entity, test_tag_comp.id()));
        ecs_delete(world, entity);
    }

    {
        ecs_entity_t entity = ecs_new(world);
        assert(ecs_entity_from_json(world, entity, kTagArrayJson, nullptr) != nullptr);
        assert(ecs_has_id(world, entity, test_tag_comp.id()));
        ecs_delete(world, entity);
    }

    {
        ecs_entity_t entity = ecs_new(world);
        assert(ecs_entity_from_json(world, entity, kTagEmptyJson, nullptr) != nullptr);
        assert(ecs_has_id(world, entity, test_tag_comp.id()));
        ecs_delete(world, entity);
    }

    {
        ecs_entity_t entity = ecs_new(world);
        assert(ecs_entity_from_json(world, entity, kTagPairJson, nullptr) != nullptr);
        assert(ecs_has_pair(world, entity, test_pair_rel, test_pair_target));
        ecs_delete(world, entity);
    }

    {
        ecs_entity_t entity = ecs_new(world);
        assert(ecs_entity_from_json(world, entity, kTagPairMapJson, nullptr) != nullptr);
        assert(ecs_has_pair(world, entity, test_pair_rel, test_pair_target));
        ecs_delete(world, entity);
    }

    PulsePrefabRequest quad_request = pulse_load_prefab(app, "quad.prefab");
    assert(pulse_asset_request_is_valid(pulse_prefab_request_to_asset_request(quad_request)));
    assert(wait_prefab_ready(app, quad_request));

    PulsePrefabHandle quad_prefab = pulse_prefab_get_handle(app, quad_request);
    assert(quad_prefab.index != 0);
    ecs_entity_t quad_root = pulse_prefab_get_root(app, quad_prefab);
    assert(quad_root != 0 && ecs_is_alive(world, quad_root));
    assert(ecs_has_id(world, quad_root, EcsPrefab));
    assert(strcmp(ecs_get_name(world, quad_root), "QuadPrefab") == 0);

    const PulseRenderable* quad_renderable = ecs_get(world, quad_root, PulseRenderable);
    assert(quad_renderable != nullptr);
    assert(!handle_is_zero(quad_renderable->mesh));
    assert(!handle_is_zero(quad_renderable->material));

    const TestNoteHolder* quad_note = ecs_get(world, quad_root, TestNoteHolder);
    assert(quad_note != nullptr);
    assert(quad_note->note.value != nullptr);
    assert(strcmp(quad_note->note.value, "not_an_asset.obj") == 0);

    const PulseLocalTransform* quad_transform = ecs_get(world, quad_root, PulseLocalTransform);
    assert(quad_transform != nullptr);
    assert(quad_transform->translation.X == 1.0f && quad_transform->translation.Y == 2.0f && quad_transform->translation.Z == 3.0f);
    assert(quad_transform->rotation.W == 1.0f);

    assert(ecs_has_id(world, quad_root, test_tag_comp.id()));
    assert(!ecs_get_type_info(world, test_tag_comp.id()));

    assert(ecs_has_pair(world, quad_root, test_pair_rel, test_pair_target));
    assert(!ecs_get_type_info(world, ecs_pair(test_pair_rel, test_pair_target)));

    {
        ecs_id_t pair_id = ecs_pair(ecs_id(PulseLocalTransform), test_pair_target);
        const PulseLocalTransform* pair_transform = static_cast<const PulseLocalTransform*>(ecs_get_id(world, quad_root, pair_id));
        assert(pair_transform != nullptr);
        assert(pair_transform->translation.X == 9.0f && pair_transform->translation.Y == 8.0f && pair_transform->translation.Z == 7.0f);
    }

    PulseMeshRequest mesh_request = pulse_load_mesh(app, "Quad.obj");
    PulseMaterialRequest material_request = pulse_load_material(app, "quad.material");
    assert(pulse_mesh_is_ready(app, mesh_request));
    assert(pulse_material_is_ready(app, material_request));
    PulseMeshHandle mesh_handle = pulse_mesh_get_handle(app, mesh_request);
    PulseMaterialHandle material_handle = pulse_material_get_handle(app, material_request);
    assert(quad_renderable->mesh.index == mesh_handle.index && quad_renderable->mesh.generation == mesh_handle.generation);
    assert(quad_renderable->material.index == material_handle.index && quad_renderable->material.generation == material_handle.generation);

    {
        char* json = ecs_entity_to_json(world, quad_root, nullptr);
        assert(json != nullptr);
        assert(strstr(json, "\"mesh\":\"Quad.obj\"") != nullptr);
        assert(strstr(json, "\"material\":\"quad.material\"") != nullptr);
        ecs_os_free(json);
    }

    {
        ecs_entity_t instance = pulse_prefab_instantiate(app, quad_prefab);
        assert(instance != 0);
        assert(ecs_has_pair(world, instance, EcsIsA, quad_root));
        assert(!ecs_has_id(world, instance, EcsPrefab));
        assert(ecs_has_id(world, instance, test_tag_comp.id()));
        assert(ecs_has_pair(world, instance, test_pair_rel, test_pair_target));
        const PulseRenderable* instance_renderable = ecs_get(world, instance, PulseRenderable);
        assert(instance_renderable != nullptr);
        assert(instance_renderable->mesh.index == mesh_handle.index && instance_renderable->mesh.generation == mesh_handle.generation);
        assert(instance_renderable->material.index == material_handle.index && instance_renderable->material.generation == material_handle.generation);
    }

    {
        PulsePrefabRequest request = pulse_load_prefab(app, "missing.prefab");
        assert(wait_prefab_ready(app, request));
        ecs_entity_t root = pulse_prefab_get_root(app, pulse_prefab_get_handle(app, request));
        const PulseRenderable* renderable = ecs_get(world, root, PulseRenderable);
        assert(renderable != nullptr);
        assert(handle_is_zero(renderable->mesh));
        assert(handle_is_zero(renderable->material));
    }

    {
        PulsePrefabRequest request = pulse_load_prefab(app, "empty.prefab");
        assert(wait_prefab_ready(app, request));
        ecs_entity_t root = pulse_prefab_get_root(app, pulse_prefab_get_handle(app, request));
        const PulseRenderable* renderable = ecs_get(world, root, PulseRenderable);
        assert(renderable != nullptr);
        assert(handle_is_zero(renderable->mesh));
        assert(handle_is_zero(renderable->material));
    }

    {
        PulsePrefabRequest request = pulse_load_prefab(app, "unknown.prefab");
        assert(wait_prefab_ready(app, request));
        ecs_entity_t root = pulse_prefab_get_root(app, pulse_prefab_get_handle(app, request));
        const PulseRenderable* renderable = ecs_get(world, root, PulseRenderable);
        assert(renderable != nullptr);
        assert(handle_is_zero(renderable->mesh));
        assert(handle_is_zero(renderable->material));
    }

    {
        PulsePrefabRequest request = pulse_load_prefab(app, "mixed.prefab");
        assert(wait_prefab_ready(app, request));
        ecs_entity_t root = pulse_prefab_get_root(app, pulse_prefab_get_handle(app, request));
        const PulseRenderable* renderable = ecs_get(world, root, PulseRenderable);
        assert(renderable != nullptr);
        assert(renderable->mesh.index == mesh_handle.index && renderable->mesh.generation == mesh_handle.generation);
        assert(handle_is_zero(renderable->material));
    }

    {
        PulsePrefabRequest request = pulse_load_prefab(app, "broken.prefab");
        assert(pulse_asset_request_is_valid(pulse_prefab_request_to_asset_request(request)));
        assert(!wait_prefab_ready(app, request));
        assert(!pulse_prefab_is_alive(app, request));
        assert(pulse_asset_system_get_state(pulse_get_asset_system(app), pulse_prefab_request_to_asset_request(request)) == PULSE_ASSET_STATE_FAILED);
    }

    {
        PulsePrefabRequest request = pulse_load_prefab(app, "missing_file.prefab");
        assert(pulse_asset_request_is_valid(pulse_prefab_request_to_asset_request(request)));
        assert(!wait_prefab_ready(app, request));
        assert(!pulse_prefab_is_alive(app, request));
        assert(pulse_asset_system_get_state(pulse_get_asset_system(app), pulse_prefab_request_to_asset_request(request)) == PULSE_ASSET_STATE_FAILED);
    }

    {
        PulsePrefabRequest request = pulse_load_prefab(app, "tags.prefab");
        assert(wait_prefab_ready(app, request));
        ecs_entity_t root = pulse_prefab_get_root(app, pulse_prefab_get_handle(app, request));
        assert(ecs_has_id(world, root, test_tag_comp.id()));
    }

    {
        PulsePrefabRequest request = pulse_load_prefab(app, "pairs.prefab");
        assert(wait_prefab_ready(app, request));
        ecs_entity_t root = pulse_prefab_get_root(app, pulse_prefab_get_handle(app, request));
        assert(ecs_has_pair(world, root, test_pair_rel, test_pair_target));
    }

    assert_prefab_fails(app, "bad_section.prefab", "tgas");
    assert_prefab_fails(app, "bad_tag_pair.prefab", "write it in 'pairs'");
    assert_prefab_fails(app, "bad_tag_component.prefab", "write it in 'components'");

    {
        ecs_entity_t entity = ecs_new(world);
        assert(ecs_entity_from_json(world, entity, kRenderableJson, nullptr) != nullptr);
        const PulseRenderable* renderable = ecs_get(world, entity, PulseRenderable);
        assert(renderable != nullptr);
        assert(renderable->mesh.index == mesh_handle.index && renderable->mesh.generation == mesh_handle.generation);
        assert(renderable->material.index == material_handle.index && renderable->material.generation == material_handle.generation);
        ecs_delete(world, entity);
    }

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("prefab test passed\n");
    return 0;
}
