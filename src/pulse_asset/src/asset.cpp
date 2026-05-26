#include "asset_internal.h"

#include <cstring>
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

    cancel_load_jobs(state);
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

static pulse_asset_handle load_impl(
    pulse_app_t app,
    uint64_t type_id,
    const char* path_or_name,
    const void* settings,
    bool from_memory,
    const void* data,
    uint64_t size,
    const pulse_asset_dependency* dependencies,
    uint32_t dependency_count
);
static bool is_load_in_progress_state(pulse_asset_state_t state);
static bool cached_slot_can_be_reused(const pulse_asset_state_o* state, pulse_asset_handle handle);

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
        !desc->extensions || !desc->step ||
        (desc->loader_size > 0 && desc->loader_align == 0) ||
        (desc->settings_size > 0 && desc->settings_align == 0)) {
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
    const char* path,
    const void* settings
) {
    return load_impl(app, type_id, path, settings, false, nullptr, 0, nullptr, 0);
}

pulse_asset_handle pulse_asset_load_with_deps(
    pulse_app_t app,
    uint64_t type_id,
    const char* path,
    const pulse_asset_dependency* dependencies,
    uint32_t dependency_count,
    const void* settings
) {
    return load_impl(app, type_id, path, settings, false, nullptr, 0, dependencies, dependency_count);
}

pulse_asset_handle pulse_asset_load_from_memory(
    pulse_app_t app,
    uint64_t type_id,
    const char* name,
    const void* data,
    uint64_t size,
    const void* settings
) {
    return load_impl(app, type_id, name, settings, true, data, size, nullptr, 0);
}

pulse_asset_handle pulse_asset_load_from_memory_with_deps(
    pulse_app_t app,
    uint64_t type_id,
    const char* name,
    const void* data,
    uint64_t size,
    const pulse_asset_dependency* dependencies,
    uint32_t dependency_count,
    const void* settings
) {
    return load_impl(app, type_id, name, settings, true, data, size, dependencies, dependency_count);
}

pulse_result_t pulse_asset_add_load_dependency(
    const pulse_asset_load_task* ctx,
    pulse_asset_handle dependency,
    pulse_dependency_flags_t flags
) {
    if (!ctx || !ctx->app || !ctx->dependency_hint) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }
    LoadJob* job = ctx->dependency_hint->parent;
    auto existing_it = std::find_if(job->dependencies.begin(), job->dependencies.end(), [&](const pulse_asset_dependency& existing) {
        return existing.handle.type_id == dependency.type_id &&
            existing.handle.index == dependency.index &&
            existing.handle.generation == dependency.generation;
    });
    if (existing_it != job->dependencies.end()) {
        if (!(flags & PULSE_DEP_OPTIONAL)) {
            existing_it->flags = PULSE_DEP_REQUIRED;
        }
    } else {
        job->dependencies.push_back({dependency, flags});
    }
    job->ctx.dependencies = job->dependencies.data();
    job->ctx.dependency_count = static_cast<uint32_t>(job->dependencies.size());
    return PULSE_OK;
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
    for (pulse_asset_handle dep_handle : slot->dependencies) {
        if (dep_handle.index == PULSE_ASSET_INVALID_INDEX) continue;
        AssetSlot* dep_slot = get_slot(state, dep_handle);
        if (dep_slot && dep_slot->state == PULSE_ASSET_STATE_LOADED) {
            dep_slot->pin_count += 1;
        }
    }
    out_ref->handle = handle;
    out_ref->ptr = slot->data.data;
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
        for (pulse_asset_handle dep_handle : slot->dependencies) {
            if (dep_handle.index == PULSE_ASSET_INVALID_INDEX) continue;
            AssetSlot* dep_slot = get_slot(state, dep_handle);
            if (dep_slot && dep_slot->pin_count > 0) {
                dep_slot->pin_count -= 1;
            }
        }
    }
    ref->handle = invalid_handle();
    ref->ptr = nullptr;
}

void pulse_asset_unload(pulse_app_t app, pulse_asset_handle handle) {
    pulse_asset_state_o* state = state_from_app(app);
    if (!state || is_invalid_handle(handle)) {
        return;
    }
    AssetSlot* slot = get_slot(state, handle);
    if (!slot || slot->pin_count == 0) {
        return;
    }
    if (slot->retiring_load_job) {
        // Defer unload requested from the loader dtor until the current job is fully retired.
        slot->pin_count -= 1;
        if (slot->pin_count == 0) {
            slot->state = PULSE_ASSET_STATE_PENDING_DELETE;
            slot->error = "asset unload pending";
        }
        return;
    }
    slot->pin_count -= 1;
    if (is_load_in_progress_state(slot->state)) {
        if (slot->pin_count == 0) {
            slot->state = PULSE_ASSET_STATE_PENDING_DELETE;
            slot->error = "asset unload pending";
        }
        return;
    }
    try_unload_slot(state, handle);
}

void pulse_asset_mark_modified(pulse_app_t app, pulse_asset_handle handle) {
    pulse_asset_state_o* state = state_from_app(app);
    AssetSlot* slot = get_slot(state, handle);
    if (slot && slot->state == PULSE_ASSET_STATE_LOADED) {
        slot->version += 1;
    }
}

} // extern "C"

