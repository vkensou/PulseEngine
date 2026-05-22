#include "asset_internal.h"

#include <new>

namespace pulse_asset_internal {

ECS_COMPONENT_DECLARE(pulse_asset_state_resource);

pulse_asset_plugin_desc normalize_plugin_desc(const pulse_asset_plugin_desc* desc) {
    pulse_asset_plugin_desc normalized = pulse_asset_plugin_desc_default();
    if (desc) {
        normalized = *desc;
    }
    normalized.struct_size = sizeof(pulse_asset_plugin_desc);
    normalized.version = PULSE_ASSET_PLUGIN_DESC_VERSION;
    if (!normalized.root_path || !normalized.root_path[0]) {
        normalized.root_path = "assets";
    }
    if (normalized.max_requests_per_update == 0) {
        normalized.max_requests_per_update = 8;
    }
    return normalized;
}

bool validate_plugin_desc(const pulse_asset_plugin_desc* desc) {
    return !desc ||
        (desc->struct_size == sizeof(pulse_asset_plugin_desc) &&
         desc->version == PULSE_ASSET_PLUGIN_DESC_VERSION);
}

pulse_asset_state_o* state_from_app(pulse_app_t app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(pulse_asset_state_resource) == 0) {
        return nullptr;
    }
    const pulse_asset_state_resource* resource =
        ecs_singleton_get(world, pulse_asset_state_resource);
    return resource ? resource->state : nullptr;
}

std::string normalize_extension(const char* extension) {
    std::string out = extension ? extension : "";
    if (!out.empty() && out[0] == '.') {
        out.erase(out.begin());
    }
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::vector<std::string> parse_extensions(const char* extensions) {
    std::vector<std::string> out;
    if (!extensions) {
        return out;
    }
    std::string current;
    for (const char* c = extensions; ; ++c) {
        if (*c == ',' || *c == '\0') {
            std::string normalized = normalize_extension(current.c_str());
            if (!normalized.empty()) {
                out.push_back(normalized);
            }
            current.clear();
            if (*c == '\0') {
                break;
            }
        }
        else if (*c != ' ' && *c != '\t') {
            current.push_back(*c);
        }
    }
    return out;
}

static bool has_loader_for_extension(
    const pulse_asset_state_o* state,
    uint64_t type_id,
    const std::vector<std::string>& extensions
) {
    for (const AssetLoader& loader : state->loaders) {
        if (loader.desc.type_id != type_id) {
            continue;
        }
        for (const std::string& existing : loader.extensions) {
            for (const std::string& extension : extensions) {
                if (existing == extension) {
                    return true;
                }
            }
        }
    }
    return false;
}

static pulse_result_t asset_plugin_build(pulse_app_t app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    pulse_asset_state_o* state = static_cast<pulse_asset_state_o*>(ctx);
    if (!world || !state) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    state->app = app;
    ECS_COMPONENT_DEFINE(world, pulse_asset_state_resource);

    pulse_asset_state_resource resource{};
    resource.state = state;
    ecs_singleton_set_ptr(world, pulse_asset_state_resource, &resource);

    install_process_system(state, world);
    if (!state->process_system) {
        return PULSE_ERROR_INTERNAL;
    }
    return PULSE_OK;
}

static void asset_plugin_shutdown(pulse_app_t app, void* ctx) {
    pulse_asset_state_o* state = static_cast<pulse_asset_state_o*>(ctx);
    ecs_world_t* world = pulse_app_world(app);

    cancel_active_loads(state);
    destroy_all_assets(state);
    uninstall_process_system(state, world);

    if (world && ecs_id(pulse_asset_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_asset_state_resource);
        if (ecs_is_alive(world, ecs_id(pulse_asset_state_resource))) {
            ecs_delete(world, ecs_id(pulse_asset_state_resource));
        }
        ecs_id(pulse_asset_state_resource) = 0;
    }
    delete state;
}

} // namespace pulse_asset_internal

using namespace pulse_asset_internal;

