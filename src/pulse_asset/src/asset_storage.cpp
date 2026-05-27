#include "asset_internal.h"

#include <algorithm>

namespace pulse::asset {

PathKey::PathKey(uint64_t key_type_id, const std::pmr::string& key_path, std::pmr::memory_resource* resource)
    : type_id(key_type_id),
      path(key_path, resource) {
}

bool PathKey::operator==(const PathKey& other) const {
    return type_id == other.type_id && path == other.path;
}

size_t PathKeyHash::operator()(const PathKey& key) const {
    return std::hash<uint64_t>{}(key.type_id) ^
        (std::hash<std::string_view>{}(std::string_view(key.path)) << 1);
}

PathCache::PathCache(std::pmr::memory_resource* resource)
    : resource_(resource),
      entries_(resource) {
}

pulse_asset_handle PathCache::find(uint64_t type_id, const std::pmr::string& path) const {
    PathKey key(type_id, path, resource_);
    auto cache_it = entries_.find(key);
    return cache_it != entries_.end() ? cache_it->second : invalid_handle();
}

void PathCache::store(uint64_t type_id, const std::pmr::string& path, pulse_asset_handle handle) {
    PathKey key(type_id, path, resource_);
    entries_.insert_or_assign(std::move(key), handle);
}

void PathCache::erase_if_matches(pulse_asset_handle handle, const std::pmr::string& path) {
    if (path.empty()) {
        return;
    }

    PathKey key(handle.type_id, path, resource_);
    auto cache_it = entries_.find(key);
    if (cache_it != entries_.end() && handles_equal(cache_it->second, handle)) {
        entries_.erase(cache_it);
    }
}

void PathCache::clear() {
    entries_.clear();
}

AssetSlot::AssetSlot(std::pmr::memory_resource* resource)
    : path(resource),
      error(resource),
      dependencies(resource),
      dependents(resource) {
}

AssetBucket::AssetBucket(std::pmr::memory_resource* resource)
    : slots(resource),
      free_indices(resource) {
    slots.emplace_back(resource);
}

void DependencyGraph::evaluate(
    const AssetStorage& storage,
    const std::pmr::vector<pulse_asset_dependency>& dependencies,
    bool& out_failed,
    bool& out_ready
) const {
    out_failed = false;
    out_ready = true;

    for (const pulse_asset_dependency& dep : dependencies) {
        if (is_invalid_handle(dep.handle)) {
            if (dep.flags & PULSE_DEP_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return;
        }

        const AssetSlot* dep_slot = storage.get_slot(dep.handle);
        if (!dep_slot) {
            if (dep.flags & PULSE_DEP_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return;
        }
        if (dep_slot->state == PULSE_ASSET_STATE_FAILED) {
            if (dep.flags & PULSE_DEP_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return;
        }
        if (dep_slot->state != PULSE_ASSET_STATE_LOADED && !(dep.flags & PULSE_DEP_OPTIONAL)) {
            out_ready = false;
        }
    }
}

void DependencyGraph::commit(
    AssetStorage& storage,
    pulse_asset_handle handle,
    const std::pmr::vector<pulse_asset_dependency>& dependencies
) const {
    AssetSlot* slot = storage.get_slot(handle);
    if (!slot) {
        return;
    }

    detach_committed_dependencies(storage, handle, *slot);
    slot->dependencies.clear();

    for (const pulse_asset_dependency& dep : dependencies) {
        if (is_invalid_handle(dep.handle)) {
            continue;
        }

        AssetSlot* dep_slot = storage.get_slot(dep.handle);
        if (!dep_slot) {
            continue;
        }

        bool exists = std::any_of(slot->dependencies.begin(), slot->dependencies.end(), [&](pulse_asset_handle existing) {
            return handles_equal(existing, dep.handle);
        });
        if (exists) {
            continue;
        }

        slot->dependencies.push_back(dep.handle);
        dep_slot->dependents.push_back(handle);
    }
}

void DependencyGraph::detach_committed_dependencies(
    AssetStorage& storage,
    pulse_asset_handle handle,
    AssetSlot& slot
) const {
    for (pulse_asset_handle dep_handle : slot.dependencies) {
        AssetSlot* dep_slot = storage.get_slot(dep_handle);
        if (!dep_slot) {
            continue;
        }

        dep_slot->dependents.erase(
            std::remove_if(dep_slot->dependents.begin(), dep_slot->dependents.end(), [&](pulse_asset_handle dependent) {
                return handles_equal(dependent, handle);
            }),
            dep_slot->dependents.end());
        storage.try_unload_slot(dep_handle);
    }
}

void DependencyGraph::pin_committed_dependencies(AssetStorage& storage, const AssetSlot& slot) const {
    for (pulse_asset_handle dep_handle : slot.dependencies) {
        if (is_invalid_handle(dep_handle)) {
            continue;
        }

        AssetSlot* dep_slot = storage.get_slot(dep_handle);
        if (dep_slot && dep_slot->state == PULSE_ASSET_STATE_LOADED) {
            dep_slot->pin_count += 1;
        }
    }
}

void DependencyGraph::unpin_committed_dependencies(AssetStorage& storage, const AssetSlot& slot) const {
    for (pulse_asset_handle dep_handle : slot.dependencies) {
        if (is_invalid_handle(dep_handle)) {
            continue;
        }

        AssetSlot* dep_slot = storage.get_slot(dep_handle);
        if (dep_slot && dep_slot->pin_count > 0) {
            dep_slot->pin_count -= 1;
        }
    }
}

AssetStorage::AssetStorage(std::pmr::memory_resource* resource, AssetRegistry& registry)
    : resource_(resource),
      registry_(registry),
      path_cache_(resource),
      buckets_(resource) {
}

AssetBucket* AssetStorage::ensure_bucket(uint64_t type_id) {
    AssetType* type = registry_.find_type(type_id);
    if (!type) {
        return nullptr;
    }

    auto bucket_result = buckets_.try_emplace(type_id, resource_);
    AssetBucket& bucket = bucket_result.first->second;
    if (!bucket.type) {
        bucket.type = type;
    }
    return &bucket;
}

AssetSlot* AssetStorage::get_slot(pulse_asset_handle handle) {
    if (is_invalid_handle(handle)) {
        return nullptr;
    }

    auto bucket_it = buckets_.find(handle.type_id);
    if (bucket_it == buckets_.end()) {
        return nullptr;
    }

    AssetBucket& bucket = bucket_it->second;
    if (handle.index >= bucket.slots.size()) {
        return nullptr;
    }

    AssetSlot& slot = bucket.slots[handle.index];
    return slot.generation == handle.generation ? &slot : nullptr;
}

const AssetSlot* AssetStorage::get_slot(pulse_asset_handle handle) const {
    if (is_invalid_handle(handle)) {
        return nullptr;
    }

    auto bucket_it = buckets_.find(handle.type_id);
    if (bucket_it == buckets_.end()) {
        return nullptr;
    }

    const AssetBucket& bucket = bucket_it->second;
    if (handle.index >= bucket.slots.size()) {
        return nullptr;
    }

    const AssetSlot& slot = bucket.slots[handle.index];
    return slot.generation == handle.generation ? &slot : nullptr;
}

AssetSlotAllocation AssetStorage::allocate_slot(uint64_t type_id, const std::pmr::string& path) {
    AssetBucket* bucket = ensure_bucket(type_id);
    if (!bucket) {
        return {invalid_handle(), nullptr};
    }

    uint32_t index = 0;
    if (!bucket->free_indices.empty()) {
        index = bucket->free_indices.back();

        AssetSlot& slot = bucket->slots[index];
        if (!slot.data.data && !slot.data.allocate(resource_, bucket->type->desc.size, bucket->type->desc.align, false)) {
            return {invalid_handle(), nullptr};
        }
        bucket->free_indices.pop_back();
    } else {
        PooledBlock data;
        if (!data.allocate(resource_, bucket->type->desc.size, bucket->type->desc.align, false)) {
            return {invalid_handle(), nullptr};
        }
        index = static_cast<uint32_t>(bucket->slots.size());
        bucket->slots.emplace_back(resource_);
        bucket->slots[index].data = std::move(data);
    }

    AssetSlot& slot = bucket->slots[index];

    slot.state = PULSE_ASSET_STATE_WAITING_LOAD;
    slot.pin_count = 0;
    slot.path = path;
    slot.error.clear();
    slot.version = 0;
    slot.constructed = false;
    slot.retiring_load_job = false;
    slot.dependencies.clear();
    slot.dependents.clear();

    return {{type_id, index, slot.generation}, &slot};
}

void AssetStorage::destroy_slot(AssetBucket& bucket, AssetSlot& slot, pulse_asset_handle handle) {
    path_cache_.erase_if_matches(handle, slot.path);

    if (slot.constructed && bucket.type && bucket.type->desc.destroy) {
        bucket.type->desc.destroy(slot.data.data, bucket.type->desc.user_data);
    }

    dependency_graph_.detach_committed_dependencies(*this, handle, slot);

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

void AssetStorage::destroy_all_assets() {
    for (auto& bucket_pair : buckets_) {
        AssetBucket& bucket = bucket_pair.second;
        for (size_t slot_idx = 0; slot_idx < bucket.slots.size(); ++slot_idx) {
            AssetSlot& slot = bucket.slots[slot_idx];
            pulse_asset_handle handle{bucket_pair.first, static_cast<uint32_t>(slot_idx), slot.generation};
            destroy_slot(bucket, slot, handle);
            slot.data.reset();
        }
    }
    buckets_.clear();
    path_cache_.clear();
}

void AssetStorage::try_unload_slot(pulse_asset_handle handle) {
    if (is_invalid_handle(handle)) {
        return;
    }

    auto bucket_it = buckets_.find(handle.type_id);
    if (bucket_it == buckets_.end()) {
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
        destroy_slot(bucket, slot, handle);
        bucket.free_indices.push_back(handle.index);
    }
}

bool AssetStorage::cached_slot_can_be_reused(pulse_asset_handle handle) const {
    const AssetSlot* slot = get_slot(handle);
    return slot && slot->state != PULSE_ASSET_STATE_PENDING_DELETE;
}

pulse_asset_handle AssetStorage::find_cached(uint64_t type_id, const std::pmr::string& path) const {
    return path_cache_.find(type_id, path);
}

void AssetStorage::cache_path(uint64_t type_id, const std::pmr::string& path, pulse_asset_handle handle) {
    path_cache_.store(type_id, path, handle);
}

} // namespace pulse::asset
