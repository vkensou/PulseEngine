#include "imgui_internal.h"

#include <cstring>

// hlsl2spv bin2c 内嵌的 imgui shader 字节（与 blit shader 相同构建方式）。
uint8_t imgui_vert_spv[] = {
    #include "imgui.vs.spv.h"
};
uint8_t imgui_frag_spv[] = {
    #include "imgui.ps.spv.h"
};

namespace pulse_imgui_internal {

namespace {

constexpr int32_t kImGuiRecordPriority = 10000; // 在游戏 pass 之后叠加

void imgui_pass_executable(PulseRenderPassEncoder* encoder, void* userdata) {
    pulse_imgui_plugin_state* state =
        *static_cast<pulse_imgui_plugin_state**>(userdata);
    if (!state || !state->font_ready) {
        return;
    }
    ImDrawData* draw_data = state->draw_data;
    if (!draw_data) {
        return;
    }

    int fb_width = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
    if (fb_width <= 0 || fb_height <= 0) {
        return;
    }

    pulse_render_pass_encoder_set_viewport(
        encoder, 0.0f, 0.0f, (float)fb_width, (float)fb_height, 0.f, 1.f);

    // 投影：imgui 正交坐标 → NDC（Vulkan 翻转 Y）。
    float scale[2] = {
        2.0f / draw_data->DisplaySize.x,
        -2.0f / draw_data->DisplaySize.y,
    };
    float translate[2] = {
        -1.0f - draw_data->DisplayPos.x * scale[0],
        +1.0f - draw_data->DisplayPos.y * scale[1],
    };
    struct ConstantData {
        float scale[2];
        float translate[2];
    } data = {
        .scale = { scale[0], scale[1] },
        .translate = { translate[0], translate[1] },
    };
    pulse_render_pass_encoder_push_constants(encoder, state->shader, "pc", &data);

    // 全局资源绑定由 encoder 内部管理（renderer_managed set）。
    pulse_render_pass_encoder_set_global_texture(
        encoder, state->font_texture, 0, 0);
    pulse_render_pass_encoder_set_global_sampler(
        encoder, state->font_sampler, 0, 1);

    ImVec2 clip_off = draw_data->DisplayPos;         // (0,0) unless using multi-viewports
    ImVec2 clip_scale = draw_data->FramebufferScale; // (1,1) unless using retina display

    int global_vtx_offset = 0;
    int global_idx_offset = 0;
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback != nullptr) {
                pcmd->UserCallback(cmd_list, pcmd);
                continue;
            }