namespace pulse_asset_internal {

static bool is_load_in_progress_state(pulse_asset_state_t state) {
    return state == PULSE_ASSET_STATE_WAITING_LOAD ||
        state == PULSE_ASSET_STATE_LOADING ||
        state == PULSE_ASSET_STATE_WAITING_DEPENDENCIES ||
        state == PULSE_ASSET_STATE_PROCESSING;
}

static bool cached_slot_can_be_reused(const pulse_asset_state_o* state, pulse_asset_handle handle) {
    const AssetSlot* slot = get_slot_const(state, handle);
    return slot && slot->state != PULSE_ASSET_STATE_PENDING_DELETE;
}

static pulse_asset_handle load_impl(
    pulse_app_t app,
    uint64_t type_id,
    const char* path_or_name,
    const void* settings,
    bool from_memory,
    const void* data,
    uint64_t size,
    const pulse_asset_dependency* dependencies,
    uint32_t dependency_count
) {
    pulse_asset_state_o* state = state_from_app(app);
    if (!state || type_id == 0) {
        return invalid_handle();
    }
    if (state->types.find(type_id) == state->types.end()) {
        return invalid_handle();
    }

    std::string slot_path = path_or_name ? normalize_path(path_or_name) : "";
    AssetLoader* request_loader = nullptr;
    if (path_or_name && path_or_name[0]) {
        request_loader = find_loader(state, type_id, slot_path);
    }
    pulse_asset_handle handle;

    if (from_memory) {
        if ((!path_or_name || !path_or_name[0]) && (!data || size == 0)) {
            handle = allocate_slot(state, type_id, "");
            if (is_invalid_handle(handle)) {
                return handle;
            }
            AssetSlot* slot = get_slot(state, handle);
            if (slot) {
                slot->state = PULSE_ASSET_STATE_LOADED;
                slot->constructed = true;
                slot->pin_count = 1;
            }
            return handle;
        }

        if (!path_or_name || !path_or_name[0] || !data || size == 0) {
            return invalid_handle();
        }

        PathKey key{type_id, slot_path};
        auto cached = state->path_cache.find(key);
        if (cached != state->path_cache.end() && cached_slot_can_be_reused(state, cached->second)) {
            return cached->second;
        }

        handle = allocate_slot(state, type_id, slot_path);
    } else {
        if (!path_or_name || !path_or_name[0]) {
            return invalid_handle();
        }

        PathKey key{type_id, slot_path};
        auto cached = state->path_cache.find(key);
        if (cached != state->path_cache.end() && cached_slot_can_be_reused(state, cached->second)) {
            return cached->second;
        }

        handle = allocate_slot(state, type_id, slot_path);
    }

    if (is_invalid_handle(handle)) {
        return handle;
    }

    if (!slot_path.empty()) {
        state->path_cache[{type_id, slot_path}] = handle;
    }

    AssetSlot* slot = get_slot(state, handle);
    if (!slot) return invalid_handle();

    slot->pin_count = 1;

    auto copy_request_settings = [&](PooledBlock& out) -> bool {
        if (!request_loader || request_loader->desc.settings_size == 0 || !settings) {
            return true;
        }
        return copy_pooled_block(
            state,
            out,
            settings,
            request_loader->desc.settings_size,
            request_loader->desc.settings_align);
    };

    auto enqueue_load_job = [&](LoadJobPhase phase) -> bool {
        LoadJob job{};
        job.handle = handle;
        job.phase = phase;
        job.source.from_memory = from_memory;
        if (from_memory) {
            job.source.memory_data.assign(
                static_cast<const uint8_t*>(data),
                static_cast<const uint8_t*>(data) + size);
        }
        if (dependency_count > 0 && dependencies) {
            job.dependencies.assign(dependencies, dependencies + dependency_count);
        }
        if (!copy_request_settings(job.settings)) {
            return false;
        }
        state->load_jobs.push_back(std::move(job));
        return true;
    };

    LoadJobPhase initial_phase = LoadJobPhase::PendingRead;
    if (dependency_count > 0 && dependencies) {
        bool has_unresolved_required = false;
        for (uint32_t i = 0; i < dependency_count; ++i) {
            const auto& dep = dependencies[i];
            if (dep.handle.index == PULSE_ASSET_INVALID_INDEX) {
                if (!(dep.flags & PULSE_DEP_OPTIONAL)) {
                    slot->state = PULSE_ASSET_STATE_FAILED;
                    slot->error = "required dependency has null handle";
                    return handle;
                }
                continue;
            }

            AssetSlot* dep_slot = get_slot(state, dep.handle);
            if (!dep_slot) {
                if (!(dep.flags & PULSE_DEP_OPTIONAL)) {
                    slot->state = PULSE_ASSET_STATE_FAILED;
                    slot->error = "required dependency handle is invalid";
                    return handle;
                }
                continue;
            }
            if (dep_slot->state == PULSE_ASSET_STATE_FAILED && !(dep.flags & PULSE_DEP_OPTIONAL)) {
                slot->state = PULSE_ASSET_STATE_FAILED;
                slot->error = "dependency asset failed to load";
                return handle;
            }
            if (dep_slot->state != PULSE_ASSET_STATE_LOADED && !(dep.flags & PULSE_DEP_OPTIONAL)) {
                has_unresolved_required = true;
            }
        }
        initial_phase = has_unresolved_required ? LoadJobPhase::WaitingDependencies : LoadJobPhase::PendingRead;
    }

    slot->state = initial_phase == LoadJobPhase::WaitingDependencies
        ? PULSE_ASSET_STATE_WAITING_DEPENDENCIES
        : PULSE_ASSET_STATE_WAITING_LOAD;

    if (!enqueue_load_job(initial_phase)) {
        slot->state = PULSE_ASSET_STATE_FAILED;
        slot->error = "failed to copy asset load settings";
        return handle;
    }

    return handle;
}

} // namespace pulse_asset_internal
