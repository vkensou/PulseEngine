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
    std::vector<std::string> extensions;
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
    std::string path;
    std::string error;
    uint64_t version = 0;
    bool constructed = false;
    bool retiring_load_job = false;
    std::vector<pulse_asset_handle> dependencies;
    std::vector<pulse_asset_handle> dependents;
};

struct LoadSource {
    bool from_memory = false;
    std::vector<uint8_t> memory_data;
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
    std::vector<uint8_t> bytes;
    AssetLoader* loader = nullptr;
    PooledBlock loader_state;
    bool loader_constructed = false;
    PooledBlock settings;
    std::vector<pulse_asset_dependency> dependencies;
    pulse_asset_load_task ctx{};
    LoadJobOutcome outcome = LoadJobOutcome::None;
};

struct AssetBucket {
    AssetType* type = nullptr;
    std::vector<AssetSlot> slots;
    std::vector<uint32_t> free_indices;
};

struct PathKey {
    uint64_t type_id = 0;
    std::string path;

    bool operator==(const PathKey& other) const {
        return type_id == other.type_id && path == other.path;
    }
};

struct PathKeyHash {
    size_t operator()(const PathKey& key) const {
        return std::hash<uint64_t>{}(key.type_id) ^
            (std::hash<std::string>{}(key.path) << 1);
    }
};

struct pulse_asset_state_o {
    pulse_app_t app = nullptr;
    pulse_asset_plugin_desc desc{};
    std::string root_path;
    ecs_entity_t process_system = 0;
    std::pmr::unsynchronized_pool_resource memory_pool;
    std::unordered_map<uint64_t, AssetType> types;
    std::deque<AssetLoader> loaders;
    std::unordered_map<uint64_t, AssetBucket> buckets;
    std::unordered_map<PathKey, pulse_asset_handle, PathKeyHash> path_cache;
    std::list<LoadJob> load_jobs;
};

extern ECS_COMPONENT_DECLARE(pulse_asset_state_resource);

pulse_asset_plugin_desc normalize_plugin_desc(const pulse_asset_plugin_desc* desc);
bool validate_plugin_desc(const pulse_asset_plugin_desc* desc);
pulse_asset_state_o* state_from_app(pulse_app_t app);

std::string normalize_extension(const char* extension);
std::vector<std::string> parse_extensions(const char* extensions);

pulse_asset_handle invalid_handle(void);
bool is_invalid_handle(pulse_asset_handle handle);
std::string normalize_path(const char* path);
AssetBucket* ensure_bucket(pulse_asset_state_o* state, uint64_t type_id);
AssetSlot* get_slot(pulse_asset_state_o* state, pulse_asset_handle handle);
const AssetSlot* get_slot_const(const pulse_asset_state_o* state, pulse_asset_handle handle);
pulse_asset_handle allocate_slot(
    pulse_asset_state_o* state,
    uint64_t type_id,
    const std::string& path
);
void destroy_slot(pulse_asset_state_o* state, AssetBucket& bucket, AssetSlot& slot, pulse_asset_handle handle);
void destroy_all_assets(pulse_asset_state_o* state);
void try_unload_slot(pulse_asset_state_o* state, pulse_asset_handle handle);

AssetLoader* find_loader(pulse_asset_state_o* state, uint64_t type_id, const std::string& path);
std::string join_asset_path(const std::string& root_path, const std::string& path);
std::optional<std::vector<uint8_t>> read_file_sdl(const char* filename);
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
void commit_asset_dependencies(pulse_asset_state_o* state, pulse_asset_handle handle, const std::vector<pulse_asset_dependency>& dependencies);

} // namespace pulse_asset_internal
