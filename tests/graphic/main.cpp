#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_window.h"
#include "pulse_cgpu_render.h"
#include "pulse_graphic.h"

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
    pulse_shader_t compute;
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

static void record_test_graphic(
    pulse_app_t app,
    pulse_rendergraph_t* graph,
    void* user_data
) {
    //auto* resources = static_cast<test_graphic_resources*>(user_data);
    //if (!graph || !resources) return;

    //// Declare a render target texture
    //pulse_texture_handle_t rt = pulse_rendergraph_declare_texture(graph);
    //pulse_rendergraph_texture_set_extent(graph, rt, 800, 600, 1);
    //pulse_rendergraph_texture_set_format(graph, rt, CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM);

    //// Add a render pass
    //pulse_renderpass_builder_t pass = pulse_rendergraph_add_renderpass(graph, "TestPass");
    //pulse_renderpass_add_color_attachment(
    //    &pass, rt,
    //    CGPU_LOAD_ACTION_CLEAR, 0x00000000,
    //    CGPU_STORE_ACTION_STORE);

    //// Set executable with passdata carrying the graphic handles
    //test_render_passdata* passdata = nullptr;
    //pulse_renderpass_set_executable(
    //    &pass, on_test_render,
    //    sizeof(test_render_passdata),
    //    reinterpret_cast<void**>(&passdata));
    //if (passdata) {
    //    passdata->material = resources->material;
    //    passdata->mesh = resources->mesh;
    //    passdata->shader = resources->shader;
    //    passdata->compute = resources->compute;
    //    passdata->texture = resources->texture;
    //    passdata->buffer = resources->buffer;
    //}

    //pulse_rendergraph_present(graph, rt);
}

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    // Add required plugins
    auto window_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_desc) == PULSE_OK);

    auto cgpu_desc = pulse_cgpu_render_plugin_desc_default();
    assert(pulse_cgpu_render_add_plugin(app, &cgpu_desc) == PULSE_OK);

    pulse_asset_plugin_desc asset_desc = pulse_asset_plugin_desc_default();
    asset_desc.root_path = "tests/graphic/data";
    assert(pulse_asset_add_plugin(app, &asset_desc) == PULSE_OK);

    // Add pulse_graphic plugin
    auto graphic_desc = pulse_graphic_plugin_desc_default();
    assert(pulse_graphic_add_plugin(app, &graphic_desc) == PULSE_OK);
    assert(pulse_app_has_plugin(app, "PulseGraphicPlugin"));

    //// ---- Create resources ----
    //pulse_shader_t shader = pulse_graphic_shader_create_from_binary(
    //    app, dummy_spv, sizeof(dummy_spv), dummy_spv, sizeof(dummy_spv),
    //    nullptr, nullptr, nullptr);

    //pulse_shader_t compute = pulse_graphic_compute_shader_create_from_binary(
    //    app, dummy_spv, sizeof(dummy_spv));

    //CGPUBufferDescriptor buf_desc{};
    //buf_desc.size = 256;
    //buf_desc.descriptors = CGPU_RESOURCE_TYPE_UNIFORM_BUFFER;
    //pulse_buffer_t buffer = pulse_graphic_buffer_create(app, &buf_desc, nullptr, 0);

    //CGPUSamplerDescriptor smp_desc{};
    //pulse_sampler_t sampler = pulse_graphic_sampler_create(app, &smp_desc);

    //CGPUTextureDescriptor tex_desc{};
    //tex_desc.width = 64;
    //tex_desc.height = 64;
    //tex_desc.depth = 1;
    //tex_desc.mip_levels = 1;
    //tex_desc.format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    pulse_texture_t texture = pulse_graphic_texture_load(
        app, "TilesGray512.jpg", false);

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

    pulse_mesh_t mesh = pulse_graphic_mesh_load(
        app, "Quad.obj");

    //pulse_material_t material = pulse_graphic_material_create(app, shader);
    //pulse_graphic_material_bind_buffer(app, &material, 0, 0, buffer);
    //pulse_graphic_material_bind_texture(app, &material, 0, 1, texture);
    //pulse_graphic_material_bind_sampler(app, &material, 0, 2, sampler);

    //// ---- Acquire/release cycle ----
    //pulse_shader_data_t* shader_data = pulse_graphic_shader_acquire(app, &shader);
    //if (shader_data) pulse_graphic_shader_release(app, &shader);
    //pulse_graphic_is_available(app, shader);

    //pulse_compute_shader_data_t* cs_data = pulse_graphic_compute_shader_acquire(app, &compute);
    //if (cs_data) pulse_graphic_shader_release(app, &compute);

    //pulse_buffer_data_t* buf_data = pulse_graphic_buffer_acquire(app, &buffer);
    //if (buf_data) pulse_graphic_buffer_release(app, &buffer);

    //pulse_sampler_data_t* smp_data = pulse_graphic_sampler_acquire(app, &sampler);
    //if (smp_data) pulse_graphic_sampler_release(app, &sampler);

    //pulse_texture_data_t* tex_data = pulse_graphic_texture_acquire(app, &texture);
    //if (tex_data) pulse_graphic_texture_release(app, &texture);

    //pulse_mesh_data_t* mesh_data = pulse_graphic_mesh_acquire(app, &mesh);
    //if (mesh_data) pulse_graphic_mesh_release(app, &mesh);

    //pulse_material_data_t* mat_data = pulse_graphic_material_acquire(app, &material);
    //if (mat_data) pulse_graphic_material_release(app, &material);

    //// ---- Dynamic mesh update ----
    //pulse_mesh_t dyn_mesh = pulse_graphic_mesh_create_dynamic(
    //    app, 1024, 12, 3072, sizeof(uint16_t),
    //    CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &vtx_layout);
    //pulse_graphic_mesh_update_vertices(app, &dyn_mesh, verts, 3);
    //pulse_graphic_mesh_update_indices(app, &dyn_mesh, idxs, 3);
    //pulse_mesh_data_t* dyn_data = pulse_graphic_mesh_acquire(app, &dyn_mesh);
    //if (dyn_data) pulse_graphic_mesh_release(app, &dyn_mesh);

    // ---- Register record callback with graphic resources ----
    //test_graphic_resources resources{shader, compute, buffer, sampler, texture, mesh, material};
    pulse_cgpu_renderer_record_callback_desc cb_desc{};
    cb_desc.callback = record_test_graphic;
    //cb_desc.user_data = &resources;
    cb_desc.priority = 0;
    pulse_cgpu_render_add_record_callback(app, &cb_desc);

    pulse_app_run(app);

    pulse_app_destroy(app);

    printf("Graphic module tests passed!\n");
    return 0;
}
