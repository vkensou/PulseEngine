#include "asset_internal.h"

#include <cstdlib>
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

static void* allocate_asset_memory(const pulse_asset_type_desc& desc) {
    (void)desc;
    return std::malloc(desc.size);
}

static void free_asset_memory(const pulse_asset_type_desc& desc, void* ptr) {
    (void)desc;
    std::free(ptr);
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
        bucket->free_indices.pop_back();
        reusing = true;
    }
    else {
        index = static_cast<uint32_t>(bucket->slots.size());
        bucket->slots.push_back(AssetSlot{});
    }

    AssetSlot& slot = bucket->slots[index];
    if (reusing) {
        slot.generation += 1;
        if (slot.generation == 0) {
            slot.generation = 1;
        }
    }
    if (!slot.data) {
        slot.data = allocate_asset_memory(bucket->type->desc);
    }
    slot.state = PULSE_ASSET_STATE_WAITING_LOAD;
    slot.pin_count = 0;
    slot.path = path;
    slot.error.clear();
    slot.loader_state = nullptr;
    slot.loader = nullptr;
    slot.version = 0;
    slot.constructed = false;
    slot.pending_load_bytes.clear();
    slot.pending_load_settings.clear();
    slot.pending_from_memory = false;

    return {type_id, index, slot.generation};
}

void destroy_slot(AssetBucket& bucket, AssetSlot& slot) {
    if (slot.constructed && bucket.type && bucket.type->desc.destroy) {
        bucket.type->desc.destroy(slot.data, bucket.type->desc.user_data);
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
    slot.loader_state = nullptr;
    slot.loader = nullptr;
    slot.version = 0;
    slot.dependencies.clear();
    slot.dependents.clear();
    slot.unresolved_count = 0;
    slot.pending_load_bytes.clear();
    slot.pending_load_settings.clear();
    slot.pending_from_memory = false;
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
    if (slot.pin_count == 0) {
        destroy_slot(bucket, slot);
        bucket.free_indices.push_back(handle.index);
    }
}

void destroy_all_assets(pulse_asset_state_o* state) {
    if (!state) {
        return;
    }
    for (auto& bucket_pair : state->buckets) {
        AssetBucket& bucket = bucket_pair.second;
        for (AssetSlot& slot : bucket.slots) {
            destroy_slot(bucket, slot);
            if (slot.data && bucket.type) {
                free_asset_memory(bucket.type->desc, slot.data);
                slot.data = nullptr;
            }
        }
    }
    state->buckets.clear();
    state->path_cache.clear();
}

} // namespace pulse_asset_internal
