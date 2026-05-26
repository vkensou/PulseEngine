#include "asset_internal.h"

#include <SDL3/SDL.h>

#include <cstring>

namespace pulse_asset_internal {

static std::string extension_from_path(const std::string& path) {
    size_t slash = path.find_last_of("/");
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return "";
    }
    return normalize_extension(path.c_str() + dot + 1);
}

AssetLoader* find_loader(pulse_asset_state_o* state, uint64_t type_id, const std::string& path) {
    std::string extension = extension_from_path(path);
    if (extension.empty()) {
        return nullptr;
    }
    for (AssetLoader& loader : state->loaders) {
        if (loader.desc.type_id != type_id) {
            continue;
        }
        for (const std::string& candidate : loader.extensions) {
            if (candidate == extension) {
                return &loader;
            }
        }
    }
    return nullptr;
}

std::string join_asset_path(const std::string& root_path, const std::string& path) {
    if (root_path.empty()) {
        return path;
    }
    if (root_path.back() == '/' || root_path.back() == '\\') {
        return root_path + path;
    }
    return root_path + "/" + path;
}

std::optional<std::vector<uint8_t>> read_file_sdl(const char* filename) {
    SDL_IOStream* stream = SDL_IOFromFile(filename, "rb");
    if (!stream) {
        return std::optional<std::vector<uint8_t>>{};
    }
    Sint64 size = SDL_GetIOSize(stream);
    if (size < 0) {
        SDL_CloseIO(stream);
        return std::optional<std::vector<uint8_t>>{};
    }
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    size_t read = SDL_ReadIO(stream, buffer.data(), buffer.size());
    SDL_CloseIO(stream);
    if (read != buffer.size()) {
        return std::optional<std::vector<uint8_t>>{};
    }
    return buffer;
}

void free_pooled_block(PooledBlock& block) {
    if (block.data && block.resource) {
        block.resource->deallocate(block.data, block.size, block.align);
    }
    block.data = nullptr;
    block.size = 0;
    block.align = 0;
    block.resource = nullptr;
}

bool allocate_pooled_block(pulse_asset_state_o* state, PooledBlock& out, uint32_t size, uint32_t align, bool zero_memory) {
    free_pooled_block(out);
    if (size == 0) {
        return true;
    }
    if (!state || align == 0) {
        return false;
    }
    try {
        out.data = state->memory_pool.allocate(size, align);
    } catch (...) {
        out.data = nullptr;
    }
    if (!out.data) {
        return false;
    }
    out.size = size;
    out.align = align;
    out.resource = &state->memory_pool;
    if (zero_memory) {
        memset(out.data, 0, size);
    }
    return true;
}

bool copy_pooled_block(pulse_asset_state_o* state, PooledBlock& out, const void* data, uint32_t size, uint32_t align) {
    if (!data || size == 0) {
        free_pooled_block(out);
        return true;
    }
    if (!allocate_pooled_block(state, out, size, align, false)) {
        return false;
    }
    memcpy(out.data, data, size);
    return true;
}

static bool dependency_is_invalid(pulse_asset_handle handle) {
    return handle.index == PULSE_ASSET_INVALID_INDEX || handle.type_id == 0;
}

