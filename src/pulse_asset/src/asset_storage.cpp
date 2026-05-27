#include "asset_internal.h"

namespace pulse_asset_internal {

pulse_asset_handle invalid_handle(void) {
    return {0, PULSE_ASSET_INVALID_INDEX, 0};
}

bool is_invalid_handle(pulse_asset_handle handle) {
    return handle.type_id == 0 || handle.index == PULSE_ASSET_INVALID_INDEX;
}

std::pmr::string normalize_path(const char* path, std::pmr::memory_resource* resource) {
    std::pmr::string out(path ? path : "", resource);
    for (char& c : out) {
        if (c == '\\') {
            c = '/';
        }
    }
    size_t first_relative_char = out.find_first_not_of('/');
    if (first_relative_char == std::pmr::string::npos) {
        out.clear();
    } else if (first_relative_char > 0) {
        out.erase(0, first_relative_char);
    }
    return out;
}

static bool handles_equal(pulse_asset_handle a, pulse_asset_handle b) {
    return a.type_id == b.type_id && a.index == b.index && a.generation == b.generation;
}

static void erase_path_cache_entry(
    pulse_asset_state_o* state,
    pulse_asset_handle handle,
    const std::pmr::string& path
) {
    if (!state || path.empty()) {
        return;
    }
    PathKey key(handle.type_id, path, &state->memory_pool);
    auto cache_it = state->path_cache.find(key);
    if (cache_it != state->path_cache.end() && handles_equal(cache_it->second, handle)) {
        state->path_cache.erase(cache_it);
    }
}

static bool allocate_asset_memory(pulse_asset_state_o* state, const pulse_asset_type_desc& desc, PooledBlock& out) {
    return allocate_pooled_block(state, out, desc.size, desc.align, false);
}

static void free_asset_memory(PooledBlock& block) {
    free_pooled_block(block);
}

AssetBucket* ensure_bucket(pulse_asset_state_o* state, uint64_t type_id) {
    auto type_it = state->types.find(type_id);
    if (type_it == state->types.end()) {
        return nullptr;
    }
    auto bucket_result = state->buckets.try_emplace(type_id, &state->memory_pool);
    AssetBucket& bucket = bucket_result.first->second;
    if (!bucket.type) {
        bucket.type = &type_it->second;
    }
    return &bucket;
}

AssetSlot* get_slot(pulse_asset_state_o* state, pulse_asset_handle handle) {
    if (!state || is_invalid_handle(handle)) {
        return nullptr;
    }
    auto bucket_it = state->buckets.find(handle.type_id);
    if (bucket_it == state->buckets.end()) {
        return nullptr;
    }
    AssetBucket& bucket = bucket_it->second;
    if (handle.index >= bucket.slots.size()) {
        return nullptr;
    }
    AssetSlot& slot = bucket.slots[handle.index];
    if (slot.generation != handle.generation) {
        return nullptr;
    }
    return &slot;
}

const AssetSlot* get_slot_const(const pulse_asset_state_o* state, pulse_asset_handle handle) {
    return get_slot(const_cast<pulse_asset_state_o*>(state), handle);
}

pulse_asset_handle allocate_slot(
    pulse_asset_state_o* state,
    uint64_t type_id,
    const std::pmr::string& path
) {
    AssetBucket* bucket = ensure_bucket(state, type_id);
    if (!bucket) {
        return invalid_handle();
    }

    uint32_t index = 0;
    bool reusing = false;
    if (!bucket->free_indices.empty()) {
        index = bucket->free_indices.back();
        reusing = true;

        AssetSlot& slot = bucket->slots[index];
        if (!slot.data.data) {
            PooledBlock data;
            if (!allocate_asset_memory(state, bucket->type->desc, data)) {
                return invalid_handle();
            }
            slot.data = std::move(data);
        }
        bucket->free_indices.pop_back();
    }
    else {
        PooledBlock data;
        if (!allocate_asset_memory(state, bucket->type->desc, data)) {
            return invalid_handle();
        }
        index = static_cast<uint32_t>(bucket->slots.size());
        bucket->slots.emplace_back(&state->memory_pool);
        bucket->slots[index].data = std::move(data);
    }

    AssetSlot& slot = bucket->slots[index];
    if (reusing) {
        slot.generation += 1;
        if (slot.generation == 0) {
            slot.generation = 1;
        }
    }
    slot.state = PULSE_ASSET_STATE_WAITING_LOAD;
    slot.pin_count = 0;
    slot.path = path;
    slot.error.clear();
    slot.version = 0;
    slot.constructed = false;
    slot.retiring_load_job = false;
    slot.dependencies.clear();
    slot.dependents.clear();

    return {type_id, index, slot.generation};
}

void destroy_slot(pulse_asset_state_o* state, AssetBucket& bucket, AssetSlot& slot, pulse_asset_handle handle) {
    erase_path_cache_entry(state, handle, slot.path);
    if (slot.constructed && bucket.type && bucket.type->desc.destroy) {
        bucket.type->desc.destroy(slot.data.data, bucket.type->desc.user_data);
    }
    if (state) {
        for (pulse_asset_handle dep_handle : slot.dependencies) {
            AssetSlot* dep_slot = get_slot(state, dep_handle);
            if (!dep_slot) {
                continue;
            }
            dep_slot->dependents.erase(
                std::remove_if(dep_slot->dependents.begin(), dep_slot->dependents.end(), [&](pulse_asset_handle dependent) {
                    return handles_equal(dependent, handle);
                }),
                dep_slot->dependents.end());
            try_unload_slot(state, dep_handle);
        }
    }
    slot.constructed = false;
    slot.state = PULSE_ASSET_STATE_EMPTY;
    slot.generation += 1;
    if (slot.generation == 0) {
        slot.generation = 1;
    }
    slot.pin_count = 0;
    slot.path.clear();
    slot.error.clear();
    slot.version = 0;
    slot.retiring_load_job = false;
    slot.dependencies.clear();
    slot.dependents.clear();
}

void try_unload_slot(pulse_asset_state_o* state, pulse_asset_handle handle) {
    if (!state || is_invalid_handle(handle)) {
        return;
    }
    auto bucket_it = state->buckets.find(handle.type_id);
    if (bucket_it == state->buckets.end()) {
        return;
    }
    AssetBucket& bucket = bucket_it->second;
    if (handle.index >= bucket.slots.size()) {
        return;
    }
    AssetSlot& slot = bucket.slots[handle.index];
    if (slot.generation != handle.generation) {
        return;
    }
    if (slot.pin_count == 0 && slot.dependents.empty()) {
        destroy_slot(state, bucket, slot, handle);
        bucket.free_indices.push_back(handle.index);
    }
}

void destroy_all_assets(pulse_asset_state_o* state) {
    if (!state) {
        return;
    }
    for (auto& bucket_pair : state->buckets) {
        AssetBucket& bucket = bucket_pair.second;
        for (size_t slot_idx = 0; slot_idx < bucket.slots.size(); ++slot_idx) {
            AssetSlot& slot = bucket.slots[slot_idx];
            pulse_asset_handle handle{bucket_pair.first, static_cast<uint32_t>(slot_idx), slot.generation};
            destroy_slot(state, bucket, slot, handle);
            if (slot.data.data && bucket.type) {
                free_asset_memory(slot.data);
            }
        }
    }
    state->buckets.clear();
    state->path_cache.clear();
}

} // namespace pulse_asset_internal
