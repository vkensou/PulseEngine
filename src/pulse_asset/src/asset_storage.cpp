#include "asset_internal.h"

#include <new>

namespace pulse_asset_internal {

pulse_asset_handle invalid_handle(void) {
    return {0, PULSE_ASSET_INVALID_INDEX, 0};
}

bool is_invalid_handle(pulse_asset_handle handle) {
    return handle.type_id == 0 || handle.index == PULSE_ASSET_INVALID_INDEX;
}

std::string normalize_path(const char* path) {
    std::string out = path ? path : "";
    for (char& c : out) {
        if (c == '\\') {
            c = '/';
        }
    }
    while (!out.empty() && out.front() == '/') {
        out.erase(out.begin());
    }
    return out;
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
    AssetBucket& bucket = state->buckets[type_id];
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
    const std::string& path
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
        bucket->slots.push_back(AssetSlot{});
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
    slot.dependencies.clear();
    slot.dependents.clear();

    return {type_id, index, slot.generation};
}

void destroy_slot(pulse_asset_state_o* state, AssetBucket& bucket, AssetSlot& slot, pulse_asset_handle handle) {
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
                    return dependent.type_id == handle.type_id &&
                        dependent.index == handle.index &&
                        dependent.generation == handle.generation;
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
