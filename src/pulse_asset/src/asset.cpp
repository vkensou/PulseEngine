#include "asset_internal.h"

namespace pulse::asset {

pulse_asset_plugin_desc default_plugin_desc() {
    pulse_asset_plugin_desc desc{};
    desc.struct_size = sizeof(pulse_asset_plugin_desc);
    desc.version = PULSE_ASSET_PLUGIN_DESC_VERSION;
    desc.root_path = "assets";
    desc.max_requests_per_update = 8;
    return desc;
}

pulse_asset_plugin_desc normalize_plugin_desc(const pulse_asset_plugin_desc* desc) {
    pulse_asset_plugin_desc normalized = default_plugin_desc();
    if (desc) {
        normalized = *desc;
    }
    normalized.struct_size = sizeof(pulse_asset_plugin_desc);
    normalized.version = PULSE_ASSET_PLUGIN_DESC_VERSION;
    if (!normalized.root_path || !normalized.root_path[0]) {
        normalized.root_path = ".";
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

pulse_asset_handle invalid_handle() {
    return pulse_asset_handle_make_invalid();
}

bool is_invalid_handle(pulse_asset_handle handle) {
    return !pulse_asset_handle_is_valid(handle);
}

bool handles_equal(pulse_asset_handle a, pulse_asset_handle b) {
    return pulse_asset_handle_equals(a, b);
}

AssetSystem::AssetSystem(const pulse_asset_plugin_desc& desc)
    : desc_(desc),
      root_path_(desc.root_path ? desc.root_path : "", &memory_pool_),
      registry_(&memory_pool_),
      storage_(&memory_pool_, registry_),
      load_queue_(&memory_pool_) {
}

void AssetSystem::process_load_requests() {
    load_queue_.process(*this);
}

pulse_result_t AssetSystem::register_type(const pulse_asset_type_desc* desc) {
    return registry_.register_type(desc);
}

pulse_result_t AssetSystem::register_loader(const pulse_asset_loader_desc* desc) {
    return registry_.register_loader(desc);
}

pulse_asset_handle AssetSystem::load(const pulse_asset_load_desc* desc) {
    if (!desc || desc->struct_size != sizeof(pulse_asset_load_desc) ||
        desc->version != PULSE_ASSET_LOAD_DESC_VERSION) {
        return invalid_handle();
    }

    LoadRequest request{};
    request.source = PULSE_ASSET_LOAD_SOURCE_FILE;
    request.type_id = desc->type_id;
    request.path_or_name = desc->path;
    request.settings = desc->settings;
    request.flags = desc->flags;
    request.dependencies = desc->dependencies;
    request.dependency_count = desc->dependency_count;
    return load_impl(request);
}

pulse_asset_handle AssetSystem::load_from_memory(const pulse_asset_memory_load_desc* desc) {
    if (!desc || desc->struct_size != sizeof(pulse_asset_memory_load_desc) ||
        desc->version != PULSE_ASSET_MEMORY_LOAD_DESC_VERSION) {
        return invalid_handle();
    }

    LoadRequest request{};
    request.source = PULSE_ASSET_LOAD_SOURCE_MEMORY;
    request.type_id = desc->type_id;
    request.path_or_name = desc->path;
    request.settings = desc->settings;
    request.flags = desc->flags;
    request.data = desc->data;
    request.size = desc->size;
    request.dependencies = desc->dependencies;
    request.dependency_count = desc->dependency_count;
    return load_impl(request);
}

pulse_asset_handle AssetSystem::build_asset(const pulse_asset_build_desc* desc) {
    if (!desc || desc->struct_size != sizeof(pulse_asset_build_desc) ||
        desc->version != PULSE_ASSET_BUILD_DESC_VERSION) {
        return invalid_handle();
    }

    LoadRequest request{};
    request.source = PULSE_ASSET_LOAD_SOURCE_BUILDER;
    request.type_id = desc->type_id;
    request.path_or_name = desc->name;
    request.settings = desc->settings;
    request.dependencies = desc->dependencies;
    request.dependency_count = desc->dependency_count;
    return load_impl(request);
}

pulse_asset_state_t AssetSystem::get_state(pulse_asset_handle handle) const {
    auto slot = storage_.get_slot(handle);
    return slot ? slot->slot.state : PULSE_ASSET_STATE_EMPTY;
}

const char* AssetSystem::get_error(pulse_asset_handle handle) const {
    auto slot = storage_.get_slot(handle);
    return slot && !slot->slot.error.empty() ? slot->slot.error.c_str() : nullptr;
}

bool AssetSystem::acquire(pulse_asset_handle handle, pulse_asset_ref* out_ref) {
    if (out_ref) {
        out_ref->handle = invalid_handle();
        out_ref->ptr = nullptr;
    }

    auto slot = storage_.get_slot(handle);
    if (!out_ref || !slot ||
        slot->slot.state != PULSE_ASSET_STATE_LOADED || !slot->slot.constructed) {
        return false;
    }

    slot->slot.pin_count += 1;
    storage_.dependencies().pin_committed_dependencies(storage_, slot->slot);
    out_ref->handle = handle;
    out_ref->ptr = slot->slot.data.data;
    return true;
}

void AssetSystem::release(pulse_asset_ref* ref) {
    if (!ref || !ref->ptr) {
        return;
    }

    auto slot = storage_.get_slot(ref->handle);
    if (slot && slot->slot.pin_count > 0) {
        slot->slot.pin_count -= 1;
        storage_.dependencies().unpin_committed_dependencies(storage_, slot->slot);
    }

    ref->handle = invalid_handle();
    ref->ptr = nullptr;
}

void AssetSystem::unload(pulse_asset_handle handle) {
    if (is_invalid_handle(handle)) {
        return;
    }

    auto slot = storage_.get_slot(handle);
    if (!slot || slot->slot.pin_count == 0) {
        return;
    }

    if (slot->slot.retiring_load_job) {
        slot->slot.pin_count -= 1;
        if (slot->slot.pin_count == 0) {
            slot->slot.state = PULSE_ASSET_STATE_PENDING_DELETE;
            slot->slot.error = "asset unload pending";
        }
        return;
    }

    slot->slot.pin_count -= 1;
    if (is_load_in_progress_state(slot->slot.state)) {
        if (slot->slot.pin_count == 0) {
            slot->slot.state = PULSE_ASSET_STATE_PENDING_DELETE;
            slot->slot.error = "asset unload pending";
        }
        return;
    }

    storage_.try_unload_slot(*slot, handle);
}

void AssetSystem::mark_modified(pulse_asset_handle handle) {
    auto slot = storage_.get_slot(handle);
    if (slot && slot->slot.state == PULSE_ASSET_STATE_LOADED) {
        slot->slot.version += 1;
    }
}

void AssetSystem::force_unload_assets(uint64_t type_id) {
    if (type_id == 0) {
        return;
    }

    load_queue_.cancel_type(*this, type_id);
    storage_.force_destroy_assets(type_id);
}

pulse_asset_handle AssetSystem::load_impl(const LoadRequest& request) {
    if (request.type_id == 0 || !registry_.find_type(request.type_id) ||
        !request_dependencies_are_valid(request)) {
        return invalid_handle();
    }

    std::pmr::string slot_path(resource());
    if (request.path_or_name) {
        slot_path = AssetIo::normalize_path(request.path_or_name, resource());
    }

    AssetLoader* request_loader = nullptr;
    if (request.source == PULSE_ASSET_LOAD_SOURCE_BUILDER) {
        request_loader = registry_.find_builder_loader(request.type_id);
        if (!request_loader) {
            return invalid_handle();
        }
    } else {
        if (slot_path.empty()) {
            return invalid_handle();
        }
        if (request.source == PULSE_ASSET_LOAD_SOURCE_MEMORY && (!request.data || request.size == 0)) {
            return invalid_handle();
        }
        request_loader = registry_.find_loader(request.type_id, slot_path);
        if (!request_loader) {
            return invalid_handle();
        }
    }

    if (request.source != PULSE_ASSET_LOAD_SOURCE_BUILDER &&
        !(request.flags & PULSE_ASSET_LOAD_SKIP_CACHE)) {
        pulse_asset_handle cached = storage_.find_cached(request.type_id, slot_path);
        if (!is_invalid_handle(cached) && storage_.cached_slot_can_be_reused(cached)) {
            return cached;
        }
    }

    AssetSlotAllocation allocation = storage_.allocate_slot(request.type_id, slot_path);
    pulse_asset_handle handle = allocation.handle;
    if (is_invalid_handle(handle)) {
        return handle;
    }

    AssetSlot* slot = allocation.slot;
    if (!slot) {
        return invalid_handle();
    }

    auto unload_failed_builder = [&]() {
        unload(handle);
        return invalid_handle();
    };

    if (request.source != PULSE_ASSET_LOAD_SOURCE_BUILDER && !slot_path.empty()) {
        storage_.cache_path(request.type_id, slot_path, handle);
    }

    slot->pin_count = 1;
    LoadJobPhase initial_phase = choose_initial_phase(request, *slot);
    if (slot->state == PULSE_ASSET_STATE_FAILED) {
        if (request.source == PULSE_ASSET_LOAD_SOURCE_BUILDER) {
            return unload_failed_builder();
        }
        return handle;
    }

    slot->state = initial_phase == LoadJobPhase::WaitingDependencies
        ? PULSE_ASSET_STATE_WAITING_DEPENDENCIES
        : PULSE_ASSET_STATE_WAITING_LOAD;

    LoadJob job(resource());
    if (!init_load_job(job, request, handle, request_loader, initial_phase)) {
        slot->state = PULSE_ASSET_STATE_FAILED;
        slot->error = "failed to copy asset load settings";
        if (request.source == PULSE_ASSET_LOAD_SOURCE_BUILDER) {
            return unload_failed_builder();
        }
        return handle;
    }

    auto job_it = load_queue_.enqueue(std::move(job));
    if (request.source == PULSE_ASSET_LOAD_SOURCE_BUILDER &&
        initial_phase == LoadJobPhase::PendingRead) {
        LoadJobOutcome outcome = load_queue_.process_immediate_builder(*this, job_it);
        if (outcome == LoadJobOutcome::Failed || outcome == LoadJobOutcome::Cancelled) {
            return unload_failed_builder();
        }
    }

    return handle;
}

bool AssetSystem::request_dependencies_are_valid(const LoadRequest& request) const {
    return request.dependency_count == 0 || request.dependencies != nullptr;
}

LoadJobPhase AssetSystem::choose_initial_phase(const LoadRequest& request, AssetSlot& slot) {
    bool has_unresolved_required = false;
    if (request.dependency_count == 0 || !request.dependencies) {
        return LoadJobPhase::PendingRead;
    }

    for (uint32_t i = 0; i < request.dependency_count; ++i) {
        const pulse_asset_dependency& dep = request.dependencies[i];
        if (is_invalid_handle(dep.handle)) {
            if (!(dep.flags & PULSE_DEP_OPTIONAL)) {
                slot.state = PULSE_ASSET_STATE_FAILED;
                slot.error = "required dependency has null handle";
            }
            continue;
        }

        auto dep_slot = storage_.get_slot(dep.handle);
        if (!dep_slot) {
            if (!(dep.flags & PULSE_DEP_OPTIONAL)) {
                slot.state = PULSE_ASSET_STATE_FAILED;
                slot.error = "required dependency handle is invalid";
            }
            continue;
        }
        if (dep_slot->slot.state == PULSE_ASSET_STATE_FAILED && !(dep.flags & PULSE_DEP_OPTIONAL)) {
            slot.state = PULSE_ASSET_STATE_FAILED;
            slot.error = "dependency asset failed to load";
            continue;
        }
        if (dep_slot->slot.state != PULSE_ASSET_STATE_LOADED && !(dep.flags & PULSE_DEP_OPTIONAL)) {
            has_unresolved_required = true;
        }
    }

    return has_unresolved_required ? LoadJobPhase::WaitingDependencies : LoadJobPhase::PendingRead;
}

bool AssetSystem::copy_request_settings(const AssetLoader& loader, const LoadRequest& request, PooledBlock& out) {
    if (loader.desc.settings_size == 0 || !request.settings) {
        return true;
    }
    return out.copy(resource(), request.settings, loader.desc.settings_size, loader.desc.settings_align);
}

bool AssetSystem::init_load_job(LoadJob& job, const LoadRequest& request, pulse_asset_handle handle, AssetLoader* loader, LoadJobPhase phase) {
    job.handle = handle;
    job.phase = phase;
    job.loader = loader;
    job.source.kind = request.source;
    if (request.source == PULSE_ASSET_LOAD_SOURCE_MEMORY) {
        job.source.memory_data.assign(
            static_cast<const uint8_t*>(request.data),
            static_cast<const uint8_t*>(request.data) + request.size);
    }
    if (request.dependency_count > 0 && request.dependencies) {
        job.dependencies.assign(request.dependencies, request.dependencies + request.dependency_count);
    }
    return loader && copy_request_settings(*loader, request, job.settings);
}

bool AssetSystem::is_load_in_progress_state(pulse_asset_state_t state) {
    return state == PULSE_ASSET_STATE_WAITING_LOAD ||
        state == PULSE_ASSET_STATE_LOADING ||
        state == PULSE_ASSET_STATE_WAITING_DEPENDENCIES ||
        state == PULSE_ASSET_STATE_PROCESSING;
}

} // namespace pulse::asset

extern "C" {

pulse_asset_plugin_desc pulse_asset_plugin_desc_default(void) {
    return pulse::asset::default_plugin_desc();
}

pulse_result_t pulse_asset_add_plugin(
    pulse_app_t app,
    const pulse_asset_plugin_desc* desc
) {
    if (!app || !pulse::asset::validate_plugin_desc(desc)) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, pulse::asset::kPluginName)) {
        return PULSE_ERROR_DUPLICATE_PLUGIN;
    }

    pulse_asset_plugin_desc normalized = pulse::asset::normalize_plugin_desc(desc);
    auto* system = new (std::nothrow) pulse::asset::AssetSystem(normalized);
    if (!system) {
        return PULSE_ERROR_INTERNAL;
    }

    pulse_plugin_desc plugin_desc = {
        sizeof(pulse_plugin_desc),
        PULSE_PLUGIN_DESC_VERSION,
        pulse::asset::kPluginName,
        system,
        pulse::asset::asset_plugin_build_callback,
        nullptr,
        pulse::asset::asset_plugin_shutdown_callback,
    };

    pulse_result_t result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_OK && !pulse_app_has_plugin(app, pulse::asset::kPluginName)) {
        delete system;
    }
    return result;
}

