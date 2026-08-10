#include "asset_internal.h"

namespace pulse::asset {

LoadSource::LoadSource(std::pmr::memory_resource* resource)
    : memory_data(resource) {
}

LoadJob::LoadJob(std::pmr::memory_resource* resource)
    : source(resource),
      bytes(resource),
      dependencies(resource) {
}

bool LoadJob::is_terminal() const {
    return outcome != LoadJobOutcome::None;
}

void LoadJob::finish(AssetSlot* slot, LoadJobOutcome next_outcome, const char* error) {
    if (is_terminal() || next_outcome == LoadJobOutcome::None) {
        return;
    }

    outcome = next_outcome;
    if (!slot) {
        return;
    }

    switch (next_outcome) {
    case LoadJobOutcome::Loaded:
        slot->constructed = true;
        slot->state = PULSE_ASSET_STATE_LOADED;
        slot->error.clear();
        break;
    case LoadJobOutcome::Failed:
        slot->state = PULSE_ASSET_STATE_FAILED;
        slot->error = error ? error : "asset load failed";
        break;
    case LoadJobOutcome::Cancelled:
        slot->state = PULSE_ASSET_STATE_FAILED;
        slot->error = error ? error : "asset load cancelled";
        break;
    case LoadJobOutcome::None:
        break;
    }
}

EPulseResult LoadJob::add_dependency(PulseAssetHandle dependency, EPulseLoadDependencyRequirement flags) {
    PulseAssetDepRef dep_ref = handle_to_dep_ref(dependency);
    auto existing_it = std::find_if(dependencies.begin(), dependencies.end(), [&](const PulseAssetDependency& existing) {
        PulseAssetHandle existing_handle = dep_ref_to_handle(existing.dep_ref);
        return handles_equal(existing_handle, dependency);
    });
    if (existing_it != dependencies.end()) {
        if (!(flags & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL)) {
            existing_it->requirement = PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED;
        }
    } else {
        dependencies.push_back({dep_ref, flags});
    }

    ctx.dependencies = dependencies.data();
    ctx.dependency_count = static_cast<uint32_t>(dependencies.size());
    return PULSE_RESULT_OK;
}

void LoadContext::refresh(AssetSystem& system, LoadJob& job, AssetSlot& slot) {
    job.ctx.app = system.app();
    job.ctx.asset_system = (PulseAssetSystemId)&system;
    job.ctx.type_id = job.handle.type_id;
    job.ctx.path = slot.path.c_str();
    job.ctx.bytes = job.bytes.empty() ? nullptr : job.bytes.data();
    job.ctx.byte_size = static_cast<uint64_t>(job.bytes.size());
    job.ctx.dependencies = job.dependencies.empty() ? nullptr : job.dependencies.data();
    job.ctx.dependency_count = static_cast<uint32_t>(job.dependencies.size());
    job.ctx.request = handle_to_request(job.handle);
    job.ctx.user_data = job.loader ? job.loader->desc.user_data : nullptr;
    job.ctx.out_asset = slot.data.data;
    job.ctx.settings = job.settings.data;
    job.ctx.dependency_hint = nullptr;
    job.ctx.source = job.source.kind;
}

LoadQueue::LoadQueue(std::pmr::memory_resource* resource)
    : jobs_(resource) {
}

LoadQueue::JobIterator LoadQueue::enqueue(LoadJob&& job) {
    jobs_.push_back(std::move(job));
    auto job_it = jobs_.end();
    --job_it;
    return job_it;
}

void LoadQueue::process(AssetSystem& system) {
    size_t jobs_to_process = jobs_.size();
    if (system.max_requests_per_update() > 0 && jobs_to_process > system.max_requests_per_update()) {
        jobs_to_process = system.max_requests_per_update();
    }

    auto job_it = jobs_.begin();
    while (jobs_to_process-- > 0 && job_it != jobs_.end()) {
        LoadJob& job = *job_it;
        process_job(system, job);

        if (job.is_terminal()) {
            auto current = job_it++;
            retire_and_erase(system, current);
            continue;
        }

        if (job.phase == LoadJobPhase::WaitingDependencies) {
            auto current = job_it++;
            jobs_.splice(jobs_.end(), jobs_, current);
            continue;
        }

        ++job_it;
    }
}

LoadJobOutcome LoadQueue::process_immediate_builder(AssetSystem& system, JobIterator job_it) {
    process_job(system, *job_it);
    LoadJobOutcome outcome = job_it->outcome;
    if (job_it->is_terminal()) {
        retire_and_erase(system, job_it);
    }
    return outcome;
}

void LoadQueue::cancel_all(AssetSystem& system) {
    for (LoadJob& job : jobs_) {
        auto slot = system.storage().get_slot(job.handle);
        job.finish(slot ? &slot->slot : nullptr, LoadJobOutcome::Cancelled, "asset load cancelled");
    }
    for (LoadJob& job : jobs_) {
        retire_load_job(system, job);
    }
    jobs_.clear();
}

void LoadQueue::cancel_type(AssetSystem& system, uint64_t type_id) {
    auto job_it = jobs_.begin();
    while (job_it != jobs_.end()) {
        if (job_it->handle.type_id != type_id) {
            ++job_it;
            continue;
        }

        PulseAssetHandle handle = job_it->handle;
        auto slot = system.storage().get_slot(handle);
        job_it->finish(slot ? &slot->slot : nullptr, LoadJobOutcome::Cancelled, "asset load cancelled");

        auto retired_slot = retire_load_job(system, *job_it);
        job_it = jobs_.erase(job_it);
        if (retired_slot) {
            system.storage().try_unload_slot(*retired_slot, handle);
        }
    }
}

std::optional<AssetBucketSlot> LoadQueue::retire_load_job(AssetSystem& system, LoadJob& job) {
    auto slot = system.storage().get_slot(job.handle);
    if (job.loader_constructed && job.loader && job.loader->desc.dtor) {
        if (slot) {
            LoadContext::refresh(system, job, slot->slot);
            slot->slot.retiring_load_job = true;
        }
        job.loader->desc.dtor(job.loader_state.data, &job.ctx);
        if (slot) {
            slot->slot.retiring_load_job = false;
        }
    }

    job.loader_state.reset();
    job.settings.reset();
    job.loader = nullptr;
    job.loader_constructed = false;
    job.bytes.clear();
    job.source.memory_data.clear();
    return slot;
}

void LoadQueue::retire_and_erase(AssetSystem& system, JobIterator job_it) {
    PulseAssetHandle handle = job_it->handle;
    auto slot = retire_load_job(system, *job_it);
    jobs_.erase(job_it);
    if (slot) {
        system.storage().try_unload_slot(*slot, handle);
    }
}

void LoadQueue::process_job(AssetSystem& system, LoadJob& job) {
    if (job.is_terminal()) {
        return;
    }

    auto slot = system.storage().get_slot(job.handle);
    if (!slot) {
        job.finish(nullptr, LoadJobOutcome::Failed, nullptr);
        return;
    }
    if (slot->slot.state == PULSE_ASSET_STATE_PENDING_DELETE) {
        job.finish(&slot->slot, LoadJobOutcome::Cancelled, "asset load cancelled");
        return;
    }

    switch (job.phase) {
    case LoadJobPhase::PendingRead:
        process_pending_read(system, job, slot->slot, true);
        break;
    case LoadJobPhase::WaitingDependencies:
        process_waiting_dependencies(system, job, slot->slot);
        break;
    case LoadJobPhase::Processing:
        process_processing(system, job, slot->slot);
        break;
    }
}

void LoadQueue::process_pending_read(AssetSystem& system, LoadJob& job, AssetSlot& slot, bool process_immediately) {
    bool deps_failed = false;
    bool deps_ready = true;
    system.storage().dependencies().evaluate(system.storage(), job.dependencies, deps_failed, deps_ready);
    if (deps_failed) {
        job.finish(&slot, LoadJobOutcome::Failed, "dependency asset failed to load");
        return;
    }
    if (!deps_ready) {
        slot.state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
        job.phase = LoadJobPhase::WaitingDependencies;
        return;
    }

    slot.state = PULSE_ASSET_STATE_LOADING;
    if (job.source.kind == PULSE_ASSET_LOAD_SOURCE_MEMORY) {
        job.bytes = std::move(job.source.memory_data);
    } else if (job.source.kind == PULSE_ASSET_LOAD_SOURCE_FILE) {
        std::pmr::string full_path = AssetIo::join_path(system.root_path(), slot.path, system.resource());
        auto file_bytes = AssetIo::read_file(full_path.c_str(), system.resource());
        if (!file_bytes.has_value()) {
            job.finish(&slot, LoadJobOutcome::Failed, "failed to read asset file");
            return;
        }
        job.bytes = std::move(file_bytes.value());
    } else {
        job.bytes.clear();
    }

    const char* error = nullptr;
    if (!construct_job_loader(system, job, slot, error)) {
        if (!job.is_terminal()) {
            job.finish(&slot, LoadJobOutcome::Failed, error);
        }
        return;
    }

    slot.state = PULSE_ASSET_STATE_PROCESSING;
    job.phase = LoadJobPhase::Processing;
    if (process_immediately) {
        process_processing(system, job, slot);
    }
}

void LoadQueue::process_waiting_dependencies(AssetSystem& system, LoadJob& job, AssetSlot& slot) {
    bool deps_failed = false;
    bool deps_ready = true;
    system.storage().dependencies().evaluate(system.storage(), job.dependencies, deps_failed, deps_ready);
    if (deps_failed) {
        job.finish(&slot, LoadJobOutcome::Failed, "dependency asset failed to load");
        return;
    }
    if (!deps_ready) {
        slot.state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
        return;
    }

    if (!job.loader_constructed) {
        job.phase = LoadJobPhase::PendingRead;
        process_pending_read(system, job, slot, false);
        return;
    }

    slot.state = PULSE_ASSET_STATE_PROCESSING;
    job.phase = LoadJobPhase::Processing;
    process_processing(system, job, slot);
}

void LoadQueue::process_processing(AssetSystem& system, LoadJob& job, AssetSlot& slot) {
    LoadContext::refresh(system, job, slot);
    PulseAssetLoadDependencyHint dependency_hint{&job};
    job.ctx.dependency_hint = &dependency_hint;

    const char* error = nullptr;
    EPulseAssetLoaderStatus status = job.loader->desc.step(job.loader_state.data, &job.ctx, &error);
    job.ctx.dependency_hint = nullptr;

    if (slot.state == PULSE_ASSET_STATE_PENDING_DELETE) {
        job.finish(&slot, LoadJobOutcome::Cancelled, "asset load cancelled");
        return;
    }
    if (job.is_terminal()) {
        return;
    }

    if (status == PULSE_ASSET_LOADER_STATUS_PENDING) {
        return;
    }
    if (status == PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES) {
        slot.state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
        job.phase = LoadJobPhase::WaitingDependencies;
        return;
    }
    if (status == PULSE_ASSET_LOADER_STATUS_DONE) {
        bool deps_failed = false;
        bool deps_ready = true;
        system.storage().dependencies().evaluate(system.storage(), job.dependencies, deps_failed, deps_ready);
        if (deps_failed) {
            job.finish(&slot, LoadJobOutcome::Failed, "dependency asset failed to load");
            return;
        }
        if (!deps_ready) {
            slot.state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
            job.phase = LoadJobPhase::WaitingDependencies;
            return;
        }

        system.storage().dependencies().commit(system.storage(), job.handle, job.dependencies);
        job.finish(&slot, LoadJobOutcome::Loaded, nullptr);
        return;
    }

    job.finish(&slot, LoadJobOutcome::Failed, error ? error : "asset loader step failed");
}

bool LoadQueue::construct_job_loader(AssetSystem& system, LoadJob& job, AssetSlot& slot, const char*& out_error) {
    if (!job.loader && !slot.path.empty()) {
        job.loader = system.registry().find_loader(job.handle.type_id, slot.path);
    }
    if (!job.loader) {
        out_error = "no loader registered for asset request";
        return false;
    }
    if (!job.loader_state.allocate(system.resource(), job.loader->desc.loader_size, job.loader->desc.loader_align, true)) {
        out_error = "failed to allocate asset loader state";
        return false;
    }

    LoadContext::refresh(system, job, slot);
    if (job.loader->desc.ctor) {
        EPulseResult ctor_result = job.loader->desc.ctor(job.loader_state.data, &job.ctx);
        if (ctor_result != PULSE_RESULT_OK) {
            job.loader_state.reset();
            out_error = "asset loader begin failed";
            return false;
        }
        job.loader_constructed = true;
        if (job.is_terminal()) {
            out_error = "asset load cancelled";
            return false;
        }
    } else {
        job.loader_constructed = true;
    }
    return true;
}

} // namespace pulse::asset
