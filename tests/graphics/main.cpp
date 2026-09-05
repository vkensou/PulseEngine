#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <vector>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"
#include "pulse_input.h"
#include "pulse_window.h"
#include "pulse_graphics.h"

static uint8_t dummy_spv[16] = {0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x00, 0x00,
                                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
struct Mat4
{
    float Elements[4][4];
};

static inline Mat4 Mat4_Orthographic(float size, float aspect, float Near, float Far)
{
    Mat4 Result = { 0 };

    Result.Elements[0][0] = 1.0f / (size * aspect);
    Result.Elements[1][1] = 1.0f / (size);
    Result.Elements[2][2] = 1 / (Near - Far);
    Result.Elements[3][3] = 1.0f;

    Result.Elements[3][2] = -(Far) / (Near - Far);

    return Result;
}

static inline Mat4 Mat4_Translate(float X, float Y, float Z)
{
    Mat4 Result{};

    Result.Elements[0][0] = 1;
    Result.Elements[1][1] = 1;
    Result.Elements[2][2] = 1;
    Result.Elements[3][3] = 1;

    Result.Elements[3][0] = X;
    Result.Elements[3][1] = Y;
    Result.Elements[3][2] = Z;

    return Result;
}

struct Vec4
{
	float X, Y, Z, W;
};

struct PassData
{
    Mat4	vpMatrix;
};

struct MaterialData
{
    Vec4	albedo;
};

struct ObjectData
{
    Mat4	wMatrix;
};

struct test_graphic_resources {
    PulseShaderRequest shader;
    PulseComputeShaderRequest compute;
    PulseGraphicsBufferRequest buffer;
    PulseSamplerRequest sampler;
    PulseTextureRequest texture;
    PulseMeshRequest mesh;
    PulseMaterialHandle material;
};

// passdata 传入 executable callback
struct test_render_passdata {
    PulseMaterialHandle material;
    PulseMeshHandle mesh;
    PulseShaderHandle shader;
    PulseComputeShaderHandle compute;
    PulseTextureHandle texture;
    PulseGraphicsBufferHandle buffer;
};

static void on_test_render(PulseRenderPassEncoder* encoder, void* userdata) {
    auto* data = static_cast<test_render_passdata*>(userdata);
    if (!encoder) return;

    pulse_render_pass_encoder_set_viewport(encoder, 0, 0, 800, 600, 0, 1);
    pulse_render_pass_encoder_set_scissor(encoder, 0, 0, 800, 600);
    pulse_render_pass_encoder_set_global_texture_handle(encoder, PulseRGTextureHandle{}, 0, 0);
    pulse_render_pass_encoder_set_global_buffer_handle(encoder, PulseRGBufferHandle{}, 0, 0);
    pulse_render_pass_encoder_set_global_buffer_offset(encoder, PulseRGBufferHandle{}, 0, 0, 0, 256);
    pulse_render_pass_encoder_push_constants(encoder, data->shader, "test", nullptr);
    pulse_render_pass_encoder_draw(encoder, data->material, data->mesh);
    pulse_render_pass_encoder_draw_submesh(encoder, data->material, data->mesh, 3, 0, 3, 0);
    pulse_render_pass_encoder_draw_procedure(encoder, data->material, CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 3);
    pulse_render_pass_encoder_dispatch(encoder, data->compute, 1, 1, 1);
    pulse_render_pass_encoder_set_global_texture(encoder, data->texture, 0, 0);
    pulse_render_pass_encoder_set_global_buffer(encoder, data->buffer, 0, 0);
    pulse_render_pass_encoder_set_global_sampler(encoder, {}, 0, 0);
}

struct test_render_state {
    ecs_query_t* window_query;
    PulseTextureRequest texture_request;
    PulseMeshRequest mesh_request;
    PulseMaterialHandle material;
    PulseTextureHandle texture;
    PulseMeshHandle mesh;
    bool material_resolved = false;
    bool mesh_resolved = false;
    PassData passData;
    ObjectData objectData;
};

enum class ktx_probe_mode {
    MustLoad,
    TracksFormatSupport,
};

struct ktx_probe {
    const char* path;
    ktx_probe_mode mode;
    ECGPUTextureFormat format;
    PulseTextureRequest request{};
    bool resolved = false;
    bool loaded = false;
    bool device_supported = true;
    int frames = 0;
};

static ktx_probe kKtxProbes[] = {
    { "Tiles130_rgba_srgb.ktx2", ktx_probe_mode::MustLoad, CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB },
    { "Tiles130_runtime_mip.ktx2", ktx_probe_mode::MustLoad, CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB },
    { "Tiles130_uastc.ktx2", ktx_probe_mode::MustLoad, CGPU_TEXTURE_FORMAT_UNDEFINED },
    { "Tiles130_uastc_zstd.ktx2", ktx_probe_mode::MustLoad, CGPU_TEXTURE_FORMAT_UNDEFINED },
    { "Tiles130_etc1s.ktx2", ktx_probe_mode::MustLoad, CGPU_TEXTURE_FORMAT_UNDEFINED },
    { "Tiles130_bc7.ktx2", ktx_probe_mode::TracksFormatSupport, CGPU_TEXTURE_FORMAT_BC7_SRGB_BLOCK },
    { "Tiles130_astc6x6.ktx2", ktx_probe_mode::TracksFormatSupport, CGPU_TEXTURE_FORMAT_ASTC_6X6_SRGB_BLOCK },
    { "Tiles130_rgb_srgb.ktx2", ktx_probe_mode::TracksFormatSupport, CGPU_TEXTURE_FORMAT_R8G8B8_SRGB },
    { "TilesGray512.ktx", ktx_probe_mode::TracksFormatSupport, CGPU_TEXTURE_FORMAT_R8G8B8_UNORM },
};

static const int kKtxProbeFrameBudget = 300;

static void request_ktx_probes(PulseAppId app, std::vector<ktx_probe>& probes) {
    const PulseRenderer* renderer = pulse_get_renderer(app);
    const CGPUAdapterDetail* detail = renderer ? cgpu_adapter_query_adapter_detail(renderer->adapter) : nullptr;
    assert(detail && "adapter detail must be available while the renderer is alive");
    assert((detail->format_supports[CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB] & CGPU_TEXTURE_FORMAT_SUPPORT_SAMPLE) != 0 && "mandatory R8G8B8A8_SRGB must be sampleable");
    for (ktx_probe& probe : kKtxProbes) {
        PulseTextureLoadDesc desc{
            .filepath = probe.path,
            .generate_mipmaps = true,
        };
        probe.resolved = false;
        probe.loaded = false;
        probe.frames = 0;
        probe.device_supported = probe.mode == ktx_probe_mode::MustLoad
            || (detail->format_supports[probe.format] & CGPU_TEXTURE_FORMAT_SUPPORT_SAMPLE) != 0;
        probe.request = pulse_load_texture(app, &desc);
        probes.push_back(probe);
    }
}

static void update_ktx_probes(PulseAppId app, std::vector<ktx_probe>& probes) {
    const PulseAssetSystemId system = pulse_get_graphics_asset_system(app);
    for (ktx_probe& probe : probes) {
        if (probe.resolved) {
            continue;
        }
        const PulseAssetRequest asset = pulse_texture_request_to_asset_request(probe.request);
        const EPulseAssetState state = pulse_asset_system_get_state(system, asset);
        if (state == PULSE_ASSET_STATE_FAILED) {
            probe.resolved = true;
            probe.loaded = false;
            const char* error = pulse_asset_system_get_error(system, asset);
            printf("ktx2 probe %s FAILED: %s\n", probe.path, error ? error : "no error message");
            continue;
        }
        if (pulse_texture_is_ready(app, probe.request)) {
            probe.resolved = true;
            probe.loaded = true;
            continue;
        }
        if (++probe.frames >= kKtxProbeFrameBudget) {
            probe.resolved = true;
            probe.loaded = false;
            printf("ktx2 probe %s stuck in state %d\n", probe.path, (int)state);
        }
    }
}

static void report_ktx_probes(std::vector<ktx_probe>& probes) {
    int failed = 0;

    for (ktx_probe& probe : probes) {
        assert(probe.resolved && "ktx2 probe did not resolve, run the window longer");
    }

    for (ktx_probe& probe : probes) {
        printf("ktx2 probe %-28s loaded=%d supported=%d\n", probe.path, (int)probe.loaded, (int)probe.device_supported);
        if (probe.mode == ktx_probe_mode::MustLoad) {
            assert(probe.loaded && "ktx2 texture must load on every device");
            if (!probe.loaded) {
                ++failed;
            }
        } else {
            assert(probe.loaded == probe.device_supported && "native format load outcome must match device support");
        }
    }
    printf("ktx2 probes: %zu checked, %d failed\n", probes.size(), failed);
}

enum class test_graphics_load_phase {
    Start,
    WaitShader,  // shader still loading; create the material once it's ready
    WaitAssets,  // record callback resolves texture + mesh handles
    Done,
};

struct test_graphics_load_machine {
    test_graphics_load_phase phase = test_graphics_load_phase::Start;
    PulseAppId app = nullptr;
    PulseShaderRequest shader{};
    test_render_state* render = nullptr;
    std::vector<ktx_probe> probes;
};

static void test_graphics_load_system(ecs_iter_t* it) {
    (void)it;
    test_graphics_load_machine& m = *(test_graphics_load_machine*)it->ctx;
    if (!m.app || !m.render) {
        return;
    }

    if (m.phase != test_graphics_load_phase::Start) {
        update_ktx_probes(m.app, m.probes);
    }

    switch (m.phase) {
        case test_graphics_load_phase::Start: {
            PulseShaderRequest shader = pulse_load_shader(m.app, "color.shader");
            request_ktx_probes(m.app, m.probes);

            PulseTextureLoadDesc tex_load_desc{
                .filepath = "TilesGray512.jpg",
                .generate_mipmaps = true,
            };
            PulseTextureRequest texture = pulse_load_texture(
                m.app, &tex_load_desc);

            std::vector<uint32_t> pixels = { 0xFF00FFFF };

            PulseTextureCreateDesc tex_create_desc
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
                .p_pixel_data = pixels.data(),
                .pixel_data_size = sizeof(uint32_t),
            };

            PulseTextureRequest texture2 = pulse_create_texture(
                m.app, &tex_create_desc);

            PulseMeshRequest mesh = pulse_load_mesh(
                m.app, "Quad.obj");

            m.shader = shader;
            m.render->texture_request = texture;
            m.render->mesh_request = mesh;
            m.phase = test_graphics_load_phase::WaitShader;
        }
            break;

        case test_graphics_load_phase::WaitShader:
            if (pulse_shader_is_ready(m.app, m.shader)) {
                PulseShaderHandle shader_handle = pulse_shader_get_handle(m.app, m.shader);
                assert(shader_handle.index != 0);

                PulseMaterialCreateDesc mat_desc = {
                    .shader = shader_handle,
                };
                m.render->material = pulse_create_material(m.app, &mat_desc);
                assert(m.render->material.index != 0);

                m.phase = test_graphics_load_phase::WaitAssets;
            }
            break;

        case test_graphics_load_phase::WaitAssets:
            // texture/mesh are resolved asynchronously inside record_test_graphic
            if (m.render->material_resolved && m.render->mesh_resolved) {
                m.phase = test_graphics_load_phase::Done;
            }
            break;

    }
}

