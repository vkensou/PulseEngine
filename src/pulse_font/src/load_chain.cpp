#include "font_internal.h"
#include "pulse_datalist.h"

#include <algorithm>
#include <new>

namespace pulse_font_internal {

namespace {

void destroy_font_chain_asset(void* ptr, void* user_data) {
    (void)user_data;
    auto* data = static_cast<font_chain_data*>(ptr);
    data->~font_chain_data();
}

EPulseAssetLoaderStatus step_font_chain_loader(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    (void)state;
    std::vector<PulseAssetHandle> fonts;
    fonts.reserve(ctx->dependencies_count);
    for (size_t i = 0; i < ctx->dependencies_count; ++i) {
        const PulseAssetDepRef& dep = ctx->p_dependencies[i].dep_ref;
        const PulseAssetHandle handle{ dep.type_id, dep.index, dep.generation };
        if (!pulse_asset_handle_is_valid(handle)) {
            *out_error = "font chain loader: dependency font is not available";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        fonts.push_back(handle);
    }
    auto* data = static_cast<font_chain_data*>(ctx->out_asset);
    new (data) font_chain_data{};
    data->fonts = std::move(fonts);
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

struct font_chain_file_load_state {
    bool deps_requested = false;
};

bool chain_font_paths_from_datalist(PulseDatalist* root, std::vector<std::string>& out_paths, const char** out_error) {
    PulseDatalist* fonts = pulse_datalist_get_obj(root, "fonts");
    if (!fonts || pulse_datalist_get_type(fonts, nullptr) != PULSE_DATALIST_TYPE_LIST) {
        *out_error = "font chain file loader: missing fonts list";
        return false;
    }
    const size_t count = pulse_datalist_count(fonts);
    if (count == 0) {
        *out_error = "font chain file loader: empty fonts list";
        return false;
    }
    for (size_t i = 0; i < count; ++i) {
        const char* path = pulse_datalist_get_string(pulse_datalist_get(fonts, i), nullptr, nullptr);
        if (!path || !path[0]) {
            *out_error = "font chain file loader: fonts entry is not a path string";
            return false;
        }
        out_paths.emplace_back(path);
    }
    return true;
}

EPulseAssetLoaderStatus step_font_chain_file_loader(void* state, const PulseAssetLoadTask* ctx, const char** out_error) {
    auto* file_state = static_cast<font_chain_file_load_state*>(state);
    PulseDatalist* dl = pulse_datalist_create_from_text(static_cast<const char*>(ctx->p_bytes), ctx->bytes_size);
    if (!dl) {
        *out_error = pulse_datalist_last_error();
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    std::vector<std::string> paths;
    if (!chain_font_paths_from_datalist(dl, paths, out_error)) {
        pulse_datalist_release(dl);
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    if (!file_state->deps_requested) {
        for (const std::string& path : paths) {
            const PulseFontRequest font_request = pulse_font_load(ctx->app, path.c_str(), 0u);
            const PulseAssetRequest asset_request = pulse_font_request_to_asset_request(font_request);
            if (!pulse_asset_request_is_valid(asset_request)) {
                pulse_datalist_release(dl);
                *out_error = "font chain file loader: failed to request font";
                return PULSE_ASSET_LOADER_STATUS_FAILED;
            }
            pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, asset_request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED);
        }
        pulse_datalist_release(dl);
        file_state->deps_requested = true;
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
    }
    pulse_datalist_release(dl);
    std::vector<PulseAssetHandle> fonts;
    fonts.reserve(paths.size());
    for (const std::string& path : paths) {
        const PulseFontRequest font_request = pulse_font_load(ctx->app, path.c_str(), 0u);
        const PulseAssetHandle handle = pulse_asset_system_get_handle(ctx->asset_system, pulse_font_request_to_asset_request(font_request));
        if (!pulse_asset_handle_is_valid(handle)) {
            *out_error = "font chain file loader: font dependency is not available";
            return PULSE_ASSET_LOADER_STATUS_FAILED;
        }
        fonts.push_back(handle);
    }
    font_chain_append_default(state_from_app(ctx->app), fonts);
    auto* data = static_cast<font_chain_data*>(ctx->out_asset);
    new (data) font_chain_data{};
    data->fonts = std::move(fonts);
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

PulseFontChainRequest chain_load_impl(PulseAppId app, const char* path) {
    PulseFontChainRequest result{};
    PulseAssetSystemId asset_system = app ? pulse_get_asset_system(app) : nullptr;
    if (!asset_system || !path || path[0] == '\0') {
        return result;
    }
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = PULSE_TYPE_FONT_CHAIN;
    desc.path = path;
    const PulseAssetRequest request = pulse_asset_system_load(asset_system, &desc);
    if (!pulse_asset_request_is_valid(request)) {
        return result;
    }
    result.index = request.index;
    result.generation = request.generation;
    return result;
}

}

void register_font_chain_type(PulseAssetSystemId asset_system, PulseAppId app) {
    PulseAssetTypeDesc type_desc{};
    type_desc.struct_size = sizeof(PulseAssetTypeDesc);
    type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
    type_desc.type_id = PULSE_TYPE_FONT_CHAIN;
    type_desc.size = sizeof(font_chain_data);
    type_desc.align = alignof(font_chain_data);
    type_desc.destroy = destroy_font_chain_asset;
    type_desc.user_data = app;
    pulse_asset_system_register_type(asset_system, &type_desc);
}

void font_chain_append_default(pulse_font_plugin_state* state, std::vector<PulseAssetHandle>& fonts) {
    if (!state || !pulse_asset_handle_is_valid(state->default_font)) {
        return;
    }
    for (const PulseAssetHandle& existing : fonts) {
        if (pulse_asset_handle_equals(existing, state->default_font)) {
            return;
        }
    }
    fonts.push_back(state->default_font);
}

void register_font_chain_loaders(PulseAssetSystemId asset_system) {
    PulseAssetLoaderDesc ld{};
    ld.struct_size = sizeof(PulseAssetLoaderDesc);
    ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
    ld.type_id = PULSE_TYPE_FONT_CHAIN;
    ld.extensions = nullptr;
    ld.loader_identifier = kChainLoaderId;
    ld.ctor = nullptr;
    ld.dtor = nullptr;
    ld.step = step_font_chain_loader;
    ld.loader_size = 0;
    ld.loader_align = 0;
    ld.user_data = nullptr;
    pulse_asset_system_register_loader(asset_system, &ld);

    ld.extensions = "fontchain";
    ld.loader_identifier = nullptr;
    ld.step = step_font_chain_file_loader;
    ld.loader_size = sizeof(font_chain_file_load_state);
    ld.loader_align = alignof(font_chain_file_load_state);
    pulse_asset_system_register_loader(asset_system, &ld);
}

}

using namespace pulse_font_internal;

extern "C" {

PulseFontChainRequest pulse_font_load_chain(PulseAppId app, const char* path) {
    return pulse_font_internal::chain_load_impl(app, path);
}

}