            // 裁剪矩形投影到 framebuffer 空间并 clamp 到视口。
            ImVec2 clip_min(
                (pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
            ImVec2 clip_max(
                (pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
            if (clip_min.x < 0.0f) { clip_min.x = 0.0f; }
            if (clip_min.y < 0.0f) { clip_min.y = 0.0f; }
            if (clip_max.x > (float)fb_width) { clip_max.x = (float)fb_width; }
            if (clip_max.y > (float)fb_height) { clip_max.y = (float)fb_height; }
            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) {
                continue;
            }

            pulse_render_pass_encoder_set_scissor(
                encoder,
                (uint32_t)clip_min.x,
                (uint32_t)clip_min.y,
                (uint32_t)(clip_max.x - clip_min.x),
                (uint32_t)(clip_max.y - clip_min.y));
            pulse_render_pass_encoder_draw_submesh(
                encoder,
                state->material,
                state->mesh,
                pcmd->ElemCount,
                pcmd->IdxOffset + global_idx_offset,
                0,
                pcmd->VtxOffset + global_vtx_offset);
        }
        global_idx_offset += cmd_list->IdxBuffer.Size;
        global_vtx_offset += cmd_list->VtxBuffer.Size;
    }

    // 避免把最后一条 ImGui 裁剪矩形泄漏给后续 pass。
    pulse_render_pass_encoder_set_scissor(
        encoder, 0, 0, (uint32_t)fb_width, (uint32_t)fb_height);
}

std::tuple<PulseRGBufferHandle, PulseRGBufferHandle> setup_mesh_data(pulse_imgui_plugin_state* state, ImDrawData* drawData, PulseRenderGraphId rg) {
	struct PassData
	{
		ImDrawData* drawData;
	};
	PassData* update_vertex_passdata;
	auto imgui_vertex_buffer = pulse_render_graph_import_dynamic_mesh_vertex_buffer(rg, state->mesh, drawData->TotalVtxCount);
	pulse_render_graph_add_uploadbufferpass(rg, "upload imgui vertex data", imgui_vertex_buffer, [](PulseUploadPassEncoder* encoder, void* passdata)
		{
			PassData* resolved_passdata = (PassData*)passdata;
			ImDrawData* drawData = resolved_passdata->drawData;
			uint32_t offset = 0;
			for (int n = 0; n < drawData->CmdListsCount; n++)
			{
				const ImDrawList* cmd_list = drawData->CmdLists[n];
				pulse_upload_pass_encoder_upload(encoder, offset, cmd_list->VtxBuffer.Size * sizeof(ImDrawVert), cmd_list->VtxBuffer.Data);

				offset += cmd_list->VtxBuffer.Size * sizeof(ImDrawVert);
			}
		}, sizeof(PassData), (void**)&update_vertex_passdata);
	update_vertex_passdata->drawData = drawData;

	PassData* update_index_passdata;
	auto imgui_index_buffer = pulse_render_graph_import_dynamic_mesh_index_buffer(rg, state->mesh, drawData->TotalIdxCount);
	pulse_render_graph_add_uploadbufferpass(rg, "upload imgui index data", imgui_index_buffer, [](PulseUploadPassEncoder* encoder, void* passdata)
		{
			PassData* resolved_passdata = (PassData*)passdata;
			ImDrawData* drawData = resolved_passdata->drawData;
			uint32_t offset = 0;
			for (int n = 0; n < drawData->CmdListsCount; n++)
			{
				const ImDrawList* cmd_list = drawData->CmdLists[n];
				pulse_upload_pass_encoder_upload(encoder, offset, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx), cmd_list->IdxBuffer.Data);

				offset += cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx);
			}
		}, sizeof(PassData), (void**)&update_index_passdata);
	update_index_passdata->drawData = drawData;
    return { imgui_vertex_buffer, imgui_index_buffer };
}

void imgui_render_record_callback(PulseAppId app, PulseRenderGraphId graph, void* user_data) {
    auto* state = static_cast<pulse_imgui_plugin_state*>(user_data);
    if (!state || !state->context) {
        return;
    }
    ImDrawData* draw_data = state->draw_data;
    if (!draw_data || draw_data->TotalVtxCount <= 0 || draw_data->TotalIdxCount <= 0) {
        return;
    }
    if (!state->font_ready) {
        return; // 字体纹理尚未异步上传完成，跳过本帧。
    }

    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t window_entity = imgui_get_window_entity(world, state);
    if (!window_entity) {
        return;
    }

    PulseRGTextureHandle backbuffer =
        pulse_import_window_backbuffer(app, graph, window_entity);
    if (!pulse_rgtexture_handle_is_valid(backbuffer)) {
        return;
    }


    // // 动态 mesh 的 VB/IB 由 pulse_update_mesh_* 在 PreStore 提交，
    // // 这里声明本 pass 使用这两个动态缓冲，render graph 会建立上传依赖。
    // pulse_render_pass_builder_use_mesh(
    //     &pass, state->mesh,
    //     draw_data->TotalVtxCount, draw_data->TotalIdxCount);
    auto [imgui_vertex_buffer, imgui_index_buffer] = setup_mesh_data(state, draw_data, graph);
    // 字体纹理也导入 render graph，让 graph 知道本 pass 会采样它，
    // 从而在需要时插入正确的布局转换/顺序依赖。
    PulseRGTextureHandle font_rg =
        pulse_render_graph_import_texture(graph, state->font_texture);

    // 叠加 pass：LOAD 保留游戏画面。
    PulseRenderPassBuilder pass =
        pulse_render_graph_add_render_pass(graph, "PulseImGui");
    pulse_render_pass_builder_add_color_attachment(
        &pass, backbuffer,
        CGPU_LOAD_ACTION_LOAD, 0, CGPU_STORE_ACTION_STORE);
    if (pulse_rgtexture_handle_is_valid(font_rg)) {
        pulse_render_pass_builder_sample(&pass, font_rg);
    }
    pulse_render_pass_builder_use_buffer(&pass, imgui_vertex_buffer);
    pulse_render_pass_builder_use_buffer(&pass, imgui_index_buffer);

    pulse_imgui_plugin_state** state_pd = nullptr;
    pulse_render_pass_builder_set_executable(
        &pass, imgui_pass_executable,
        sizeof(pulse_imgui_plugin_state*), (void**)&state_pd);
    if (state_pd) {
        *state_pd = state;
    }
}

} // namespace

