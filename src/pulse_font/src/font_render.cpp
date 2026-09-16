#include "font_internal.h"

#include "pulse_asset.h"
#include "pulse_graphics.h"
#include "pulse_window.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

uint8_t font_glyph_vert_spv[] = {
    #include "font_glyph.vs.spv.h"
};
uint8_t font_glyph_frag_spv[] = {
    #include "font_glyph.ps.spv.h"
};

namespace pulse_font_internal {

namespace {

constexpr uint32_t kMaxGroupsPerPass = 8;

struct gpu_glyph_instance {
    float rect[4];
    float uv[4];
    float color[4];
};

struct font_push_constants {
    float scale[2];
    float translate[2];
};

struct font_record_passdata {
    PulseShaderHandle shader;
    PulseMaterialHandle material;
    PulseMeshHandle mesh;
    PulseSamplerHandle sampler;
    PulseTextureHandle page_textures[kMaxGroupsPerPass];
    font_push_constants constants;
    font_pass_group groups[kMaxGroupsPerPass];
    uint32_t group_count;
    uint32_t scissor[4];
    uint32_t viewport_width;
    uint32_t viewport_height;
};

void draw_record_executable(PulseRenderPassEncoder* encoder, void* userdata) {
    auto* data = static_cast<font_record_passdata*>(userdata);
    if (!encoder || !data) {
        return;
    }
    pulse_render_pass_encoder_set_viewport(encoder, 0.0f, 0.0f, (float)data->viewport_width, (float)data->viewport_height, 0.0f, 1.0f);
    pulse_render_pass_encoder_set_scissor(encoder, data->scissor[0], data->scissor[1], data->scissor[2], data->scissor[3]);
    pulse_render_pass_encoder_push_constants(encoder, data->shader, "pc", &data->constants);
    pulse_render_pass_encoder_set_global_sampler(encoder, data->sampler, 0, 1);
    for (uint32_t i = 0; i < data->group_count; ++i) {
        pulse_render_pass_encoder_set_global_texture(encoder, data->page_textures[i], 0, 0);
        pulse_render_pass_encoder_draw_instanced(encoder, data->material, data->mesh, 6, data->groups[i].count, data->groups[i].first);
    }
}

void compute_scissor(const PulseScissor& scissor, const PulseTransform& transform, int32_t width, int32_t height, uint32_t out[4]) {
    const float x0 = scissor.x * transform.scale_x + transform.translate_x;
    const float x1 = (scissor.x + scissor.width) * transform.scale_x + transform.translate_x;
    const float y0 = scissor.y * transform.scale_y + transform.translate_y;
    const float y1 = (scissor.y + scissor.height) * transform.scale_y + transform.translate_y;
    const float left = std::clamp((std::min(x0, x1) + 1.0f) * 0.5f * (float)width, 0.0f, (float)width);
    const float right = std::clamp((std::max(x0, x1) + 1.0f) * 0.5f * (float)width, 0.0f, (float)width);
    const float top = std::clamp((1.0f - std::max(y0, y1)) * 0.5f * (float)height, 0.0f, (float)height);
    const float bottom = std::clamp((1.0f - std::min(y0, y1)) * 0.5f * (float)height, 0.0f, (float)height);
    out[0] = (uint32_t)left;
    out[1] = (uint32_t)top;
    out[2] = (uint32_t)std::max(0.0f, right - left);
    out[3] = (uint32_t)std::max(0.0f, bottom - top);
}

ecs_entity_t find_window(font_render_state& render) {
    if (!render.window_query) {
        return 0;
    }
    ecs_entity_t found = 0;
    ecs_iter_t it = ecs_query_iter(render.window_query->world, render.window_query);
    while (ecs_query_next(&it)) {
        if (it.count > 0) {
            found = it.entities[0];
            break;
        }
    }
    if (it.flags & EcsIterIsValid) {
        ecs_iter_fini(&it);
    }
    return found;
}

void append_gpu_instance(std::vector<uint8_t>& out, const PulseGlyphInstance& instance) {
    gpu_glyph_instance gpu{};
    gpu.rect[0] = instance.x;
    gpu.rect[1] = instance.y;
    gpu.rect[2] = instance.width;
    gpu.rect[3] = instance.height;
    gpu.uv[0] = instance.u0;
    gpu.uv[1] = instance.v0;
    gpu.uv[2] = instance.u1;
    gpu.uv[3] = instance.v1;
    gpu.color[0] = instance.r;
    gpu.color[1] = instance.g;
    gpu.color[2] = instance.b;
    gpu.color[3] = instance.a;
    const size_t offset = out.size();
    out.resize(offset + sizeof(gpu_glyph_instance));
    std::memcpy(out.data() + offset, &gpu, sizeof(gpu_glyph_instance));
}

void font_record_callback(PulseAppId app, PulseRenderGraphId graph, void* user_data) {
    auto* state = static_cast<pulse_font_plugin_state*>(user_data);
    if (!state || !graph) {
        return;
    }
    font_render_state& render = state->render;
    render.gpu_instances.clear();
    render.groups.clear();
    render.record_groups.clear();
    if (render.records.empty() || render.instances.empty()) {
        render.records.clear();
        render.instances.clear();
        return;
    }
    ecs_entity_t window_entity = find_window(render);
    ecs_world_t* world = pulse_app_world(app);
    const PulseWindow* window = (world && window_entity) ? ecs_get(world, window_entity, PulseWindow) : nullptr;
    if (!window || window->width <= 0 || window->height <= 0) {
        render.records.clear();
        render.instances.clear();
        return;
    }
    PulseRGTextureHandle backbuffer = pulse_import_window_backbuffer(app, graph, window_entity);
    if (!pulse_rgtexture_handle_is_valid(backbuffer)) {
        render.records.clear();
        render.instances.clear();
        return;
    }
    for (uint32_t i = 0; i < render.pages.size() && i < state->pages.size(); ++i) {
        font_page_gpu& gpu = render.pages[i];
        atlas_page& page = state->pages[i];
        if (!gpu.requested) {
            continue;
        }
        if (!gpu.ready) {
            if (!pulse_texture_is_alive(app, gpu.request)) {
                gpu.requested = false;
                continue;
            }
            if (!pulse_texture_is_ready(app, gpu.request)) {
                continue;
            }
            gpu.handle = pulse_texture_get_handle(app, gpu.request);
            gpu.ready = true;
            page.dirty = true;
        }
        if (!page.dirty) {
            continue;
        }
        PulseRGTextureHandle texture = pulse_render_graph_import_texture(graph, gpu.handle);
        if (!pulse_rgtexture_handle_is_valid(texture)) {
            continue;
        }
        page.dirty = false;
        pulse_render_graph_add_uploadtexturepass_ex(graph, "PulseFontAtlasUpload", texture, 0, 0, page.pixels.size(), 0, page.pixels.data(), nullptr, 0, nullptr);
    }
    for (const font_draw_record& record : render.records) {
        const uint32_t group_first = (uint32_t)render.groups.size();
        for (uint32_t page = 0; page < render.pages.size() && page < state->pages.size(); ++page) {
            if (!render.pages[page].ready) {
                continue;
            }
            const uint32_t first = (uint32_t)(render.gpu_instances.size() / sizeof(gpu_glyph_instance));
            for (uint32_t i = record.first; i < record.first + record.count; ++i) {
                if (render.instances[i].page == page) {
                    append_gpu_instance(render.gpu_instances, render.instances[i]);
                }
            }
            const uint32_t count = (uint32_t)(render.gpu_instances.size() / sizeof(gpu_glyph_instance)) - first;
            if (count == 0) {
                continue;
            }
            render.groups.push_back(font_pass_group{ page, first, count });
            if (render.groups.size() - group_first >= kMaxGroupsPerPass) {
                break;
            }
        }
        render.record_groups.push_back(font_record_groups{ group_first, (uint32_t)render.groups.size() - group_first });
    }
    if (render.gpu_instances.empty()) {
        render.records.clear();
        render.instances.clear();
        return;
    }
    const uint32_t instance_count = (uint32_t)(render.gpu_instances.size() / sizeof(gpu_glyph_instance));
    PulseRGBufferHandle instances = pulse_render_graph_import_dynamic_mesh_vertex_buffer(graph, render.mesh, instance_count);
    if (!pulse_rgbuffer_handle_is_valid(instances)) {
        render.records.clear();
        render.instances.clear();
        return;
    }
    pulse_render_graph_add_uploadbufferpass_ex(graph, "PulseFontInstances", instances, render.gpu_instances.size(), 0, render.gpu_instances.data(), nullptr, 0, nullptr);
    for (uint32_t r = 0; r < render.record_groups.size() && r < render.records.size(); ++r) {
        const font_record_groups& groups = render.record_groups[r];
        if (groups.count == 0) {
            continue;
        }
        const font_draw_record& record = render.records[r];
        PulseRenderPassBuilder pass = pulse_render_graph_add_render_pass(graph, "PulseFont");
        pulse_render_pass_builder_add_color_attachment(&pass, backbuffer, CGPU_LOAD_ACTION_LOAD, 0, CGPU_STORE_ACTION_STORE);
        pulse_render_pass_builder_use_buffer(&pass, instances);
        font_record_passdata* passdata = nullptr;
        pulse_render_pass_builder_set_executable(&pass, draw_record_executable, sizeof(font_record_passdata), (void**)&passdata);
        if (!passdata) {
            continue;
        }
        passdata->shader = render.shader;
        passdata->material = render.material;
        passdata->mesh = render.mesh;
        passdata->sampler = render.sampler;
        passdata->group_count = 0;
        passdata->viewport_width = (uint32_t)window->width;
        passdata->viewport_height = (uint32_t)window->height;
        passdata->constants.scale[0] = record.transform.scale_x;
        passdata->constants.scale[1] = record.transform.scale_y;
        passdata->constants.translate[0] = record.transform.translate_x;
        passdata->constants.translate[1] = record.transform.translate_y;
        if (record.scissor.enabled) {
            compute_scissor(record.scissor, record.transform, window->width, window->height, passdata->scissor);
        } else {
            passdata->scissor[0] = 0;
            passdata->scissor[1] = 0;
            passdata->scissor[2] = (uint32_t)window->width;
            passdata->scissor[3] = (uint32_t)window->height;
        }
        for (uint32_t g = 0; g < groups.count; ++g) {
            const font_pass_group& group = render.groups[groups.first + g];
            PulseRGTextureHandle texture = pulse_render_graph_import_texture(graph, render.pages[group.page].handle);
            if (!pulse_rgtexture_handle_is_valid(texture)) {
                continue;
            }
            pulse_render_pass_builder_sample(&pass, texture);
            passdata->page_textures[passdata->group_count] = render.pages[group.page].handle;
            passdata->groups[passdata->group_count] = group;
            ++passdata->group_count;
        }
    }
    render.records.clear();
    render.instances.clear();
}

}

EPulseResult render_init(pulse_font_plugin_state* state) {
    font_render_state& render = state->render;
    if (render.initialized) {
        return PULSE_RESULT_OK;
    }
    PulseAppId app = state->app;
    const PulseRenderer* renderer = pulse_get_renderer(app);
    if (!renderer) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(PulsePrimaryWindowId) == 0) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    CGPUBlendAttachmentState blend_attachment = {
        .enable = true,
        .src_factor = CGPU_BLEND_FACTOR_SRC_ALPHA,
        .dst_factor = CGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .src_alpha_factor = CGPU_BLEND_FACTOR_SRC_ALPHA,
        .dst_alpha_factor = CGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        .blend_op = CGPU_BLEND_OP_ADD,
        .blend_alpha_op = CGPU_BLEND_OP_ADD,
        .color_mask = CGPU_COLOR_MASK_RGBA,
    };
    CGPUBlendStateDescriptor blend_desc = {
        .attachment_count = 1,
        .p_attachments = &blend_attachment,
        .alpha_to_coverage = false,
        .independent_blend = false,
    };
    CGPUDepthStateDescriptor depth_desc = {
        .depth_test = false,
        .depth_write = false,
        .stencil_test = false,
    };
    CGPURasterizerStateDescriptor rasterizer_state = {
        .cull_mode = CGPU_CULL_MODE_NONE,
    };