extern "C" {

pulse_asset_plugin_desc pulse_asset_plugin_desc_default(void) {
    pulse_asset_plugin_desc desc{};
    desc.struct_size = sizeof(pulse_asset_plugin_desc);
    desc.version = PULSE_ASSET_PLUGIN_DESC_VERSION;
    desc.root_path = "assets";
    desc.max_requests_per_update = 8;
    return desc;
}

pulse_result_t pulse_asset_add_plugin(
    pulse_app_t app,
    const pulse_asset_plugin_desc* desc
) {
    if (!app || !validate_plugin_desc(desc)) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_ERROR_DUPLICATE_PLUGIN;
    }

    pulse_asset_state_o* state = new (std::nothrow) pulse_asset_state_o();
    if (!state) {
        return PULSE_ERROR_INTERNAL;
    }
    state->desc = normalize_plugin_desc(desc);
    state->root_path = state->desc.root_path;

    pulse_plugin_desc plugin_desc = {
        sizeof(pulse_plugin_desc),
        PULSE_PLUGIN_DESC_VERSION,
        kPluginName,
        state,
        asset_plugin_build,
        nullptr,
        asset_plugin_shutdown,
    };

    pulse_result_t result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

pulse_result_t pulse_asset_register_type(
    pulse_app_t app,
    const pulse_asset_type_desc* desc
) {
    pulse_asset_state_o* state = state_from_app(app);
    if (!state || !desc || !desc->type_id || desc->struct_size != sizeof(pulse_asset_type_desc) ||
        desc->version != PULSE_ASSET_TYPE_DESC_VERSION ||
        desc->size == 0 || desc->align == 0) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    if (state->types.find(desc->type_id) != state->types.end()) {
        return PULSE_ERROR_INVALID_STATE;
    }

    AssetType type{};
    type.desc = *desc;
    state->types.emplace(desc->type_id, type);
    return PULSE_OK;
}

pulse_result_t pulse_asset_register_loader(
    pulse_app_t app,
    const pulse_asset_loader_desc* desc
) {
    pulse_asset_state_o* state = state_from_app(app);
    if (!state || !desc || desc->struct_size != sizeof(pulse_asset_loader_desc) ||
        desc->version != PULSE_ASSET_LOADER_DESC_VERSION || desc->type_id == 0 ||
        !desc->extensions || !desc->start || !desc->step) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }
    if (state->types.find(desc->type_id) == state->types.end()) {
        return PULSE_ERROR_NOT_FOUND;
    }
    std::vector<std::string> extensions = parse_extensions(desc->extensions);
    if (extensions.empty()) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }
    if (has_loader_for_extension(state, desc->type_id, extensions)) {
        return PULSE_ERROR_INVALID_STATE;
    }
    AssetLoader loader{};
    loader.desc = *desc;
    loader.extensions = std::move(extensions);
    state->loaders.push_back(std::move(loader));
    return PULSE_OK;
}

pulse_asset_handle pulse_asset_load(
    pulse_app_t app,
    uint64_t type_id,
    const char* path
) {
    pulse_asset_state_o* state = state_from_app(app);
    if (!state || type_id == 0 || !path || !path[0]) {
        return invalid_handle();
    }
    if (state->types.find(type_id) == state->types.end()) {
        return invalid_handle();
    }

    std::string normalized_path = normalize_path(path);
    if (normalized_path.empty()) {
        return invalid_handle();
    }

    PathKey key{type_id, normalized_path};
    auto cached = state->path_cache.find(key);
    if (cached != state->path_cache.end() && get_slot(state, cached->second)) {
        return cached->second;
    }

    pulse_asset_handle handle = allocate_slot(state, type_id, normalized_path);
    if (is_invalid_handle(handle)) {
        return handle;
    }
    state->path_cache[key] = handle;
    state->pending_requests.push_back({handle});
    return handle;
}

pulse_asset_handle pulse_asset_load_from_memory(
    pulse_app_t app,
    uint64_t type_id,
    const char* name,
    const void* data,
    uint64_t size
) {
    pulse_asset_state_o* state = state_from_app(app);
    if (!state || type_id == 0) {
        return invalid_handle();
    }
    if (state->types.find(type_id) == state->types.end()) {
        return invalid_handle();
    }

    // Anonymous slot: empty name with no data → allocate and mark LOADED immediately
    if ((!name || !name[0]) && (!data || size == 0)) {
        pulse_asset_handle handle = allocate_memory_slot(state, type_id, "");
        if (is_invalid_handle(handle)) {
            return handle;
        }
        AssetSlot* slot = get_slot(state, handle);
        if (slot) {
            slot->state = PULSE_ASSET_STATE_LOADED;
            slot->constructed = true;
        }
        return handle;
    }

    if (!name || !name[0] || !data || size == 0) {
        return invalid_handle();
    }
    std::string normalized_name = normalize_path(name);
    if (normalized_name.empty()) {
        return invalid_handle();
    }

    PathKey key{type_id, normalized_name};
    auto cached = state->path_cache.find(key);
    if (cached != state->path_cache.end() && get_slot(state, cached->second)) {
        return cached->second;
    }

    pulse_asset_handle handle = allocate_memory_slot(state, type_id, normalized_name);
    if (is_invalid_handle(handle)) {
        return handle;
    }
    state->path_cache[key] = handle;

    LoadRequest request{handle};
    request.from_memory = true;
    request.memory_data.assign(static_cast<const uint8_t*>(data),
                               static_cast<const uint8_t*>(data) + size);
    state->pending_requests.push_back(std::move(request));
    return handle;
}