EPulseResult imgui_render_init(PulseAppId app, pulse_imgui_plugin_state* state) {
    const PulseRenderer* renderer = pulse_get_renderer(app);
    if (!renderer) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    ImGui::SetCurrentContext(state->context);

    // ---- 1. shader（同步构建，push constant "pc" 由 cgpu 反射自动进 root sig）----
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

    // 声明一个 NON_MATERIAL 属性使 set 0 成为 renderer_managed，
    // encoder 才会用 set_global_texture/set_global_sampler 构建并绑定 dset。
    PulseShaderProperty global_property = {
        .name = "imgui_global",
        .type = PULSE_SHADER_PROPERTY_TYPE_TEXTURE,
        .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL,
        .set = 0,
        .binding = 0,
        .offset = 0,
        .size = 0,
    };
    PulseShaderCreateFromBinaryDesc shader_desc = {};
    shader_desc.vs_data = imgui_vert_spv;
    shader_desc.vs_size = sizeof(imgui_vert_spv);
    shader_desc.fs_data = imgui_frag_spv;
    shader_desc.fs_size = sizeof(imgui_frag_spv);
    shader_desc.blend_desc = blend_desc;
    shader_desc.depth_desc = depth_desc;
    shader_desc.rasterizer_state = rasterizer_state;
    shader_desc.property_count = 1;
    shader_desc.p_properties = &global_property;

    state->shader = pulse_create_shader_from_binary(app, &shader_desc);
    if (!pulse_asset_handle_is_valid(pulse_shader_to_handle(state->shader))) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    // ---- 2. material（公共 API，draw_submesh 需要 material+mesh）----
    PulseMaterialCreateDesc material_desc = {};
    material_desc.shader = state->shader;
    state->material = pulse_create_material(app, &material_desc);
    if (!pulse_asset_handle_is_valid(pulse_material_to_handle(state->material))) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    // ---- 3. 动态 mesh（ImDrawVert 布局，pipeline 由 encoder 按 layout 自动管理）----
    CGPUVertexAttribute attributes[3] = {
        { "POSITION", 1, CGPU_VERTEX_FORMAT_FLOAT32X2, 0, 0, sizeof(float) * 2, CGPU_VERTEX_INPUT_RATE_VERTEX },
        { "TEXCOORD", 1, CGPU_VERTEX_FORMAT_FLOAT32X2, 0, sizeof(float) * 2, sizeof(float) * 2, CGPU_VERTEX_INPUT_RATE_VERTEX },
        { "COLOR", 1, CGPU_VERTEX_FORMAT_UNORM8X4, 0, sizeof(float) * 4, sizeof(uint32_t), CGPU_VERTEX_INPUT_RATE_VERTEX },
    };
    CGPUVertexLayout vertex_layout = {
        .attribute_count = 3,
        .p_attributes = attributes,
    };
    PulseMeshCreateDynamicDesc mesh_desc = {};
    mesh_desc.topology = CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    mesh_desc.index_stride = sizeof(ImDrawIdx);
    mesh_desc.layout = vertex_layout;

    state->mesh = pulse_create_mesh_dynamic(app, &mesh_desc);
    if (!pulse_asset_handle_is_valid(pulse_mesh_to_handle(state->mesh))) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    // ---- 4. 字体纹理（异步上传，pixel_data 会被资产系统拷贝）----
    ImGuiIO& io = ImGui::GetIO();
    unsigned char* pixels = nullptr;
    int font_width = 0;
    int font_height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &font_width, &font_height);

    PulseTextureCreateDesc tex_desc = {};
    tex_desc.desc.name = "PulseImGuiFontTexture";
    tex_desc.desc.width = (uint64_t)font_width;
    tex_desc.desc.height = (uint64_t)font_height;
    tex_desc.desc.depth = 1;
    tex_desc.desc.array_size = 1;
    tex_desc.desc.format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    tex_desc.desc.mip_levels = 1;
    tex_desc.desc.owner_queue = renderer->graphics_queue;
    tex_desc.desc.start_state = CGPU_RESOURCE_STATE_COPY_DEST;
    tex_desc.desc.descriptors = CGPU_RESOURCE_TYPE_TEXTURE;
    tex_desc.pixel_data = pixels;
    tex_desc.pixel_data_size = (uint64_t)font_width * font_height * 4;
    tex_desc.generate_mipmaps = false;

    state->font_request = pulse_create_texture(app, &tex_desc);
    if (!pulse_texture_is_alive(app, state->font_request)) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    // ---- 5. 字体采样器（公共 API，一次创建全生命周期使用）----
    PulseSamplerCreateDesc sampler_create_desc = {};
    sampler_create_desc.desc = {
        .min_filter = CGPU_FILTER_TYPE_LINEAR,
        .mag_filter = CGPU_FILTER_TYPE_LINEAR,
        .mipmap_mode = CGPU_MIP_MAP_MODE_LINEAR,
        .address_u = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_v = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
        .address_w = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
    };
    state->font_sampler = pulse_create_sampler(app, &sampler_create_desc);
    if (!pulse_asset_handle_is_valid(pulse_sampler_to_handle(state->font_sampler))) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    // ---- 6. 渲染记录回调（高 priority，在游戏 pass 之后叠加）----
    PulseRenderRecordCallbackDesc cb_desc = {
        .callback = imgui_render_record_callback,
        .user_data = state,
        .priority = kImGuiRecordPriority,
    };
    return pulse_add_render_record_callback(app, &cb_desc);
}

void imgui_render_shutdown(PulseAppId app, pulse_imgui_plugin_state* state) {
    if (!state) {
        return;
    }
    pulse_remove_render_record_callback(app, imgui_render_record_callback);
    // mesh/material/shader/texture/sampler 资产跟随 app 生命周期，不主动释放。
    auto asset_system = pulse_get_asset_system(app);
    pulse_asset_system_release(asset_system, pulse_mesh_to_handle(state->mesh), nullptr);
    state->mesh = {};
    pulse_asset_system_release(asset_system, pulse_material_to_handle(state->material), nullptr);
    state->material = {};
    pulse_asset_system_release(asset_system, pulse_shader_to_handle(state->shader), nullptr);
    state->shader = {};
    pulse_asset_system_release(asset_system, pulse_texture_to_handle(state->font_texture), nullptr);
    state->font_texture = {};
    pulse_asset_system_release(asset_system, pulse_sampler_to_handle(state->font_sampler), nullptr);
    state->font_sampler = {};
    state->font_ready = false;
}

} // namespace pulse_imgui_internal