    PulseShaderProperty global_property = {
        .name = "font_global",
        .type = PULSE_SHADER_PROPERTY_TYPE_TEXTURE,
        .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL,
        .set = 0,
        .binding = 0,
        .offset = 0,
        .size = 0,
    };
    PulseShaderCreateFromBinaryDesc shader_desc = {};
    shader_desc.p_vs_data = font_glyph_vert_spv;
    shader_desc.vs_data_size = sizeof(font_glyph_vert_spv);
    shader_desc.p_fs_data = font_glyph_frag_spv;
    shader_desc.fs_data_size = sizeof(font_glyph_frag_spv);
    shader_desc.blend_desc = blend_desc;
    shader_desc.depth_desc = depth_desc;
    shader_desc.rasterizer_state = rasterizer_state;
    shader_desc.p_properties = &global_property;
    shader_desc.properties_count = 1;

    render.shader = pulse_create_shader_from_binary(app, &shader_desc);
    if (!pulse_asset_handle_is_valid(pulse_shader_to_handle(render.shader))) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    PulseMaterialCreateDesc material_desc = {};
    material_desc.shader = render.shader;
    render.material = pulse_create_material(app, &material_desc);
    if (!pulse_asset_handle_is_valid(pulse_material_to_handle(render.material))) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    CGPUVertexAttribute attributes[3] = {
        { "RECT", 0, CGPU_VERTEX_FORMAT_FLOAT32X4, 0, 0, sizeof(float) * 4, CGPU_VERTEX_INPUT_RATE_INSTANCE },
        { "TEXCOORD", 0, CGPU_VERTEX_FORMAT_FLOAT32X4, 0, sizeof(float) * 4, sizeof(float) * 4, CGPU_VERTEX_INPUT_RATE_INSTANCE },
        { "COLOR", 0, CGPU_VERTEX_FORMAT_FLOAT32X4, 0, sizeof(float) * 8, sizeof(float) * 4, CGPU_VERTEX_INPUT_RATE_INSTANCE },
    };
    CGPUVertexLayout vertex_layout = {
        .attribute_count = 3,
        .p_attributes = attributes,
    };
    PulseMeshCreateDynamicDesc mesh_desc = {};
    mesh_desc.topology = CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    mesh_desc.index_stride = 0;
    mesh_desc.layout = vertex_layout;
    render.mesh = pulse_create_mesh_dynamic(app, &mesh_desc);
    if (!pulse_asset_handle_is_valid(pulse_mesh_to_handle(render.mesh))) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    PulseSamplerCreateDesc sampler_desc = {};
    sampler_desc.desc = {
        .min_filter = CGPU_FILTER_TYPE_LINEAR,
        .mag_filter = CGPU_FILTER_TYPE_LINEAR,
        .mipmap_mode = CGPU_MIP_MAP_MODE_LINEAR,
        .address_u = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_v = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_w = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
    };
    render.sampler = pulse_create_sampler(app, &sampler_desc);
    if (!pulse_asset_handle_is_valid(pulse_sampler_to_handle(render.sampler))) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    ecs_query_desc_t query_desc{};
    query_desc.terms[0] = { .id = ecs_id(PulsePrimaryWindowId) };
    query_desc.cache_kind = EcsQueryCacheAuto;
    render.window_query = ecs_query_init(world, &query_desc);
    if (!render.window_query) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    PulseRenderRecordCallbackDesc callback_desc = {
        .callback = font_record_callback,
        .user_data = state,
        .priority = state->desc.record_priority,
    };
    EPulseResult result = pulse_add_render_record_callback(app, &callback_desc);
    if (result != PULSE_RESULT_OK) {
        return result;
    }
    render.initialized = true;
    return PULSE_RESULT_OK;
}

