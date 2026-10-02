#include "../font_internal.h"

#include <cstdio>
#include <cstdlib>

namespace pulse_font_internal {

namespace {

constexpr int32_t kAtlasUploadRecordPriority = -900;

PulseTextureHandle create_page_texture(pulse_font_plugin_state* state) {
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
    const PulseTextureRequest request = pulse_create_texture(state->app, &texture_desc);
    if (!pulse_texture_is_ready(state->app, request)) {
        return PulseTextureHandle{};
    }
    return pulse_texture_get_handle(state->app, request);
}

void font_record_callback(PulseAppId app, PulseRenderGraphId graph, void* user_data) {
    auto* state = static_cast<pulse_font_plugin_state*>(user_data);
    if (!state || !graph) {
        return;
    }
    for (uint32_t page = 0; page < state->pages.size(); ++page) {
        const PulseTextureHandle handle = render_ensure_page(state, page);
        font_page_gpu& gpu = state->render.pages[page];
        if (gpu.last_uploaded_version == state->pages[page].version) {
            continue;
        }
        const uint64_t size = state->pages[page].pixels.size();
        if (size == 0) {
            continue;
        }
        PulseRGTextureHandle texture = pulse_render_graph_import_texture(graph, handle);
        if (!pulse_rgtexture_handle_is_valid(texture)) {
            continue;
        }
        gpu.last_uploaded_version = state->pages[page].version;
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
            if (pulse_asset_handle_is_valid(pulse_texture_to_handle(gpu.handle))) {
                pulse_asset_system_release(asset_system, pulse_texture_to_handle(gpu.handle), nullptr);
            }
            gpu = {};
        }
    }
    render.pages.clear();
    render.initialized = false;
}

PulseTextureHandle render_ensure_page(pulse_font_plugin_state* state, uint32_t page) {
    if (!state->render.initialized || page >= state->pages.size()) {
        return PulseTextureHandle{};
    }
    if (state->render.pages.size() <= page) {
        state->render.pages.resize((size_t)page + 1);
    }
    font_page_gpu& gpu = state->render.pages[page];
    if (pulse_asset_handle_is_valid(pulse_texture_to_handle(gpu.handle))) {
        return gpu.handle;
    }
    gpu.handle = create_page_texture(state);
    if (!pulse_asset_handle_is_valid(pulse_texture_to_handle(gpu.handle))) {
        std::fprintf(stderr, "pulse_font: failed to synchronously create atlas page %u texture\n", page);
        std::abort();
    }
    return gpu.handle;
}

}

using namespace pulse_font_internal;

extern "C" {

PulseTextureHandle pulse_font_page_texture(PulseAppId app, uint32_t page) {
    pulse_font_plugin_state* state = state_from_app(app);
    if (!state) {
        return PulseTextureHandle{};
    }
    return render_ensure_page(state, page);
}

}