pulse_result_t pulse_asset_register_type(
    pulse_app_t app,
    const pulse_asset_type_desc* desc
) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    return system ? system->register_type(desc) : PULSE_ERROR_INVALID_ARGUMENT;
}

pulse_result_t pulse_asset_register_loader(
    pulse_app_t app,
    const pulse_asset_loader_desc* desc
) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    return system ? system->register_loader(desc) : PULSE_ERROR_INVALID_ARGUMENT;
}

pulse_asset_handle pulse_asset_load(
    pulse_app_t app,
    const pulse_asset_load_desc* desc
) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    return system ? system->load(desc) : pulse::asset::invalid_handle();
}

pulse_asset_handle pulse_asset_load_from_memory(
    pulse_app_t app,
    const pulse_asset_memory_load_desc* desc
) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    return system ? system->load_from_memory(desc) : pulse::asset::invalid_handle();
}

pulse_asset_handle pulse_asset_build(
    pulse_app_t app,
    const pulse_asset_build_desc* desc
) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    return system ? system->build_asset(desc) : pulse::asset::invalid_handle();
}

pulse_result_t pulse_asset_add_load_dependency(
    const pulse_asset_load_task* ctx,
    pulse_asset_handle dependency,
    pulse_dependency_flags_t flags
) {
    if (!ctx || !ctx->app || !ctx->dependency_hint || !ctx->dependency_hint->parent) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }
    return ctx->dependency_hint->parent->add_dependency(dependency, flags);
}

