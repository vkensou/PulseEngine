#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_window.h"
#include "pulse_graphics.h"
#include "HandmadeMath.h"

static uint8_t dummy_spv[16] = {0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
struct PassData
{
    HMM_Mat4	vpMatrix;
};

struct MaterialData
{
    HMM_Vec4	albedo;
};

struct ObjectData
{
    HMM_Mat4	wMatrix;
};

struct test_graphic_resources {
    PulseShaderHandle shader;
    PulseShaderHandle compute;
    PulseBufferHandle buffer;
    PulseSamplerHandle sampler;
    PulseTextureHandle texture;
    PulseMeshHandle mesh;
    PulseMaterialHandle material;
};

// passdata 传入 executable callback
struct test_render_passdata {
    PulseMaterial material;
    PulseMesh mesh;
    PulseShader shader;
    PulseComputeShader compute;
    PulseTexture texture;
    PulseBuffer buffer;
};

static void on_test_render(pulse_renderpass_encoder_t* encoder, void* userdata) {
    auto* data = static_cast<test_render_passdata*>(userdata);
    if (!encoder) return;

    pulse_graphics_encoder_set_viewport(encoder, 0, 0, 800, 600, 0, 1);
    pulse_graphics_encoder_set_scissor(encoder, 0, 0, 800, 600);
    pulse_graphics_encoder_set_global_texture_handle(encoder, pulse_texture_handle_t{}, 0, 0);
    pulse_graphics_encoder_set_global_buffer_handle(encoder, pulse_buffer_handle_t{}, 0, 0);
    pulse_graphics_encoder_set_global_buffer_offset(encoder, pulse_buffer_handle_t{}, 0, 0, 0, 256);
    pulse_graphics_encoder_push_constants(encoder, data->shader, "test", nullptr);
    pulse_graphics_encoder_draw(encoder, data->material, data->mesh);
    pulse_graphics_encoder_draw_submesh(encoder, data->material, data->mesh, 3, 0, 3, 0);
    pulse_graphics_encoder_draw_procedure(encoder, data->material, CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
    pulse_graphics_encoder_dispatch(encoder, data->compute, 1, 1, 1);
    pulse_graphics_encoder_set_global_texture(encoder, data->texture, 0, 0);
    pulse_graphics_encoder_set_global_buffer(encoder, data->buffer, 0, 0);
    pulse_graphics_encoder_set_global_sampler(encoder, {}, 0, 0);
}

struct test_render_state {
    ecs_query_t* window_query;
    PulseMaterialHandle material;
    PulseTextureHandle texture;
    PulseMeshHandle mesh;
    PulseMaterial material_ref;
    PulseMesh mesh_ref;
    HMM_Mat4 viewMat;
    PassData passData;
    ObjectData objectData;
};

static void record_test_graphic(
    PulseAppId app,
    pulse_rendergraph_t* graph,
    void* user_data
) {
    test_render_state* state = static_cast<test_render_state*>(user_data);
    if (!graph || !state) {
        return;
    }

    if (!state->window_query) {
        return;
    }

    if (state->material_ref.handle.index == 0 && pulse_graphics_material_is_ready(app, state->material) && pulse_graphics_texture_is_ready(app, state->texture)) {
        pulse_graphics_material_acquire(app, state->material, &state->material_ref);
        PulseTexture texture_ref;
        pulse_graphics_texture_acquire(app, state->texture, &texture_ref);
        pulse_graphics_material_bind_texture(state->material_ref, 0, 1, texture_ref);
        pulse_graphics_texture_release(app, &texture_ref);

        auto materialData = MaterialData{
            .albedo = HMM_V4(1, 0, 0, 1),
        };
        pulse_graphics_material_bind_data(state->material_ref, 1, 0, sizeof(MaterialData), &materialData);
    }

    if (state->mesh_ref.handle.index == 0 && pulse_graphics_mesh_is_ready(app, state->mesh)) {
        pulse_graphics_mesh_acquire(app, state->mesh, &state->mesh_ref);
    }

    ecs_iter_t it = ecs_query_iter(state->window_query->world, state->window_query);
    while (ecs_query_next(&it)) {
        PulseWindow* windows = ecs_field(&it, PulseWindow, 0);
        for (int i = 0; i < it.count; ++i) {
            ecs_entity_t entity = it.entities[i];
            const auto& window = windows[i];

            pulse_texture_handle_t target_handle =
                pulse_graphics_render_import_window_backbuffer(app, graph, entity);
            if (!pulse_rendergraph_texture_handle_valid(target_handle)) {
                continue;
            }

            int width = window.width;
            int height = window.height;
            float aspect = (float)width / height;
            float near = 0.1;
            float far = 1000;
            float fov = 45;
            auto proj = HMM_Perspective_LH_RO(fov * HMM_DegToRad, aspect, near, far);
            auto vpMat = proj * state->viewMat;
            state->passData = { vpMat };

            auto objectMat = HMM_Translate(HMM_V3(0, 0, 0));
            state->objectData = { objectMat };

            auto pass_ubo_handle = pulse_rendergraph_declare_uniform_buffer_quick(graph, sizeof(PassData), &state->passData);
            auto object_ubo_handle = pulse_rendergraph_declare_uniform_buffer_quick(graph, sizeof(ObjectData), &state->objectData);

            pulse_renderpass_builder_t pass =
                pulse_rendergraph_add_renderpass(graph, "TestCallbackPass");
            pulse_renderpass_add_color_attachment(
                &pass,
                target_handle,
                CGPU_LOAD_ACTION_CLEAR,
                0xff00ffff,
                CGPU_STORE_ACTION_STORE
            );

            if (state->material_ref.handle.index == 0 || state->mesh_ref.handle.index == 0) {
                continue;
            }

            pulse_renderpass_use_buffer(&pass, pass_ubo_handle);
            pulse_renderpass_use_buffer(&pass, object_ubo_handle);

            struct MainPassPassData
            {
                PulseMaterial material_ref;
                PulseMesh mesh_ref;
                pulse_buffer_handle_t pass_ubo_handle;
                pulse_buffer_handle_t object_ubo_handle;
            };
            MainPassPassData* passdata;
            pulse_renderpass_set_executable(&pass, [](pulse_renderpass_encoder_t* encoder, void* passdata)
                {
                    MainPassPassData* resolved_passdata = (MainPassPassData*)passdata;
                    pulse_graphics_encoder_set_global_buffer_handle(encoder, resolved_passdata->pass_ubo_handle, 0, 0);
                    pulse_graphics_encoder_set_global_buffer_offset(encoder, resolved_passdata->object_ubo_handle, 2, 0, 0, sizeof(ObjectData));
                    pulse_graphics_encoder_draw(encoder, resolved_passdata->material_ref, resolved_passdata->mesh_ref);
                    //set_global_dynamic_buffer(encoder, resolved_passdata->pass_ubo_handle, 0, 0);
                    //for (size_t i = 0; i < resolved_passdata->view->renderObjects.size(); ++i)
                    //{
                    //    auto& obj = resolved_passdata->view->renderObjects[i];
                    //    set_global_buffer_with_offset_size(encoder, resolved_passdata->object_ubo_handle, 2, 0, i * sizeof(ObjectData), sizeof(ObjectData));
                    //    draw(encoder, resolved_passdata->resourceManager->materials[obj.material], resolved_passdata->resourceManager->meshes[obj.mesh]);
                    //}
                }, sizeof(MainPassPassData), (void**)&passdata);
            //passdata->resourceManager = &world.get<ResourceManager>();
            //passdata->view = &view;
            passdata->material_ref = state->material_ref;
            passdata->mesh_ref = state->mesh_ref;
            passdata->pass_ubo_handle = pass_ubo_handle;
            passdata->object_ubo_handle = object_ubo_handle;

            pulse_rendergraph_present(graph, target_handle);
        }
    }
}

int main(void) {
    PulseAppId app = pulse_create_app("test-graphics");
    assert(app != nullptr);

    // Add required plugins
    auto window_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_desc) == PULSE_RESULT_OK);

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    asset_desc.root_path = "tests/graphics/data";
    assert(pulse_asset_add_plugin(app, &asset_desc) == PULSE_RESULT_OK);

    // Add pulse_graphic plugin
    auto graphic_desc = pulse_graphics_plugin_desc_default();
    graphic_desc.enable_debug_layer = true;
    graphic_desc.enable_gpu_based_validation = true;
    assert(pulse_graphics_add_plugin(app, &graphic_desc) == PULSE_RESULT_OK);
    assert(pulse_app_has_plugin(app, "PulseGraphicPlugin"));

    //// ---- Create resources ----
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
    PulseGraphicsShaderCreateFromFileDesc shader_desc = {
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
        }
    };
    PulseShaderHandle shader = pulse_graphics_shader_create_from_file(app, &shader_desc);

    //pulse_shader_t compute = pulse_graphics_compute_shader_create_from_binary(
    //    app, dummy_spv, sizeof(dummy_spv));

    //CGPUBufferDescriptor buf_desc{};
    //buf_desc.size = 256;
    //buf_desc.descriptors = CGPU_RESOURCE_TYPE_UNIFORM_BUFFER;
    //pulse_buffer_t buffer = pulse_graphics_buffer_create(app, &buf_desc, nullptr, 0);

    //CGPUSamplerDescriptor smp_desc{};
    //pulse_sampler_t sampler = pulse_graphics_sampler_create(app, &smp_desc);

	PulseGraphicsTextureLoadDesc tex_load_desc{
        .filepath = "TilesGray512.jpg",
		.generate_mipmaps = true,
    };
    PulseTextureHandle texture = pulse_graphics_texture_load(
        app, &tex_load_desc);

    std::vector<uint32_t> pixels = { 0xFF00FFFF };

    PulseGraphicsTextureCreateDesc tex_create_desc
    {
        .desc = {
            .name = "create_texture",
            .width = 1,
            .height = 1,
            .depth = 1,
            .array_size = 1,
            .format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM,
            .mip_levels = 1,
            .descriptors = CGPU_RESOURCE_TYPE_TEXTURE,
        },
		.pixel_data_size = sizeof(uint32_t),
		.pixel_data = pixels.data(),
    };

    PulseTextureHandle texture2 = pulse_graphics_texture_create(
        app, &tex_create_desc);

    //float verts[] = {0,0,0, 1,0,0, 0,1,0};
    //uint16_t idxs[] = {0, 1, 2};
    //CGPUVertexLayout vtx_layout{};
    //CGPUVertexAttribute attr{};
    //attr.semantic_name = "POSITION";
    //attr.format = CGPU_VERTEX_FORMAT_FLOAT32X3;
    //attr.offset = 0;
    //attr.binding = 0;
    //attr.array_size = 0;
    //attr.elem_stride = 0;
    //attr.rate = CGPU_VERTEX_INPUT_RATE_VERTEX;
    //vtx_layout.p_attributes = &attr;
    //vtx_layout.attribute_count = 1;

    PulseMeshHandle mesh = pulse_graphics_mesh_load(
        app, "Quad.obj");

	PulseGraphicsMaterialCreateDesc mat_desc{
		.shader = shader,
    };
    PulseMaterialHandle material = pulse_graphics_material_create(app, &mat_desc);

    //// ---- Acquire/release cycle ----
    //pulse_shader_data_t* shader_data = pulse_graphics_shader_acquire(app, &shader);
    //if (shader_data) pulse_graphics_shader_release(app, &shader);
    //pulse_graphics_is_available(app, shader);

    //pulse_compute_shader_data_t* cs_data = pulse_graphics_compute_shader_acquire(app, &compute);
    //if (cs_data) pulse_graphics_shader_release(app, &compute);

    //pulse_buffer_data_t* buf_data = pulse_graphics_buffer_acquire(app, &buffer);
    //if (buf_data) pulse_graphics_buffer_release(app, &buffer);

    //pulse_sampler_data_t* smp_data = pulse_graphics_sampler_acquire(app, &sampler);
    //if (smp_data) pulse_graphics_sampler_release(app, &sampler);

    //pulse_texture_data_t* tex_data = pulse_graphics_texture_acquire(app, &texture);
    //if (tex_data) pulse_graphics_texture_release(app, &texture);

    //pulse_mesh_data_t* mesh_data = pulse_graphics_mesh_acquire(app, &mesh);
    //if (mesh_data) pulse_graphics_mesh_release(app, &mesh);

    //pulse_material_data_t* mat_data = pulse_graphics_material_acquire(app, &material);
    //if (mat_data) pulse_graphics_material_release(app, &material);

    //// ---- Dynamic mesh update ----
    //pulse_mesh_t dyn_mesh = pulse_graphics_mesh_create_dynamic(
    //    app, 1024, 12, 3072, sizeof(uint16_t),
    //    CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &vtx_layout);
    //pulse_graphics_mesh_update_vertices(app, &dyn_mesh, verts, 3);
    //pulse_graphics_mesh_update_indices(app, &dyn_mesh, idxs, 3);
    //pulse_mesh_data_t* dyn_data = pulse_graphics_mesh_acquire(app, &dyn_mesh);
    //if (dyn_data) pulse_graphics_mesh_release(app, &dyn_mesh);

    // ---- Register record callback with graphic resources ----
    //test_graphic_resources resources{shader, compute, buffer, sampler, texture, mesh, material};

    test_render_state render_state{};

    ecs_query_desc_t window_query_desc{};
    window_query_desc.terms[0] = { .id = ecs_id(PulseWindow) };
    window_query_desc.cache_kind = EcsQueryCacheAuto;
    render_state.window_query = ecs_query_init(pulse_app_world(app), &window_query_desc);
    render_state.material = material;
    render_state.texture = texture;
    render_state.mesh = mesh;

    auto cameraParentMat = HMM_M4_Identity;
    auto cameraLocalMat = HMM_Translate(HMM_V3(0 + 0.5, 0 + 0.5, -38));
    auto cameraMat = HMM_Mul(cameraParentMat, cameraLocalMat);
    auto eye = HMM_M4GetTranslate(cameraMat);
    auto forward = HMM_M4GetForward(cameraMat);
    (void)forward;
    render_state.viewMat = HMM_LookAt2_LH(eye, forward, HMM_V3_Up);

    PulseGraphicsRendererRecordCallbackDesc cb_desc{};
    cb_desc.callback = record_test_graphic;
    cb_desc.user_data = &render_state;
    cb_desc.priority = 0;
    pulse_graphics_render_add_record_callback(app, &cb_desc);

    pulse_app_update(app);
    pulse_app_update(app);
    pulse_app_update(app);
    pulse_app_update(app);
    pulse_app_update(app);
    pulse_app_update(app);

    //pulse_graphics_material_bind_sampler(app, &material, 0, 2, sampler);
    //pulse_graphics_material_bind_buffer(app, &material, 0, 0, buffer);

    pulse_app_run(app);

    ecs_query_fini(render_state.window_query);

    pulse_destroy_app(app);

    printf("Graphic module tests passed!\n");
    return 0;
}
