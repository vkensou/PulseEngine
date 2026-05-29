#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_window.h"
#include "pulse_graphics.h"

static uint8_t dummy_spv[16] = {0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

struct test_graphic_resources {
    pulse_shader_t shader;
    pulse_shader_t compute;
    pulse_buffer_t buffer;
    pulse_sampler_t sampler;
    pulse_texture_t texture;
    pulse_mesh_t mesh;
    pulse_material_t material;
};

// passdata 传入 executable callback
struct test_render_passdata {
    pulse_material_t material;
    pulse_mesh_t mesh;
    pulse_shader_t shader;
    pulse_compute_shader_t compute;
    pulse_texture_t texture;
    pulse_buffer_t buffer;
};

static void on_test_render(pulse_renderpass_encoder_t* encoder, void* userdata) {
    auto* data = static_cast<test_render_passdata*>(userdata);
    if (!encoder) return;

    pulse_encoder_set_viewport(encoder, 0, 0, 800, 600, 0, 1);
    pulse_encoder_set_scissor(encoder, 0, 0, 800, 600);
    pulse_encoder_set_global_texture_handle(encoder, pulse_texture_handle_t{}, 0, 0);
    pulse_encoder_set_global_buffer_handle(encoder, pulse_buffer_handle_t{}, 0, 0);
    pulse_encoder_set_global_buffer_offset(encoder, pulse_buffer_handle_t{}, 0, 0, 0, 256);
    pulse_encoder_push_constants(encoder, data->shader, "test", nullptr);
    pulse_encoder_draw(encoder, data->material, data->mesh);
    pulse_encoder_draw_submesh(encoder, data->material, data->mesh, 3, 0, 3, 0);
    pulse_encoder_draw_procedure(encoder, data->material, CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
    pulse_encoder_dispatch(encoder, data->compute, 1, 1, 1);
    pulse_encoder_set_global_texture(encoder, data->texture, 0, 0);
    pulse_encoder_set_global_buffer(encoder, data->buffer, 0, 0);
    pulse_encoder_set_global_sampler(encoder, pulse_sampler_t{}, 0, 0);
}

struct test_render_state {
    ecs_query_t* window_query;
};

static void record_test_graphic(
    pulse_app_t app,
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

    ecs_iter_t it = ecs_query_iter(state->window_query->world, state->window_query);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; ++i) {
            ecs_entity_t entity = it.entities[i];

            pulse_texture_handle_t target_handle =
                pulse_graphics_render_import_window_backbuffer(app, graph, entity);
            if (!pulse_rendergraph_texture_handle_valid(target_handle)) {
                continue;
            }

            pulse_renderpass_builder_t pass =
                pulse_rendergraph_add_renderpass(graph, "TestCallbackPass");
            pulse_renderpass_add_color_attachment(
                &pass,
                target_handle,
                CGPU_LOAD_ACTION_CLEAR,
                0xff00ffff,
                CGPU_STORE_ACTION_STORE
            );
            pulse_rendergraph_present(graph, target_handle);
        }
    }
}

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    // Add required plugins
    auto window_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_desc) == PULSE_OK);

    pulse_asset_plugin_desc asset_desc = pulse_asset_plugin_desc_default();
    asset_desc.root_path = "tests/graphics/data";
    assert(pulse_asset_add_plugin(app, &asset_desc) == PULSE_OK);

    // Add pulse_graphic plugin
    auto graphic_desc = pulse_graphics_plugin_desc_default();
    assert(pulse_graphics_add_plugin(app, &graphic_desc) == PULSE_OK);
    assert(pulse_app_has_plugin(app, "PulseGraphicPlugin"));

    //// ---- Create resources ----
    pulse_graphics_shader_create_from_file_desc shader_desc = {
        .vert_path = "color.vert.spv",
        .frag_path = "color.frag.spv",
    };
    pulse_shader_t shader = pulse_graphics_shader_create_from_file(app, &shader_desc);

    //pulse_shader_t compute = pulse_graphics_compute_shader_create_from_binary(
    //    app, dummy_spv, sizeof(dummy_spv));

    //CGPUBufferDescriptor buf_desc{};
    //buf_desc.size = 256;
    //buf_desc.descriptors = CGPU_RESOURCE_TYPE_UNIFORM_BUFFER;
    //pulse_buffer_t buffer = pulse_graphics_buffer_create(app, &buf_desc, nullptr, 0);

    //CGPUSamplerDescriptor smp_desc{};
    //pulse_sampler_t sampler = pulse_graphics_sampler_create(app, &smp_desc);

	pulse_graphics_texture_load_desc tex_load_desc{
        .filepath = "TilesGray512.jpg",
		.generate_mipmaps = true,
    };
    pulse_texture_t texture = pulse_graphics_texture_load(
        app, &tex_load_desc);

    std::vector<uint32_t> pixels = { 0xFF00FFFF };

    pulse_graphics_texture_create_desc tex_create_desc
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

    pulse_texture_t texture2 = pulse_graphics_texture_create(
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

    pulse_mesh_t mesh = pulse_graphics_mesh_load(
        app, "Quad.obj");

	pulse_graphics_material_create_desc mat_desc{
		.shader = shader,
    };
    pulse_material_t material = pulse_graphics_material_create(app, &mat_desc);

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
    window_query_desc.terms[0] = { .id = ecs_id(pulse_window) };
    window_query_desc.cache_kind = EcsQueryCacheAuto;
    render_state.window_query = ecs_query_init(pulse_app_world(app), &window_query_desc);

    pulse_graphics_renderer_record_callback_desc cb_desc{};
    cb_desc.callback = record_test_graphic;
    cb_desc.user_data = &render_state;
    cb_desc.priority = 0;
    pulse_graphics_render_add_record_callback(app, &cb_desc);

    pulse_app_run(app);

    pulse_graphics_material_ref material_ref;
    pulse_graphics_material_acquire(app, material, &material_ref);
    pulse_graphics_texture_ref texture_ref;
    pulse_graphics_texture_acquire(app, texture, &texture_ref);
    pulse_graphics_material_bind_texture(&material_ref, 0, 1, &texture_ref);
    //pulse_graphics_material_bind_sampler(app, &material, 0, 2, sampler);
    //pulse_graphics_material_bind_buffer(app, &material, 0, 0, buffer);

    ecs_query_fini(render_state.window_query);

    pulse_app_destroy(app);

    printf("Graphic module tests passed!\n");
    return 0;
}