pulse_asset_state_t pulse_asset_get_state(
    pulse_app_t app,
    pulse_asset_handle handle
) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    return system ? system->get_state(handle) : PULSE_ASSET_STATE_EMPTY;
}

bool pulse_asset_is_alive(pulse_app_t app, pulse_asset_handle handle)
{
	auto state = pulse_asset_get_state(app, handle);
    return !(state == PULSE_ASSET_STATE_EMPTY || state == PULSE_ASSET_STATE_FAILED || state == PULSE_ASSET_STATE_PENDING_DELETE);
}

bool pulse_asset_is_ready(pulse_app_t app, pulse_asset_handle handle) {
    return pulse_asset_get_state(app, handle) == PULSE_ASSET_STATE_LOADED;
}

const char* pulse_asset_get_error(pulse_app_t app, pulse_asset_handle handle) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    return system ? system->get_error(handle) : nullptr;
}

bool pulse_asset_acquire(
    pulse_app_t app,
    pulse_asset_handle handle,
    pulse_asset_ref* out_ref
) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    if (!system) {
        if (out_ref) {
            out_ref->handle = pulse::asset::invalid_handle();
            out_ref->ptr = nullptr;
        }
        return false;
    }
    return system->acquire(handle, out_ref);
}

void pulse_asset_release(pulse_app_t app, pulse_asset_ref* ref) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    if (system) {
        system->release(ref);
    }
}

void pulse_asset_unload(pulse_app_t app, pulse_asset_handle handle) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    if (system) {
        system->unload(handle);
    }
}

void pulse_asset_mark_modified(pulse_app_t app, pulse_asset_handle handle) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    if (system) {
        system->mark_modified(handle);
    }
}

void pulse_asset_force_unload_assets(pulse_app_t app, uint64_t type_id) {
    pulse::asset::AssetSystem* system = pulse::asset::system_from_app(app);
    if (system) {
        system->force_unload_assets(type_id);
    }
}

} // extern "C"
