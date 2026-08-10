#include "asset_internal.h"

namespace pulse::asset {

PulseAssetHandle invalid_handle() {
    return pulse_asset_handle_make_invalid();
}

bool is_invalid_handle(PulseAssetHandle handle) {
    return !pulse_asset_handle_is_valid(handle);
}

bool handles_equal(PulseAssetHandle a, PulseAssetHandle b) {
    return pulse_asset_handle_equals(a, b);
}

AssetSystem::AssetSystem(const PulseAssetPluginDesc& desc)
    : desc_(desc),
      root_path_(desc.root_path ? desc.root_path : "", &memory_pool_),
      registry_(&memory_pool_),
      storage_(&memory_pool_, registry_),
      load_queue_(&memory_pool_) {
}

void AssetSystem::process_load_requests() {
    load_queue_.process(*this);
}

EPulseResult AssetSystem::register_type(const PulseAssetTypeDesc* desc) {
    return registry_.register_type(desc);
}

EPulseResult AssetSystem::register_loader(const PulseAssetLoaderDesc* desc) {
    return registry_.register_loader(desc);
}

PulseAssetHandle AssetSystem::load(const PulseAssetLoadDesc* desc) {
    if (!desc || desc->struct_size != sizeof(PulseAssetLoadDesc) ||
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

PulseAssetHandle AssetSystem::load_from_memory(const PulseAssetMemoryLoadDesc* desc) {
    if (!desc || desc->struct_size != sizeof(PulseAssetMemoryLoadDesc) ||
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

PulseAssetHandle AssetSystem::build_asset(const PulseAssetBuildDesc* desc) {
    if (!desc || desc->struct_size != sizeof(PulseAssetBuildDesc) ||
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

EPulseAssetState AssetSystem::get_state(PulseAssetHandle handle) const {
    auto slot = storage_.get_slot(handle);
    return slot ? slot->slot.state : PULSE_ASSET_STATE_EMPTY;
}

const char* AssetSystem::get_error(PulseAssetHandle handle) const {
    auto slot = storage_.get_slot(handle);
    return slot && !slot->slot.error.empty() ? slot->slot.error.c_str() : nullptr;
}

bool AssetSystem::retain(PulseAssetHandle handle, EPulseRetainErrorCode* out_error) {
    auto slot = storage_.get_slot(handle);
    if (!slot || slot->slot.state == PULSE_ASSET_STATE_EMPTY) {
        if (out_error) {
            *out_error = PULSE_RETAIN_ERROR_CODE_ASSET_IS_RELEASED;
        }
        return false;
    }

    if (slot->slot.state == PULSE_ASSET_STATE_PENDING_DELETE) {
        if (out_error) {
            *out_error = PULSE_RETAIN_ERROR_CODE_ASSET_IS_PENDING_DELETE;
        }
        return false;
    }

    slot->slot.pin_count += 1;
    storage_.dependencies().pin_committed_dependencies(storage_, slot->slot);
    return true;
}

bool AssetSystem::release(PulseAssetHandle handle, EPulseReleaseErrorCode* out_error) {
    auto slot = storage_.get_slot(handle);
    if (!slot || slot->slot.pin_count == 0) {
        if (out_error) {
            *out_error = PULSE_RELEASE_ERROR_CODE_ASSET_IS_OVER_RELEASED;
        }
        return false;
    }

    slot->slot.pin_count -= 1;
    storage_.dependencies().unpin_committed_dependencies(storage_, slot->slot);

    if (slot->slot.retiring_load_job) {
        if (slot->slot.pin_count == 0) {
            slot->slot.state = PULSE_ASSET_STATE_PENDING_DELETE;
            slot->slot.error = "asset unload pending";
        }
        return true;
    }

    if (is_load_in_progress_state(slot->slot.state)) {
        if (slot->slot.pin_count == 0) {
            slot->slot.state = PULSE_ASSET_STATE_PENDING_DELETE;
            slot->slot.error = "asset unload pending";
        }
        return true;
    }

    if (slot->slot.pin_count == 0) {
        storage_.try_unload_slot(*slot, handle);
    }
    return true;
}

bool AssetSystem::borrow(PulseAssetHandle handle, void** out_ptr, EPulseBorrowErrorCode* out_error) {
    if (out_ptr) {
        *out_ptr = nullptr;
    }

    auto slot = storage_.get_slot(handle);
    if (!slot) {
        if (out_error) {
            *out_error = PULSE_BORROW_ERROR_CODE_ASSET_IS_RELEASED;
        }
        return false;
    }

    if (slot->slot.state == PULSE_ASSET_STATE_PENDING_DELETE) {
        if (out_error) {
            *out_error = PULSE_BORROW_ERROR_CODE_ASSET_IS_PENDING_DELETE;
        }
        return false;
    }

    if (out_ptr) {
        *out_ptr = slot->slot.data.data;
    }
    return true;
}

void AssetSystem::mark_modified(PulseAssetHandle handle) {
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

PulseAssetHandle AssetSystem::load_impl(const LoadRequest& request) {
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
        PulseAssetHandle cached = storage_.find_cached(request.type_id, slot_path);
        if (!is_invalid_handle(cached) && storage_.cached_slot_can_be_reused(cached)) {
            return cached;
        }
    }

    AssetSlotAllocation allocation = storage_.allocate_slot(request.type_id, slot_path);
    PulseAssetHandle handle = allocation.handle;
    if (is_invalid_handle(handle)) {
        return handle;
    }

    AssetSlot* slot = allocation.slot;
    if (!slot) {
        return invalid_handle();
    }

    auto release_failed_builder = [&]() {
        release(handle, nullptr);
        return invalid_handle();
    };

    if (request.source != PULSE_ASSET_LOAD_SOURCE_BUILDER && !slot_path.empty()) {
        storage_.cache_path(request.type_id, slot_path, handle);
    }

    slot->pin_count = 1;
    LoadJobPhase initial_phase = choose_initial_phase(request, *slot);
    if (slot->state == PULSE_ASSET_STATE_FAILED) {
        if (request.source == PULSE_ASSET_LOAD_SOURCE_BUILDER) {
            return release_failed_builder();
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
            return release_failed_builder();
        }
        return handle;
    }

    auto job_it = load_queue_.enqueue(std::move(job));
    if (request.source == PULSE_ASSET_LOAD_SOURCE_BUILDER &&
        initial_phase == LoadJobPhase::PendingRead) {
        LoadJobOutcome outcome = load_queue_.process_immediate_builder(*this, job_it);
        if (outcome == LoadJobOutcome::Failed || outcome == LoadJobOutcome::Cancelled) {
            return release_failed_builder();
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
        const PulseAssetDependency& dep = request.dependencies[i];
        PulseAssetHandle dep_handle = dep_ref_to_handle(dep.dep_ref);
        if (is_invalid_handle(dep_handle)) {
            if (!(dep.requirement & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL)) {
                slot.state = PULSE_ASSET_STATE_FAILED;
                slot.error = "required dependency has null handle";
            }
            continue;
        }

        auto dep_slot = storage_.get_slot(dep_handle);
        if (!dep_slot) {
            if (!(dep.requirement & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL)) {
                slot.state = PULSE_ASSET_STATE_FAILED;
                slot.error = "required dependency handle is invalid";
            }
            continue;
        }
        if (dep_slot->slot.state == PULSE_ASSET_STATE_FAILED && !(dep.requirement & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL)) {
            slot.state = PULSE_ASSET_STATE_FAILED;
            slot.error = "dependency asset failed to load";
            continue;
        }
        if (dep_slot->slot.state != PULSE_ASSET_STATE_LOADED && !(dep.requirement & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL)) {
            has_unresolved_required = true;
        }
    }

    return has_unresolved_required ? LoadJobPhase::WaitingDependencies : LoadJobPhase::PendingRead;
}

bool AssetSystem::copy_request_settings(const AssetLoader& loader, const LoadRequest& request, PooledBlock& out) {
    if (loader.desc.settings_size == 0 || !request.settings) {
        return true;
    }

    if (loader.desc.settings_size_fn && loader.desc.settings_copy_fn) {
        // Deep-copy settings: query the total size (struct + all nested data), then
        // allocate one block in the asset pool. The copy callback fills the nested
        // data inside the block and fixes nested pointers to point into it, so the
        // whole block is released together by PooledBlock::reset().
        uint64_t total = loader.desc.settings_size_fn(request.settings, loader.desc.user_data);
        if (total < loader.desc.settings_size || total > UINT32_MAX) {
            return false;
        }
        if (!out.allocate(resource(), static_cast<uint32_t>(total), loader.desc.settings_align, false)) {
            return false;
        }
        std::memcpy(out.data, request.settings, loader.desc.settings_size);
        if (!loader.desc.settings_copy_fn(out.data, request.settings, total, loader.desc.user_data)) {
            out.reset();
            return false;
        }
        return true;
    }

    // Default path: byte-copy of the settings struct only. Nested pointers (if any)
    // keep pointing at caller memory, which must stay alive until the job retires.
    return out.copy(resource(), request.settings, loader.desc.settings_size, loader.desc.settings_align);
}

bool AssetSystem::init_load_job(LoadJob& job, const LoadRequest& request, PulseAssetHandle handle, AssetLoader* loader, LoadJobPhase phase) {
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

bool AssetSystem::is_load_in_progress_state(EPulseAssetState state) {
    return state == PULSE_ASSET_STATE_WAITING_LOAD ||
        state == PULSE_ASSET_STATE_LOADING ||
        state == PULSE_ASSET_STATE_WAITING_DEPENDENCIES ||
        state == PULSE_ASSET_STATE_PROCESSING;
}

} // namespace pulse::asset