static void record_test_graphic(
    PulseAppId app,
    PulseRenderGraphId graph,
    void* user_data
) {
    test_render_state* state = static_cast<test_render_state*>(user_data);
    if (!graph || !state) {
        return;
    }

    if (!state->window_query) {
        return;
    }

    if (!state->material_resolved && pulse_texture_is_ready(app, state->texture_request)) {
        state->texture = pulse_texture_get_handle(app, state->texture_request);
        pulse_material_set_property_float4(app, state->material, "albedo", HMM_V4(1.0f, 0.0f, 0.0f, 1.0f));
        state->material_resolved = true;
    }

    if (!state->mesh_resolved && pulse_mesh_is_ready(app, state->mesh_request)) {
        state->mesh = pulse_mesh_get_handle(app, state->mesh_request);
        state->mesh_resolved = true;
    }

    ecs_iter_t it = ecs_query_iter(state->window_query->world, state->window_query);
    while (ecs_query_next(&it)) {
        PulseWindow* windows = ecs_field(&it, PulseWindow, 0);
        for (int i = 0; i < it.count; ++i) {
            ecs_entity_t entity = it.entities[i];
            const auto& window = windows[i];

            PulseRGTextureHandle target_handle =
                pulse_import_window_backbuffer(app, graph, entity);
            if (!pulse_rgtexture_handle_is_valid(target_handle)) {
                continue;
            }

            int width = window.width;
            int height = window.height;
            float aspect = (float)width / height;
            float near = -1;
            float far = 1;
            auto proj = Mat4_Orthographic(5, aspect, near, far);
            state->passData = { proj };

            auto objectMat = Mat4_Translate(0, 0, 0);
            state->objectData = { objectMat };

            auto pass_ubo_handle = pulse_render_graph_declare_uniform_buffer_quick(graph, sizeof(PassData), &state->passData);
            auto object_ubo_handle = pulse_render_graph_declare_uniform_buffer_quick(graph, sizeof(ObjectData), &state->objectData);

            PulseRenderPassBuilder pass =
                pulse_render_graph_add_render_pass(graph, "TestCallbackPass");
            pulse_render_pass_builder_add_color_attachment(
                &pass,
                target_handle,
                CGPU_LOAD_ACTION_CLEAR,
                0xff00ffff,
                CGPU_STORE_ACTION_STORE
            );

            if (!state->material_resolved || !state->mesh_resolved) {
                continue;
            }

            pulse_render_pass_builder_use_buffer(&pass, pass_ubo_handle);
            pulse_render_pass_builder_use_buffer(&pass, object_ubo_handle);

            struct MainPassPassData
            {
                PulseMaterialHandle material;
                PulseMeshHandle mesh;
                PulseRGBufferHandle pass_ubo_handle;
                PulseRGBufferHandle object_ubo_handle;
            };
            MainPassPassData* passdata;
            pulse_render_pass_builder_set_executable(&pass, [](PulseRenderPassEncoder* encoder, void* passdata)
                {
                    MainPassPassData* resolved_passdata = (MainPassPassData*)passdata;
                    pulse_render_pass_encoder_set_global_buffer_handle(encoder, resolved_passdata->pass_ubo_handle, 0, 0);
                    pulse_render_pass_encoder_set_global_buffer_offset(encoder, resolved_passdata->object_ubo_handle, 2, 0, 0, sizeof(ObjectData));
                    pulse_render_pass_encoder_draw(encoder, resolved_passdata->material, resolved_passdata->mesh);
                }, sizeof(MainPassPassData), (void**)&passdata);
            passdata->material = state->material;
            passdata->mesh = state->mesh;
            passdata->pass_ubo_handle = pass_ubo_handle;
            passdata->object_ubo_handle = object_ubo_handle;
        }
    }
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-graphics",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // Add required plugins
    auto window_desc = pulse_window_plugin_desc_default();
    assert(pulse_add_window_plugin(app, &window_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_vfs_mount("tests/graphics/data", "/", false));
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // Add pulse_graphic plugin
    const char *per_draw_shader_properties[] = {
        "wMatrix",
    };

    auto graphic_desc = pulse_graphics_plugin_desc_default();
    graphic_desc.enable_debug_layer = true;
    graphic_desc.enable_gpu_based_validation = true;
    graphic_desc.p_per_draw_shader_properties = per_draw_shader_properties;
    graphic_desc.per_draw_shader_properties_count = sizeof(per_draw_shader_properties) / sizeof(const char*);
    assert(pulse_add_graphics_plugin(app, &graphic_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_has_plugin(app, "pulse_graphics"));

    // ---- Register record callback with graphic resources ----
    test_render_state render_state{};

    ecs_query_desc_t window_query_desc{};
    window_query_desc.terms[0] = { .id = ecs_id(PulseWindow) };
    window_query_desc.cache_kind = EcsQueryCacheAuto;
    render_state.window_query = ecs_query_init(pulse_app_world(app), &window_query_desc);

    PulseRenderRecordCallbackDesc cb_desc{};
    cb_desc.callback = record_test_graphic;
    cb_desc.user_data = &render_state;
    cb_desc.priority = 0;
    pulse_add_render_record_callback(app, &cb_desc);


    test_graphics_load_machine test_graphics_load;
    test_graphics_load.app = app;
    test_graphics_load.render = &render_state;

    ecs_world_t* world = pulse_app_world(app);

    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "test_graphics_load_machine";

    ecs_system_desc_t system_desc{};
    system_desc.entity = ecs_entity_init(world, &entity_desc);
    system_desc.phase = EcsOnUpdate;      // run system in the OnUpdate phase
    system_desc.ctx = &test_graphics_load;
    system_desc.run = test_graphics_load_system;
    ecs_entity_t load_system = ecs_system_init(world, &system_desc);
    assert(load_system != 0);

    pulse_app_run(app);

    assert(render_state.material_resolved);
    assert(render_state.mesh_resolved);

    report_ktx_probes(test_graphics_load.probes);

    ecs_query_fini(render_state.window_query);

    pulse_destroy_app(app);

    printf("Graphic module tests passed!\n");
    return 0;
}