void render_shutdown(pulse_font_plugin_state* state) {
    font_render_state& render = state->render;
    if (render.window_query) {
        ecs_query_fini(render.window_query);
        render.window_query = nullptr;
    }
    if (!render.initialized) {
        return;
    }
    PulseAppId app = state->app;
    if (app && pulse_get_renderer(app)) {
        pulse_remove_render_record_callback(app, font_record_callback);
        PulseAssetSystemId asset_system = pulse_get_asset_system(app);
        for (font_page_gpu& gpu : render.pages) {
            if (gpu.ready) {
                pulse_asset_system_release(asset_system, pulse_texture_to_handle(gpu.handle), nullptr);
                gpu.handle = {};
                gpu.ready = false;
            }
        }
        pulse_asset_system_release(asset_system, pulse_mesh_to_handle(render.mesh), nullptr);
        render.mesh = {};
        pulse_asset_system_release(asset_system, pulse_material_to_handle(render.material), nullptr);
        render.material = {};
        pulse_asset_system_release(asset_system, pulse_shader_to_handle(render.shader), nullptr);
        render.shader = {};
        pulse_asset_system_release(asset_system, pulse_sampler_to_handle(render.sampler), nullptr);
        render.sampler = {};
    }
    render.pages.clear();
    render.instances.clear();
    render.records.clear();
    render.groups.clear();
    render.record_groups.clear();
    render.gpu_instances.clear();
    render.initialized = false;
}

