#pragma once

#include "pulse_asset.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <list>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include <utility>
#include <memory_resource>
#include <string_view>

#define flecs_STATIC
#include <flecs.h>

namespace pulse_asset_internal {
struct pulse_asset_state_o;
struct LoadJob;
}

struct pulse_asset_load_dependency_hint {
    pulse_asset_internal::LoadJob* parent;
};

namespace pulse_asset_internal {

constexpr const char* kPluginName = "PulseAssetPlugin";

struct pulse_asset_state_resource {
    struct pulse_asset_state_o* state;
};

struct AssetType {
    pulse_asset_type_desc desc{};
};

struct AssetLoader {
    pulse_asset_loader_desc desc{};
    std::pmr::vector<std::pmr::string> extensions;

    explicit AssetLoader(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : extensions(resource) {
    }
};

struct PooledBlock;
void free_pooled_block(PooledBlock& block);

struct PooledBlock {
    void* data = nullptr;
    uint32_t size = 0;
    uint32_t align = 0;
    std::pmr::memory_resource* resource = nullptr;

    PooledBlock() = default;
    ~PooledBlock() { free_pooled_block(*this); }

    PooledBlock(const PooledBlock&) = delete;
    PooledBlock& operator=(const PooledBlock&) = delete;

    PooledBlock(PooledBlock&& other) noexcept
        : data(other.data), size(other.size), align(other.align), resource(other.resource) {
        other.data = nullptr;
        other.size = 0;
        other.align = 0;
        other.resource = nullptr;
    }

    PooledBlock& operator=(PooledBlock&& other) noexcept {
        if (this != &other) {
            free_pooled_block(*this);
            data = other.data;
            size = other.size;
            align = other.align;
            resource = other.resource;
            other.data = nullptr;
            other.size = 0;
            other.align = 0;
            other.resource = nullptr;
        }
        return *this;
    }
};

struct AssetSlot {
    uint32_t generation = 1;
    pulse_asset_state_t state = PULSE_ASSET_STATE_EMPTY;
    uint32_t pin_count = 0;
    PooledBlock data;
    std::pmr::string path;
    std::pmr::string error;
    uint64_t version = 0;
    bool constructed = false;
    bool retiring_load_job = false;
    std::pmr::vector<pulse_asset_handle> dependencies;
    std::pmr::vector<pulse_asset_handle> dependents;

    explicit AssetSlot(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : path(resource),
          error(resource),
          dependencies(resource),
          dependents(resource) {
    }

    AssetSlot(const AssetSlot&) = delete;
    AssetSlot& operator=(const AssetSlot&) = delete;
    AssetSlot(AssetSlot&&) noexcept = default;
    AssetSlot& operator=(AssetSlot&&) noexcept = default;
};

struct LoadSource {
    bool from_memory = false;
    std::pmr::vector<uint8_t> memory_data;

    explicit LoadSource(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : memory_data(resource) {
    }
};

enum class LoadJobPhase {
    PendingRead,
    WaitingDependencies,
    Processing,
};

enum class LoadJobOutcome {
    None,
    Loaded,
    Failed,
    Cancelled,
};

struct LoadJob {
    pulse_asset_handle handle{};
    LoadJobPhase phase = LoadJobPhase::PendingRead;
    LoadSource source;
    std::pmr::vector<uint8_t> bytes;
    AssetLoader* loader = nullptr;
    PooledBlock loader_state;
    bool loader_constructed = false;
    PooledBlock settings;
    std::pmr::vector<pulse_asset_dependency> dependencies;
    pulse_asset_load_task ctx{};
    LoadJobOutcome outcome = LoadJobOutcome::None;

    explicit LoadJob(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : source(resource),
          bytes(resource),
          dependencies(resource) {
    }

    LoadJob(const LoadJob&) = delete;
    LoadJob& operator=(const LoadJob&) = delete;
    LoadJob(LoadJob&&) noexcept = default;
    LoadJob& operator=(LoadJob&&) noexcept = default;
};

struct AssetBucket {
    AssetType* type = nullptr;
    std::pmr::vector<AssetSlot> slots;
    std::pmr::vector<uint32_t> free_indices;

    explicit AssetBucket(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : slots(resource),
          free_indices(resource) {
    }

    AssetBucket(const AssetBucket&) = delete;
    AssetBucket& operator=(const AssetBucket&) = delete;
    AssetBucket(AssetBucket&&) noexcept = default;
    AssetBucket& operator=(AssetBucket&&) noexcept = default;
};

struct PathKey {
    uint64_t type_id = 0;
    std::pmr::string path;

