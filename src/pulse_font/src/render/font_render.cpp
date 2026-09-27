#include "../font_internal.h"

#include <cstdio>

namespace pulse_font_internal {

namespace {

constexpr int32_t kAtlasUploadRecordPriority = -900;

PulseTextureHandle resolve_page_texture(PulseAppId app, pulse_font_plugin_state* state, uint32_t page) {
    font_render_state& render = state->render;
    if (page >= render.pages.size()) {
        return PulseTextureHandle{};
    }
    font_page_gpu& gpu = render.pages[page];
    if (!gpu.requested) {
        return PulseTextureHandle{};
    }
    if (gpu.ready) {
        return gpu.handle;
    }
    if (!pulse_texture_is_alive(app, gpu.request)) {
        gpu.requested = false;
        return PulseTextureHandle{};
    }
    if (!pulse_texture_is_ready(app, gpu.request)) {
        return PulseTextureHandle{};
    }
    gpu.handle = pulse_texture_get_handle(app, gpu.request);
    gpu.ready = pulse_asset_handle_is_valid(pulse_texture_to_handle(gpu.handle));
    if (!gpu.ready) {
        gpu.handle = {};
        return PulseTextureHandle{};
    }
    gpu.last_uploaded_version = 0;
    return gpu.handle;
}

void font_record_callback(PulseAppId app, PulseRenderGraphId graph, void* user_data) {
    auto* state = static_cast<pulse_font_plugin_state*>(user_data);
    if (!state || !graph) {
        return;
    }
    font_render_state& render = state->render;
    for (uint32_t page = 0; page < state->pages.size(); ++page) {
        render_ensure_page(state, page);
    }
    for (uint32_t page = 0; page < render.pages.size() && page < state->pages.size(); ++page) {
        if (!render.pages[page].requested) {
            continue;
        }
        if (!pulse_asset_handle_is_valid(pulse_texture_to_handle(resolve_page_texture(app, state, page)))) {
            continue;
        }
        const uint64_t version = state->pages[page].version;
        if (render.pages[page].last_uploaded_version == version) {
            continue;
        }
        const uint64_t size = state->pages[page].pixels.size();
        if (size == 0) {
            continue;
        }
        PulseRGTextureHandle texture = pulse_render_graph_import_texture(graph, render.pages[page].handle);
        if (!pulse_rgtexture_handle_is_valid(texture)) {
            continue;
        }
        render.pages[page].last_uploaded_version = version;
        pulse_render_graph_add_uploadtexturepass_ex(graph, "PulseFontAtlasUpload", texture, 0, 0, size, 0, state->pages[page].pixels.data(), nullptr, 0, nullptr);
    }
}

}

EPulseResult render_init(pulse_font_plugin_state* state) {
    font_render_state& render = state->render;
    if (render.initialized) {
        return PULSE_RESULT_OK;
    }
    PulseAppId app = state->app;
    if (!app || !pulse_get_renderer(app)) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }
    PulseRenderRecordCallbackDesc callback_desc = {
        .callback = font_record_callback,
        .user_data = state,
        .priority = kAtlasUploadRecordPriority,
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
    if (!render.initialized) {
        return;
    }
    PulseAppId app = state->app;
    if (app && pulse_get_renderer(app)) {
        pulse_remove_render_record_callback(app, font_record_callback);
        PulseAssetSystemId asset_system = pulse_get_asset_system(app);
        for (font_page_gpu& gpu : render.pages) {
            if (!gpu.requested) {
                continue;
            }
            PulseTextureHandle handle = pulse_texture_is_alive(app, gpu.request) ? pulse_texture_get_handle(app, gpu.request) : gpu.handle;
            if (pulse_asset_handle_is_valid(pulse_texture_to_handle(handle))) {
                pulse_asset_system_release(asset_system, pulse_texture_to_handle(handle), nullptr);
            }
            gpu = {};
        }
    }
    render.pages.clear();
    render.initialized = false;
}

void render_ensure_page(pulse_font_plugin_state* state, uint32_t page) {
    if (!state->app || !pulse_get_renderer(state->app)) {
        return;
    }
    if (state->render.pages.size() <= page) {
        state->render.pages.resize((size_t)page + 1);
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

PulseTextureHandle pulse_font_page_texture(PulseAppId app, uint32_t page) {
    pulse_font_plugin_state* state = state_from_app(app);
    if (!state || !state->render.initialized) {
        return PulseTextureHandle{};
    }
    render_ensure_page(state, page);
    return resolve_page_texture(app, state, page);
}

}