void render_ensure_page(pulse_font_plugin_state* state, uint32_t page) {
    if (!state->app || !pulse_get_renderer(state->app)) {
        return;
    }
    if (page >= state->render.pages.size()) {
        return;
    }
    font_page_gpu& gpu = state->render.pages[page];
    if (gpu.requested) {
        return;
    }
    const PulseRenderer* renderer = pulse_get_renderer(state->app);
    PulseTextureCreateDesc texture_desc = {};
    texture_desc.desc.name = "PulseFontAtlas";
    texture_desc.desc.width = state->desc.atlas_width;
    texture_desc.desc.height = state->desc.atlas_height;
    texture_desc.desc.depth = 1;
    texture_desc.desc.array_size = 1;
    texture_desc.desc.format = CGPU_TEXTURE_FORMAT_R8_UNORM;
    texture_desc.desc.mip_levels = 1;
    texture_desc.desc.owner_queue = renderer->graphics_queue;
    texture_desc.desc.start_state = CGPU_RESOURCE_STATE_COPY_DEST;
    texture_desc.desc.descriptors = CGPU_RESOURCE_TYPE_TEXTURE;
    texture_desc.p_pixel_data = nullptr;
    texture_desc.pixel_data_size = 0;
    texture_desc.generate_mipmaps = false;
    gpu.request = pulse_create_texture(state->app, &texture_desc);
    if (!pulse_texture_is_alive(state->app, gpu.request)) {
        std::fprintf(stderr, "pulse_font: failed to create atlas texture\n");
        gpu.request = {};
        return;
    }
    gpu.requested = true;
}

}

using namespace pulse_font_internal;

extern "C" {

EPulseResult pulse_font_submit(PulseAppId app, const PulseDrawDesc* desc) {
    pulse_font_plugin_state* state = state_from_app(app);
    if (!state || !desc) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (desc->instances_count == 0) {
        return PULSE_RESULT_OK;
    }
    if (!desc->p_instances) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    font_render_state& render = state->render;
    if (!render.initialized) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }
    if (render.records.size() >= kMaxPendingDraws) {
        if (!render.overflow_reported) {
            std::fprintf(stderr, "pulse_font: pending draw queue is full, dropping submits\n");
            render.overflow_reported = true;
        }
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }
    font_draw_record record{};
    record.transform = desc->transform;
    record.scissor = desc->scissor;
    record.first = (uint32_t)render.instances.size();
    record.count = (uint32_t)desc->instances_count;
    render.instances.resize(render.instances.size() + desc->instances_count);
    std::memcpy(render.instances.data() + record.first, desc->p_instances, desc->instances_count * sizeof(PulseGlyphInstance));
    render.records.push_back(record);
    return PULSE_RESULT_OK;
}

}