pulse_asset_handle pulse_asset_load_with_deps(
    pulse_app_t app,
    uint64_t type_id,
    const char* path,
    const pulse_asset_handle* dependencies,
    uint32_t dependency_count
) {
    pulse_asset_state_o* state = state_from_app(app);
    if (!state || type_id == 0) {
        return invalid_handle();
    }
    if (state->types.find(type_id) == state->types.end()) {
        return invalid_handle();
    }

    std::string slot_path = path ? normalize_path(path) : "";
    pulse_asset_handle handle;

    if (!slot_path.empty()) {
        PathKey key{type_id, slot_path};
        auto cached = state->path_cache.find(key);
        if (cached != state->path_cache.end() && get_slot(state, cached->second)) {
            return cached->second;
        }
    }

    handle = allocate_slot(state, type_id, slot_path);
    if (is_invalid_handle(handle)) {
        return handle;
    }
    if (!slot_path.empty()) {
        state->path_cache[{type_id, slot_path}] = handle;
    }

    AssetSlot* slot = get_slot(state, handle);
    if (!slot) return invalid_handle();

    if (dependency_count > 0 && dependencies) {
        slot->state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
        slot->dependencies.assign(dependencies, dependencies + dependency_count);
        slot->unresolved_count = dependency_count;

        for (uint32_t i = 0; i < dependency_count; ++i) {
            AssetSlot* dep_slot = get_slot(state, dependencies[i]);
            if (dep_slot) {
                dep_slot->dependents.push_back(handle);
            }
        }
    } else {
        slot->state = PULSE_ASSET_STATE_WAITING_LOAD;
        state->pending_requests.push_back({handle});
    }

    return handle;
}

pulse_asset_state_t pulse_asset_get_state(
    pulse_app_t app,
    pulse_asset_handle handle
) {
    pulse_asset_state_o* state = state_from_app(app);
    const AssetSlot* slot = get_slot_const(state, handle);
    return slot ? slot->state : PULSE_ASSET_STATE_EMPTY;
}

bool pulse_asset_is_available(pulse_app_t app, pulse_asset_handle handle) {
    return pulse_asset_get_state(app, handle) == PULSE_ASSET_STATE_LOADED;
}

const char* pulse_asset_get_error(pulse_app_t app, pulse_asset_handle handle) {
    pulse_asset_state_o* state = state_from_app(app);
    const AssetSlot* slot = get_slot_const(state, handle);
    return slot && !slot->error.empty() ? slot->error.c_str() : nullptr;
}

bool pulse_asset_get_progress(
    pulse_app_t app,
    pulse_asset_handle handle,
    pulse_asset_progress* out_progress
) {
    if (!out_progress) {
        return false;
    }
    pulse_asset_state_o* state = state_from_app(app);
    AssetSlot* slot = get_slot(state, handle);
    if (!slot || slot->state != PULSE_ASSET_STATE_PROCESSING ||
        !slot->loader || !slot->loader->desc.progress) {
        return false;
    }
    return slot->loader->desc.progress(
        slot->loader_state,
        out_progress,
        slot->loader->desc.user_data
    );
}

bool pulse_asset_acquire(
    pulse_app_t app,
    pulse_asset_handle handle,
    pulse_asset_ref* out_ref
) {
    if (out_ref) {
        out_ref->handle = invalid_handle();
        out_ref->ptr = nullptr;
    }
    pulse_asset_state_o* state = state_from_app(app);
    AssetSlot* slot = get_slot(state, handle);
    if (!out_ref || !slot || slot->state != PULSE_ASSET_STATE_LOADED || !slot->constructed) {
        return false;
    }
    slot->pin_count += 1;
    for (const pulse_asset_handle& dep : slot->dependencies) {
        AssetSlot* dep_slot = get_slot(state, dep);
        if (dep_slot && dep_slot->state == PULSE_ASSET_STATE_LOADED) {
            dep_slot->pin_count += 1;
        }
    }
    out_ref->handle = handle;
    out_ref->ptr = slot->data;
    return true;
}

void pulse_asset_release(pulse_app_t app, pulse_asset_ref* ref) {
    if (!ref || !ref->ptr) {
        return;
    }
    pulse_asset_state_o* state = state_from_app(app);
    AssetSlot* slot = get_slot(state, ref->handle);
    if (slot && slot->pin_count > 0) {
        slot->pin_count -= 1;
        for (const pulse_asset_handle& dep : slot->dependencies) {
            AssetSlot* dep_slot = get_slot(state, dep);
            if (dep_slot && dep_slot->pin_count > 0) {
                dep_slot->pin_count -= 1;
            }
        }
    }
    ref->handle = invalid_handle();
    ref->ptr = nullptr;
}

void pulse_asset_mark_modified(pulse_app_t app, pulse_asset_handle handle) {
    pulse_asset_state_o* state = state_from_app(app);
    AssetSlot* slot = get_slot(state, handle);
    if (slot && slot->state == PULSE_ASSET_STATE_LOADED) {
        slot->version += 1;
    }
}

} // extern "C"