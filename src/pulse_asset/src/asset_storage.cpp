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

PulseAssetHandle PathCache::find(uint64_t type_id, const std::pmr::string& path) const {
    PathKey key(type_id, path, resource_);
    auto cache_it = entries_.find(key);
    return cache_it != entries_.end() ? cache_it->second : invalid_handle();
}

void PathCache::store(uint64_t type_id, const std::pmr::string& path, PulseAssetHandle handle) {
    PathKey key(type_id, path, resource_);
    entries_.insert_or_assign(std::move(key), handle);
}

void PathCache::erase_if_matches(PulseAssetHandle handle, const std::pmr::string& path) {
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
    const std::pmr::vector<PulseAssetDependency>& dependencies,
    bool& out_failed,
    bool& out_ready
) const {
    out_failed = false;
    out_ready = true;

    for (const PulseAssetDependency& dep : dependencies) {
        PulseAssetHandle dep_handle = dep_ref_to_handle(dep.dep_ref);
        if (is_invalid_handle(dep_handle)) {
            if (dep.requirement & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return;
        }

        auto dep_slot = storage.get_slot(dep_handle);
        if (!dep_slot) {
            if (dep.requirement & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return;
        }
        if (dep_slot->slot.state == PULSE_ASSET_STATE_FAILED) {
            if (dep.requirement & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL) {
                continue;
            }
            out_failed = true;
            return;
        }
        if (dep_slot->slot.state != PULSE_ASSET_STATE_LOADED && !(dep.requirement & PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL)) {
            out_ready = false;
        }
    }
}

void DependencyGraph::commit(
    AssetStorage& storage,
    PulseAssetHandle handle,
    const std::pmr::vector<PulseAssetDependency>& dependencies
) const {
    auto found = storage.get_slot(handle);
    if (!found) {
        return;
    }

    AssetSlot& slot = found->slot;
    detach_committed_dependencies(storage, handle, slot);
    slot.dependencies.clear();

    for (const PulseAssetDependency& dep : dependencies) {
        PulseAssetHandle dep_handle = dep_ref_to_handle(dep.dep_ref);
        if (is_invalid_handle(dep_handle)) {
            continue;
        }

        auto dep_slot = storage.get_slot(dep_handle);
        if (!dep_slot) {
            continue;
        }

        bool exists = std::any_of(slot.dependencies.begin(), slot.dependencies.end(), [&](PulseAssetHandle existing) {
            return handles_equal(existing, dep_handle);
        });
        if (exists) {
            continue;
        }

        slot.dependencies.push_back(dep_handle);
        dep_slot->slot.dependents.push_back(handle);
    }
}

void DependencyGraph::detach_committed_dependencies(
    AssetStorage& storage,
    PulseAssetHandle handle,
    AssetSlot& slot
) const {
    for (PulseAssetHandle dep_handle : slot.dependencies) {
        auto dep_slot = storage.get_slot(dep_handle);
        if (!dep_slot) {
            continue;
        }

        dep_slot->slot.dependents.erase(
            std::remove_if(dep_slot->slot.dependents.begin(), dep_slot->slot.dependents.end(), [&](PulseAssetHandle dependent) {
                return handles_equal(dependent, handle);
            }),
            dep_slot->slot.dependents.end());
        storage.try_unload_slot(*dep_slot, dep_handle);
    }
}

void DependencyGraph::pin_committed_dependencies(AssetStorage& storage, const AssetSlot& slot) const {
    for (PulseAssetHandle dep_handle : slot.dependencies) {
        if (is_invalid_handle(dep_handle)) {
            continue;
        }

        auto dep_slot = storage.get_slot(dep_handle);
        if (dep_slot && dep_slot->slot.state == PULSE_ASSET_STATE_LOADED) {
            dep_slot->slot.pin_count += 1;
        }
    }
}

void DependencyGraph::unpin_committed_dependencies(AssetStorage& storage, const AssetSlot& slot) const {
    for (PulseAssetHandle dep_handle : slot.dependencies) {
        if (is_invalid_handle(dep_handle)) {
            continue;
        }

        auto dep_slot = storage.get_slot(dep_handle);
        if (dep_slot && dep_slot->slot.pin_count > 0) {
            dep_slot->slot.pin_count -= 1;
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

std::optional<AssetBucketSlot> AssetStorage::get_slot(PulseAssetHandle handle) {
    if (is_invalid_handle(handle)) {
        return std::nullopt;
    }

    auto bucket_it = buckets_.find(handle.type_id);
    if (bucket_it == buckets_.end()) {
        return std::nullopt;
    }

    AssetBucket& bucket = bucket_it->second;
    if (handle.index >= bucket.slots.size()) {
        return std::nullopt;
    }

    AssetSlot& slot = bucket.slots[handle.index];
    if (slot.generation != handle.generation) {
        return std::nullopt;
    }

    return AssetBucketSlot{bucket, slot};
}

std::optional<ConstAssetBucketSlot> AssetStorage::get_slot(PulseAssetHandle handle) const {
    if (is_invalid_handle(handle)) {
        return std::nullopt;
    }

    auto bucket_it = buckets_.find(handle.type_id);
    if (bucket_it == buckets_.end()) {
        return std::nullopt;
    }

    const AssetBucket& bucket = bucket_it->second;
    if (handle.index >= bucket.slots.size()) {
        return std::nullopt;
    }

    const AssetSlot& slot = bucket.slots[handle.index];
    if (slot.generation != handle.generation) {
        return std::nullopt;
    }

    return ConstAssetBucketSlot{bucket, slot};
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

void AssetStorage::destroy_slot(AssetBucket& bucket, AssetSlot& slot, PulseAssetHandle handle) {
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
            PulseAssetHandle handle{bucket_pair.first, static_cast<uint32_t>(slot_idx), slot.generation};
            destroy_slot(bucket, slot, handle);
            slot.data.reset();
        }
    }
    buckets_.clear();
    path_cache_.clear();
}

void AssetStorage::force_destroy_assets(uint64_t type_id) {
    auto bucket_it = buckets_.find(type_id);
    if (bucket_it == buckets_.end()) {
        return;
    }

    AssetBucket& bucket = bucket_it->second;
    for (uint32_t slot_idx = 1; slot_idx < bucket.slots.size(); ++slot_idx) {
        AssetSlot& slot = bucket.slots[slot_idx];
        if (slot.state == PULSE_ASSET_STATE_EMPTY) {
            continue;
        }

        PulseAssetHandle handle{type_id, slot_idx, slot.generation};
        detach_dependents_from_slot(handle, slot);
        destroy_slot(bucket, slot, handle);
        bucket.free_indices.push_back(slot_idx);
    }
}

void AssetStorage::try_unload_slot(AssetBucketSlot bucket_slot, PulseAssetHandle handle) {
    if (is_invalid_handle(handle) ||
        handle.index >= bucket_slot.bucket.slots.size() ||
        &bucket_slot.bucket.slots[handle.index] != &bucket_slot.slot ||
        bucket_slot.slot.generation != handle.generation) {
        return;
    }

    if (bucket_slot.slot.pin_count == 0 && bucket_slot.slot.dependents.empty()) {
        destroy_slot(bucket_slot.bucket, bucket_slot.slot, handle);
        bucket_slot.bucket.free_indices.push_back(handle.index);
    }
}

bool AssetStorage::cached_slot_can_be_reused(PulseAssetHandle handle) const {
    auto slot = get_slot(handle);
    return slot && slot->slot.state != PULSE_ASSET_STATE_PENDING_DELETE;
}

PulseAssetHandle AssetStorage::find_cached(uint64_t type_id, const std::pmr::string& path) const {
    return path_cache_.find(type_id, path);
}

void AssetStorage::cache_path(uint64_t type_id, const std::pmr::string& path, PulseAssetHandle handle) {
    path_cache_.store(type_id, path, handle);
}

void AssetStorage::detach_dependents_from_slot(PulseAssetHandle handle, AssetSlot& slot) {
    for (PulseAssetHandle dependent_handle : slot.dependents) {
        auto dependent_slot = get_slot(dependent_handle);
        if (!dependent_slot) {
            continue;
        }

        dependent_slot->slot.dependencies.erase(
            std::remove_if(dependent_slot->slot.dependencies.begin(), dependent_slot->slot.dependencies.end(), [&](PulseAssetHandle dependency) {
                return handles_equal(dependency, handle);
            }),
            dependent_slot->slot.dependencies.end());
    }
    slot.dependents.clear();
}

} // namespace pulse::asset