static bool evaluate_dependencies(
    pulse_asset_state_o* state,
    const std::vector<pulse_asset_dependency>& dependencies,
    bool& out_failed,
    bool& out_ready
) {
    out_failed = false;
    out_ready = true;
    for (const pulse_asset_dependency& dep : dependencies) {
        if (dependency_is_invalid(dep.handle)) {
            if (dep.flags & PULSE_DEP_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return true;
        }
        const AssetSlot* dep_slot = get_slot_const(state, dep.handle);
        if (!dep_slot) {
            if (dep.flags & PULSE_DEP_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return true;
        }
        if (dep_slot->state == PULSE_ASSET_STATE_FAILED) {
            if (dep.flags & PULSE_DEP_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return true;
        }
        if (dep_slot->state != PULSE_ASSET_STATE_LOADED && !(dep.flags & PULSE_DEP_OPTIONAL)) {
            out_ready = false;
        }
    }
    return true;
}

static void refresh_job_ctx(pulse_asset_state_o* state, LoadJob& job, AssetSlot& slot) {
    job.ctx.app = state->app;
    job.ctx.type_id = job.handle.type_id;
    job.ctx.path = slot.path.c_str();
    job.ctx.bytes = job.bytes.data();
    job.ctx.byte_size = static_cast<uint64_t>(job.bytes.size());
    job.ctx.dependencies = job.dependencies.data();
    job.ctx.dependency_count = static_cast<uint32_t>(job.dependencies.size());
    job.ctx.handle = job.handle;
    job.ctx.user_data = job.loader ? job.loader->desc.user_data : nullptr;
    job.ctx.out_asset = slot.data.data;
    job.ctx.settings = job.settings.data;
    job.ctx.dependency_hint = nullptr;
}

static bool construct_job_loader(pulse_asset_state_o* state, LoadJob& job, AssetSlot& slot, const char*& out_error) {
    if (!job.loader) {
        job.loader = find_loader(state, job.handle.type_id, slot.path);
        if (!job.loader && slot.path.empty()) {
            for (AssetLoader& loader : state->loaders) {
                if (loader.desc.type_id == job.handle.type_id) {
                    job.loader = &loader;
                    break;
                }
            }
        }
    }
    if (!job.loader) {
        out_error = "no loader registered for asset extension";
        return false;
    }
    if (!allocate_pooled_block(state, job.loader_state, job.loader->desc.loader_size, job.loader->desc.loader_align, true)) {
        out_error = "failed to allocate asset loader state";
        return false;
    }
    refresh_job_ctx(state, job, slot);
    if (job.loader->desc.ctor) {
        job.in_callback = true;
        pulse_result_t ctor_result = job.loader->desc.ctor(job.loader_state.data, &job.ctx);
        job.in_callback = false;
        if (ctor_result != PULSE_OK) {
            free_pooled_block(job.loader_state);
            out_error = "asset loader begin failed";
            return false;
        }
        job.loader_constructed = true;
        if (job.finished) {
            out_error = "asset load cancelled";
            return false;
        }
    } else {
        job.loader_constructed = true;
    }
    return true;
}

void destroy_load_job(pulse_asset_state_o* state, LoadJob& job) {
    AssetSlot* slot = get_slot(state, job.handle);
    if (job.loader_constructed && job.loader && job.loader->desc.dtor) {
        if (slot) {
            refresh_job_ctx(state, job, *slot);
        }
        job.in_callback = true;
        job.loader->desc.dtor(job.loader_state.data, &job.ctx);
        job.in_callback = false;
    }
    free_pooled_block(job.loader_state);
    free_pooled_block(job.settings);
    job.loader = nullptr;
    job.loader_constructed = false;
    job.bytes.clear();
    job.source.memory_data.clear();
}

void commit_asset_dependencies(pulse_asset_state_o* state, pulse_asset_handle handle, const std::vector<pulse_asset_dependency>& dependencies) {
    AssetSlot* slot = get_slot(state, handle);
    if (!slot) {
        return;
    }
    for (pulse_asset_handle old_dep : slot->dependencies) {
        AssetSlot* dep_slot = get_slot(state, old_dep);
        if (!dep_slot) {
            continue;
        }
        dep_slot->dependents.erase(
            std::remove_if(dep_slot->dependents.begin(), dep_slot->dependents.end(), [&](pulse_asset_handle dependent) {
                return dependent.type_id == handle.type_id && dependent.index == handle.index && dependent.generation == handle.generation;
            }),
            dep_slot->dependents.end());
    }
    slot->dependencies.clear();
    for (const pulse_asset_dependency& dep : dependencies) {
        if (dep.handle.index == PULSE_ASSET_INVALID_INDEX) {
            continue;
        }
        AssetSlot* dep_slot = get_slot(state, dep.handle);
        if (!dep_slot) {
            continue;
        }
        bool exists = std::any_of(slot->dependencies.begin(), slot->dependencies.end(), [&](pulse_asset_handle existing) {
            return existing.type_id == dep.handle.type_id && existing.index == dep.handle.index && existing.generation == dep.handle.generation;
        });
        if (exists) {
            continue;
        }
        slot->dependencies.push_back(dep.handle);
        dep_slot->dependents.push_back(handle);
    }
}

static void fail_job(LoadJob& job, AssetSlot& slot, const char* error) {
    slot.state = PULSE_ASSET_STATE_FAILED;
    slot.error = error ? error : "asset load failed";
    job.finished = true;
}

static void process_processing(pulse_asset_state_o* state, LoadJob& job, AssetSlot& slot);

static void process_pending_read(pulse_asset_state_o* state, LoadJob& job, AssetSlot& slot, bool process_immediately) {
    bool deps_failed = false;
    bool deps_ready = true;
    evaluate_dependencies(state, job.dependencies, deps_failed, deps_ready);
    if (deps_failed) {
        fail_job(job, slot, "dependency asset failed to load");
        return;
    }
    if (!deps_ready) {
        slot.state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
        job.phase = LoadJobPhase::WaitingDependencies;
        return;
    }

    slot.state = PULSE_ASSET_STATE_LOADING;
    if (job.source.from_memory) {
        job.bytes = std::move(job.source.memory_data);
    } else {
        std::string full_path = join_asset_path(state->root_path, slot.path);
        auto file_bytes = read_file_sdl(full_path.c_str());
        if (!file_bytes.has_value()) {
            fail_job(job, slot, "failed to read asset file");
            return;
        }
        job.bytes = std::move(file_bytes.value());
    }

    const char* error = nullptr;
    if (!construct_job_loader(state, job, slot, error)) {
        fail_job(job, slot, error);
        return;
    }
    slot.state = PULSE_ASSET_STATE_PROCESSING;
    job.phase = LoadJobPhase::Processing;
    if (process_immediately) {
        process_processing(state, job, slot);
    }
}

static void process_waiting_dependencies(pulse_asset_state_o* state, LoadJob& job, AssetSlot& slot) {
    bool deps_failed = false;
    bool deps_ready = true;
    evaluate_dependencies(state, job.dependencies, deps_failed, deps_ready);
    if (deps_failed) {
        fail_job(job, slot, "dependency asset failed to load");
        return;
    }
    if (!deps_ready) {
        slot.state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
        return;
    }
    if (!job.loader_constructed) {
        job.phase = LoadJobPhase::PendingRead;
        process_pending_read(state, job, slot, false);
        return;
    }
    slot.state = PULSE_ASSET_STATE_PROCESSING;
    job.phase = LoadJobPhase::Processing;
    process_processing(state, job, slot);
}

static void process_processing(pulse_asset_state_o* state, LoadJob& job, AssetSlot& slot) {
    refresh_job_ctx(state, job, slot);
    pulse_asset_load_dependency_hint dependency_hint{&job};
    job.ctx.dependency_hint = &dependency_hint;
    const char* error = nullptr;
    job.in_callback = true;
    pulse_asset_loader_status_t status = job.loader->desc.step(job.loader_state.data, &job.ctx, &error);
    job.in_callback = false;
    job.ctx.dependency_hint = nullptr;
    if (job.finished) {
        return;
    }
    if (status == PULSE_ASSET_LOADER_PENDING) {
        return;
    }
    if (status == PULSE_ASSET_LOADER_WAIT_DEPENDENCIES) {
        slot.state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
        job.phase = LoadJobPhase::WaitingDependencies;
        return;
    }
    if (status == PULSE_ASSET_LOADER_DONE) {
        bool deps_failed = false;
        bool deps_ready = true;
        evaluate_dependencies(state, job.dependencies, deps_failed, deps_ready);
        if (deps_failed) {
            fail_job(job, slot, "dependency asset failed to load");
            return;
        }
        if (!deps_ready) {
            slot.state = PULSE_ASSET_STATE_WAITING_DEPENDENCIES;
            job.phase = LoadJobPhase::WaitingDependencies;
            return;
        }
        commit_asset_dependencies(state, job.handle, job.dependencies);
        slot.constructed = true;
        slot.state = PULSE_ASSET_STATE_LOADED;
        slot.error.clear();
        job.finished = true;
        return;
    }
    fail_job(job, slot, error ? error : "asset loader step failed");
}

static void process_load_job(pulse_asset_state_o* state, LoadJob& job) {
    if (job.finished) {
        return;
    }
    AssetSlot* slot = get_slot(state, job.handle);
    if (!slot) {
        job.finished = true;
        return;
    }
    switch (job.phase) {
    case LoadJobPhase::PendingRead:
        process_pending_read(state, job, *slot, true);
        break;
    case LoadJobPhase::WaitingDependencies:
        process_waiting_dependencies(state, job, *slot);
        break;
    case LoadJobPhase::Processing:
        process_processing(state, job, *slot);
        break;
    }
}

static bool has_load_job_for_handle(pulse_asset_state_o* state, pulse_asset_handle handle) {
    return std::any_of(state->load_jobs.begin(), state->load_jobs.end(), [&](const LoadJob& job) {
        return job.handle.type_id == handle.type_id &&
            job.handle.index == handle.index &&
            job.handle.generation == handle.generation;
    });
}

void process_load_requests_system(ecs_iter_t* it) {
    pulse_asset_state_o* state = static_cast<pulse_asset_state_o*>(it->ctx);
    if (!state) {
        return;
    }

    size_t jobs_to_process = state->load_jobs.size();
    if (state->desc.max_requests_per_update > 0 && jobs_to_process > state->desc.max_requests_per_update) {
        jobs_to_process = state->desc.max_requests_per_update;
    }
    auto job_it = state->load_jobs.begin();
    state->processing_load_jobs = true;
    while (jobs_to_process-- > 0 && job_it != state->load_jobs.end()) {
        LoadJob& job = *job_it;
        process_load_job(state, job);

        if (job.finished) {
            auto current = job_it++;
            destroy_load_job(state, *current);
            state->load_jobs.erase(current);
            continue;
        }

        if (job.phase == LoadJobPhase::WaitingDependencies) {
            auto current = job_it++;
            state->load_jobs.splice(state->load_jobs.end(), state->load_jobs, current);
            continue;
        }

        ++job_it;
    }
    state->processing_load_jobs = false;

    for (auto& bucket_pair : state->buckets) {
        AssetBucket& bucket = bucket_pair.second;
        for (size_t slot_idx = 0; slot_idx < bucket.slots.size(); ++slot_idx) {
            AssetSlot& slot = bucket.slots[slot_idx];
            pulse_asset_handle handle{bucket_pair.first, static_cast<uint32_t>(slot_idx), slot.generation};
            if (slot.pin_count == 0 && slot.state == PULSE_ASSET_STATE_FAILED && slot.data.data && bucket.type) {
                if (has_load_job_for_handle(state, handle)) {
                    continue;
                }
                destroy_slot(state, bucket, slot, handle);
                bucket.free_indices.push_back(static_cast<uint32_t>(slot_idx));
            }
        }
    }
}

void install_process_system(pulse_asset_state_o* state, ecs_world_t* world) {
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "PulseAssetProcessLoadRequests";

    ecs_system_desc_t system_desc{};
    system_desc.entity = ecs_entity_init(world, &entity_desc);
    system_desc.phase = EcsOnLoad;
    system_desc.callback = process_load_requests_system;
    system_desc.ctx = state;
    state->process_system = ecs_system_init(world, &system_desc);
}

void uninstall_process_system(pulse_asset_state_o* state, ecs_world_t* world) {
    if (state && world && state->process_system && ecs_is_alive(world, state->process_system)) {
        ecs_delete(world, state->process_system);
    }
    if (state) {
        state->process_system = 0;
    }
}

void cancel_load_jobs(pulse_asset_state_o* state) {
    if (!state) {
        return;
    }
    for (LoadJob& job : state->load_jobs) {
        AssetSlot* slot = get_slot(state, job.handle);
        if (slot) {
            slot->state = PULSE_ASSET_STATE_FAILED;
            slot->error = "asset load cancelled";
        }
        job.finished = true;
        destroy_load_job(state, job);
    }
    state->load_jobs.clear();
}

} // namespace pulse_asset_internal
