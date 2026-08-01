#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_input.h"
#include "pulse_window.h"
#include "pulse_graphics.h"
#include "pulse_math.h"
#include "pulse_transform.h"
#include "pulse_renderer.h"

// ============================================================
// Test: Verify pulse_renderer plugin registration and ECS systems
// ============================================================

// Simple helper to create a transform entity
static ecs_entity_t create_transform_entity(
    ecs_world_t* world,
    float x, float y, float z)
{
    ecs_entity_t entity = ecs_new(world);

    PulseLocalTransform local = {};
    local.translation = HMM_Vec3{ x, y, z };
    local.rotation = HMM_Quat{ 0, 0, 0, 1 };
    local.scale = HMM_Vec3{ 1, 1, 1 };
    ecs_set_ptr(world, entity, PulseLocalTransform, &local);

    // Also explicitly initialize WorldTransform to identity,
    // so it's valid even if propagation hasn't run yet.
    PulseWorldTransform world_tx = {};
    world_tx.value = HMM_TRS(local.translation, local.rotation, local.scale);
    ecs_set_ptr(world, entity, PulseWorldTransform, &world_tx);

    return entity;
}

int main(void) {
    // ---- Create app ----
    PulseAppId app = pulse_create_app("test-renderer");
    assert(app != nullptr);

    // ---- Add plugins ----
    // input plugin (required by window)
    assert(pulse_add_input_plugin(app) == PULSE_RESULT_OK);

    // window plugin
    auto window_desc = pulse_window_plugin_desc_default();
    window_desc.primary_window.width = 800;
    window_desc.primary_window.height = 600;
    assert(pulse_add_window_plugin(app, &window_desc) == PULSE_RESULT_OK);

    // asset plugin
    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    // Use test data from graphics test
    asset_desc.root_path = "tests/graphics/data";
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_RESULT_OK);

    // transform plugin
    assert(pulse_add_transform_plugin(app) == PULSE_RESULT_OK);

    // graphics plugin
    auto graphic_desc = pulse_graphics_plugin_desc_default();
    graphic_desc.enable_debug_layer = true;
    graphic_desc.enable_gpu_based_validation = true;
    assert(pulse_add_graphics_plugin(app, &graphic_desc) == PULSE_RESULT_OK);
    assert(pulse_app_has_plugin(app, "PulseGraphicPlugin"));

    // ---- Load resources ----
    // Create a shader
    CGPUBlendAttachmentState blend_attachments = {
        .enable = false,
        .src_factor = CGPU_BLEND_FACTOR_ONE,
        .dst_factor = CGPU_BLEND_FACTOR_ZERO,
        .src_alpha_factor = CGPU_BLEND_FACTOR_ONE,
        .dst_alpha_factor = CGPU_BLEND_FACTOR_ZERO,
        .blend_op = CGPU_BLEND_OP_ADD,
        .blend_alpha_op = CGPU_BLEND_OP_ADD,
        .color_mask = CGPU_COLOR_MASK_RGBA,
    };

    PulseShaderPropertyDesc shader_props[] = {
        { .name = "vpMatrix", .type = PULSE_SHADER_PROPERTY_TYPE_MAT4,   .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL, .set = 0, .binding = 0, .offset = 0, .size = 64 },
        { .name = "albedo",   .type = PULSE_SHADER_PROPERTY_TYPE_FLOAT4, .role = PULSE_SHADER_PROPERTY_ROLE_MATERIAL,     .set = 1, .binding = 0, .offset = 0, .size = 16 },
        { .name = "wMatrix",  .type = PULSE_SHADER_PROPERTY_TYPE_MAT4,   .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL, .set = 2, .binding = 0, .offset = 0, .size = 64 },
    };

    PulseShaderCreateFromFileDesc shader_desc = {
        .vert_path = "color.vert.spv",
        .frag_path = "color.frag.spv",
        .blend_desc = {
            .attachment_count = 1,
            .p_attachments = &blend_attachments,
            .alpha_to_coverage = false,
            .independent_blend = false,
        },
        .depth_desc = {
            .depth_test = true,
            .depth_write = true,
            .depth_op = CGPU_COMPARE_OP_GREATER_EQUAL,
            .stencil_test = false,
        },
        .rasterizer_state = {
            .cull_mode = CGPU_CULL_MODE_BACK,
            .front_face = CGPU_FRONT_FACE_CLOCK_WISE,
        },
        .property_count = 3,
        .p_properties = shader_props,
    };
    PulseShaderHandle shader = pulse_create_shader_from_file(app, &shader_desc);

    // Load mesh
    PulseMeshHandle mesh = pulse_load_mesh(app, "Quad.obj");

    // Create material
    PulseMaterialCreateDesc mat_desc = {
        .shader = shader,
    };
    PulseMaterialHandle material = pulse_create_material(app, &mat_desc);

    // ---- Add renderer plugin ----
    assert(pulse_add_renderer_plugin(app) == PULSE_RESULT_OK);
    assert(pulse_app_has_plugin(app, "PulseRendererPlugin"));

    // ---- Wait for material to be ready, then bind its data ----
    // Run a few updates to let async assets load
    for (int i = 0; i < 5; ++i) {
        pulse_app_update(app);
    }

    // Bind material color via property name (replaces manual set/binding)
    if (pulse_material_is_ready(app, material)) {
        PulseMaterial mat_ref = {};
        if (pulse_acquire_material(app, material, &mat_ref)) {
            pulse_material_set_float4(&mat_ref, "albedo", HMM_V4(1.0f, 0.0f, 0.0f, 1.0f));
            pulse_release_material(app, &mat_ref);
        }
    }

    // ---- Create ECS entities ----
    ecs_world_t* world = pulse_app_world(app);

    ecs_query_desc_t window_query_desc{};
    window_query_desc.terms[0] = { .id = ecs_id(PulseWindow) };
    window_query_desc.cache_kind = EcsQueryCacheAuto;
    ecs_query_t* window_query = ecs_query_init(world, &window_query_desc);
    ecs_entity_t finded_window = 0;
    ecs_iter_t it = ecs_query_iter(world, window_query);
    while (ecs_query_next(&it)) {
        if (finded_window == 0)
            finded_window = it.entities[0];
    }
    assert(finded_window != 0);
    ecs_query_fini(window_query);

    // Create a camera entity
    ecs_entity_t camera_entity = create_transform_entity(world, 0 + 0.5, 0 + 0.5, -38);
    PulseCamera camera = {};
    camera.window_entity = finded_window;  // Will be set after window is created
    camera.fov = 45.0f;
    camera.near_plane = 0.1f;
    camera.far_plane = 1000.0f;
    ecs_set_ptr(world, camera_entity, PulseCamera, &camera);

    {
        // Create a renderable entity
        ecs_entity_t renderable_entity = create_transform_entity(world, 10, 0, 0);
        PulseRenderable renderable = {};
        renderable.mesh = mesh;
        renderable.material = material;
        ecs_set_ptr(world, renderable_entity, PulseRenderable, &renderable);
    }

    {
        // Create a renderable entity
        ecs_entity_t renderable_entity = create_transform_entity(world, -10, 5, 0);
        PulseRenderable renderable = {};
        renderable.mesh = mesh;
        renderable.material = material;
        ecs_set_ptr(world, renderable_entity, PulseRenderable, &renderable);
    }

    // Create a light entity (optional)
    ecs_entity_t light_entity = ecs_new(world);
    PulseLight light = {};
    light.color = HMM_Vec4{ 1.0f, 1.0f, 1.0f, 1.0f };
    ecs_set_ptr(world, light_entity, PulseLight, &light);

     pulse_app_run(app);

    // ---- Verify ECS component queries ----
    // Camera should exist
    ecs_query_desc_t cam_qdesc = {};
    cam_qdesc.terms[0].id = ecs_id(PulseCamera);
    ecs_query_t* camera_query = ecs_query_init(world, &cam_qdesc);
    int camera_count = 0;
    it = ecs_query_iter(world, camera_query);
    while (ecs_query_next(&it)) {
        camera_count += it.count;
    }
    assert(camera_count == 1);
    ecs_query_fini(camera_query);

    // Renderable should exist
    ecs_query_desc_t ren_qdesc = {};
    ren_qdesc.terms[0].id = ecs_id(PulseRenderable);
    ecs_query_t* renderable_query = ecs_query_init(world, &ren_qdesc);
    int renderable_count = 0;
    it = ecs_query_iter(world, renderable_query);
    while (ecs_query_next(&it)) {
        renderable_count += it.count;
    }
    assert(renderable_count == 2);
    ecs_query_fini(renderable_query);

    // ---- Cleanup ----
    pulse_destroy_app(app);

    printf("pulse_renderer tests passed!\n");
    return 0;
}