    explicit PathKey(std::pmr::memory_resource* resource = std::pmr::get_default_resource())
        : path(resource) {
    }

    PathKey(uint64_t type_id, std::string_view path, std::pmr::memory_resource* resource)
        : type_id(type_id),
          path(path.data(), path.size(), resource) {
    }

    PathKey(uint64_t type_id, const std::pmr::string& path, std::pmr::memory_resource* resource)
        : type_id(type_id),
          path(path, resource) {
    }

    bool operator==(const PathKey& other) const {
        return type_id == other.type_id && path == other.path;
    }
};

struct PathKeyHash {
    size_t operator()(const PathKey& key) const {
        return std::hash<uint64_t>{}(key.type_id) ^
            (std::hash<std::string_view>{}(std::string_view(key.path)) << 1);
    }
};

struct pulse_asset_state_o {
    pulse_app_t app = nullptr;
    pulse_asset_plugin_desc desc{};
    std::pmr::unsynchronized_pool_resource memory_pool;
    std::pmr::string root_path;
    ecs_entity_t process_system = 0;
    std::pmr::unordered_map<uint64_t, AssetType> types;
    std::pmr::deque<AssetLoader> loaders;
    std::pmr::unordered_map<uint64_t, AssetBucket> buckets;
    std::pmr::unordered_map<PathKey, pulse_asset_handle, PathKeyHash> path_cache;
    std::pmr::list<LoadJob> load_jobs;

    pulse_asset_state_o()
        : root_path(&memory_pool),
          types(&memory_pool),
          loaders(&memory_pool),
          buckets(&memory_pool),
          path_cache(&memory_pool),
          load_jobs(&memory_pool) {
    }
};

extern ECS_COMPONENT_DECLARE(pulse_asset_state_resource);

pulse_asset_plugin_desc normalize_plugin_desc(const pulse_asset_plugin_desc* desc);
bool validate_plugin_desc(const pulse_asset_plugin_desc* desc);
pulse_asset_state_o* state_from_app(pulse_app_t app);

std::pmr::string normalize_extension(const char* extension, std::pmr::memory_resource* resource);
std::pmr::vector<std::pmr::string> parse_extensions(const char* extensions, std::pmr::memory_resource* resource);

pulse_asset_handle invalid_handle(void);
bool is_invalid_handle(pulse_asset_handle handle);
std::pmr::string normalize_path(const char* path, std::pmr::memory_resource* resource);
AssetBucket* ensure_bucket(pulse_asset_state_o* state, uint64_t type_id);
AssetSlot* get_slot(pulse_asset_state_o* state, pulse_asset_handle handle);
const AssetSlot* get_slot_const(const pulse_asset_state_o* state, pulse_asset_handle handle);
pulse_asset_handle allocate_slot(
    pulse_asset_state_o* state,
    uint64_t type_id,
    const std::pmr::string& path
);
void destroy_slot(pulse_asset_state_o* state, AssetBucket& bucket, AssetSlot& slot, pulse_asset_handle handle);
void destroy_all_assets(pulse_asset_state_o* state);
void try_unload_slot(pulse_asset_state_o* state, pulse_asset_handle handle);

AssetLoader* find_loader(pulse_asset_state_o* state, uint64_t type_id, const std::pmr::string& path);
std::pmr::string join_asset_path(const std::pmr::string& root_path, const std::pmr::string& path, std::pmr::memory_resource* resource);
std::optional<std::pmr::vector<uint8_t>> read_file_sdl(const char* filename, std::pmr::memory_resource* resource);
void process_load_requests_system(ecs_iter_t* it);
void install_process_system(pulse_asset_state_o* state, ecs_world_t* world);
void uninstall_process_system(pulse_asset_state_o* state, ecs_world_t* world);
void cancel_load_jobs(pulse_asset_state_o* state);
void free_pooled_block(PooledBlock& block);
bool copy_pooled_block(pulse_asset_state_o* state, PooledBlock& out, const void* data, uint32_t size, uint32_t align);
bool allocate_pooled_block(pulse_asset_state_o* state, PooledBlock& out, uint32_t size, uint32_t align, bool zero_memory);
bool load_job_is_terminal(const LoadJob& job);
void finish_load_job(LoadJob& job, AssetSlot* slot, LoadJobOutcome outcome, const char* error);
void retire_load_job(pulse_asset_state_o* state, LoadJob& job);
void commit_asset_dependencies(pulse_asset_state_o* state, pulse_asset_handle handle, const std::pmr::vector<pulse_asset_dependency>& dependencies);

} // namespace pulse_asset_internal
