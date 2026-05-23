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

static void record_test_graphic(
    pulse_app_t app,
    pulse_rendergraph_t* graph,
    void* user_data
) {
    (void)app; (void)graph; (void)user_data;
}

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    // Add required plugins
    pulse_asset_plugin_desc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_asset_add_plugin(app, &asset_desc) == PULSE_OK);

    auto window_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_desc) == PULSE_OK);

    auto cgpu_desc = pulse_cgpu_render_plugin_desc_default();
    assert(pulse_cgpu_render_add_plugin(app, &cgpu_desc) == PULSE_OK);

    // Register a record callback so the render pipeline doesn't stall
    pulse_cgpu_render_set_record_callback(app, record_test_graphic, nullptr);

    // Add pulse_graphic plugin
    auto graphic_desc = pulse_graphic_plugin_desc_default();
    assert(graphic_desc.struct_size == sizeof(pulse_graphic_plugin_desc));
    assert(graphic_desc.version == PULSE_GRAPHIC_PLUGIN_DESC_VERSION);
    assert(pulse_graphic_add_plugin(app, &graphic_desc) == PULSE_OK);
    assert(pulse_app_has_plugin(app, "PulseGraphicPlugin"));

    // ---- Shader ----
    pulse_shader_t shader = pulse_graphic_shader_create_from_binary(
        app, dummy_spv, sizeof(dummy_spv), dummy_spv, sizeof(dummy_spv),
        nullptr, nullptr, nullptr);
    pulse_shader_data_t* shader_data = pulse_graphic_shader_acquire(app, &shader);
    if (shader_data) {
        pulse_graphic_shader_release(app, &shader);
    }
    pulse_graphic_is_available(app, shader);

    // ---- Compute Shader ----
    pulse_shader_t compute = pulse_graphic_compute_shader_create_from_binary(
        app, dummy_spv, sizeof(dummy_spv));
    pulse_compute_shader_data_t* cs_data = pulse_graphic_compute_shader_acquire(app, &compute);
    if (cs_data) {
        pulse_graphic_shader_release(app, &compute);
    }

    // ---- Buffer ----
    CGPUBufferDescriptor buf_desc{};
    buf_desc.size = 256;
    buf_desc.descriptors = CGPU_RESOURCE_TYPE_UNIFORM_BUFFER;
    pulse_buffer_t buffer = pulse_graphic_buffer_create(app, &buf_desc, nullptr, 0);
    pulse_buffer_data_t* buf_data = pulse_graphic_buffer_acquire(app, &buffer);
    if (buf_data) {
        pulse_graphic_buffer_release(app, &buffer);
    }

    // ---- Sampler ----
    CGPUSamplerDescriptor smp_desc{};
    pulse_sampler_t sampler = pulse_graphic_sampler_create(app, &smp_desc);
    pulse_sampler_data_t* smp_data = pulse_graphic_sampler_acquire(app, &sampler);
    if (smp_data) {
        pulse_graphic_sampler_release(app, &sampler);
    }

    // ---- Texture ----
    CGPUTextureDescriptor tex_desc{};
    tex_desc.width = 64;
    tex_desc.height = 64;
    tex_desc.depth = 1;
    tex_desc.mip_levels = 1;
    tex_desc.format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    pulse_texture_t texture = pulse_graphic_texture_create_from_data(
        app, &tex_desc, nullptr, 0);
    pulse_texture_data_t* tex_data = pulse_graphic_texture_acquire(app, &texture);
    if (tex_data) {
        pulse_graphic_texture_release(app, &texture);
    }

    // ---- Mesh ----
    float verts[] = {0,0,0, 1,0,0, 0,1,0};
    uint16_t idxs[] = {0, 1, 2};
    CGPUVertexLayout vtx_layout{};
    CGPUVertexAttribute attr{};
    attr.semantic_name = "POSITION";
    attr.format = CGPU_VERTEX_FORMAT_FLOAT32X3;
    attr.offset = 0;
    attr.binding = 0;
    attr.array_size = 0;
    attr.elem_stride = 0;
    attr.rate = CGPU_VERTEX_INPUT_RATE_VERTEX;
    vtx_layout.p_attributes = &attr;
    vtx_layout.attribute_count = 1;

    pulse_mesh_t mesh = pulse_graphic_mesh_create_from_data(
        app, verts, 3, 12, idxs, 3, sizeof(uint16_t),
        CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP, &vtx_layout);
    pulse_mesh_data_t* mesh_data = pulse_graphic_mesh_acquire(app, &mesh);
    if (mesh_data) {
        pulse_graphic_mesh_release(app, &mesh);
    }

    // ---- Dynamic Mesh ----
    pulse_mesh_t dyn_mesh = pulse_graphic_mesh_create_dynamic(
        app, 1024, 12, 3072, sizeof(uint16_t),
        CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, &vtx_layout);
    pulse_graphic_mesh_update_vertices(app, &dyn_mesh, verts, 3);
    pulse_graphic_mesh_update_indices(app, &dyn_mesh, idxs, 3);
    pulse_mesh_data_t* dyn_data = pulse_graphic_mesh_acquire(app, &dyn_mesh);
    if (dyn_data) {
        pulse_graphic_mesh_release(app, &dyn_mesh);
    }

    // ---- Material ----
    pulse_material_t material = pulse_graphic_material_create(app, shader);
    pulse_graphic_material_bind_buffer(app, &material, 0, 0, buffer);
    pulse_graphic_material_bind_texture(app, &material, 0, 1, texture);
    pulse_graphic_material_bind_sampler(app, &material, 0, 2, sampler);
    pulse_material_data_t* mat_data = pulse_graphic_material_acquire(app, &material);
    if (mat_data) {
        pulse_graphic_material_release(app, &material);
    }

    // ---- Encoder API (compile-only: pulse_renderpass_encoder_t* is obtained during render) ----
    // These call directly to verify linkage; encoder is null so calls are safe no-ops.
    pulse_encoder_set_viewport(nullptr, 0, 0, 800, 600, 0, 1);
    pulse_encoder_set_scissor(nullptr, 0, 0, 800, 600);
    pulse_encoder_set_global_texture_handle(nullptr, pulse_texture_handle_t{}, 0, 0);
    pulse_encoder_set_global_buffer_handle(nullptr, pulse_buffer_handle_t{}, 0, 0);
    pulse_encoder_set_global_buffer_offset(nullptr, pulse_buffer_handle_t{}, 0, 0, 0, 256);
    pulse_encoder_push_constants(nullptr, shader, "test", nullptr);
    pulse_encoder_draw(nullptr, material, mesh);
    pulse_encoder_draw_submesh(nullptr, material, mesh, 3, 0, 3, 0);
    pulse_encoder_draw_procedure(nullptr, material, CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
    pulse_encoder_dispatch(nullptr, compute, 1, 1, 1);
    pulse_encoder_set_global_texture(nullptr, texture, 0, 0);
    pulse_encoder_set_global_buffer(nullptr, buffer, 0, 0);
    pulse_encoder_set_global_sampler(nullptr, sampler, 0, 0);

    // Run a single update to exercise plugin pipeline
    pulse_app_update(app);

    pulse_app_destroy(app);

    printf("Graphic module tests passed!\n");
    return 0;
}
