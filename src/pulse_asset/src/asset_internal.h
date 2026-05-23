#pragma once

#include "pulse_asset.h"

#include <algorithm>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

#define flecs_STATIC
#include <flecs.h>

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

struct AssetSlot {
    uint32_t generation = 1;
    pulse_asset_state_t state = PULSE_ASSET_STATE_EMPTY;
    uint32_t pin_count = 0;
    void* data = nullptr;
    std::string path;
    std::string error;
    void* loader_state = nullptr;
    AssetLoader* loader = nullptr;
    uint64_t version = 0;
    bool constructed = false;
    std::vector<pulse_asset_handle> dependencies;
    std::vector<pulse_asset_handle> dependents;
    uint32_t unresolved_count = 0;
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

struct LoadRequest {
    pulse_asset_handle handle{};
    bool from_memory = false;
    std::vector<uint8_t> memory_data;
};

struct ActiveLoad {
    pulse_asset_handle handle{};
    std::vector<uint8_t> bytes;
    std::vector<pulse_asset_handle> dep_handles;
    pulse_asset_load_task ctx{};
    bool from_memory = false;
};

struct pulse_asset_state_o {
    pulse_app_t app = nullptr;
    pulse_asset_plugin_desc desc{};
    std::string root_path;
    ecs_entity_t process_system = 0;
    std::unordered_map<uint64_t, AssetType> types;
    std::deque<AssetLoader> loaders;
    std::unordered_map<uint64_t, AssetBucket> buckets;
    std::unordered_map<PathKey, pulse_asset_handle, PathKeyHash> path_cache;
    std::deque<LoadRequest> pending_requests;
    std::vector<ActiveLoad> active_loads;
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
pulse_asset_handle allocate_memory_slot(
    pulse_asset_state_o* state,
    uint64_t type_id,
    const std::string& name
);
void destroy_slot(AssetBucket& bucket, AssetSlot& slot);
void destroy_all_assets(pulse_asset_state_o* state);

AssetLoader* find_loader(pulse_asset_state_o* state, uint64_t type_id, const std::string& path);
std::string join_asset_path(const std::string& root_path, const std::string& path);
std::optional<std::vector<uint8_t>> read_file_sdl(const char* filename);
void process_load_requests_system(ecs_iter_t* it);
void install_process_system(pulse_asset_state_o* state, ecs_world_t* world);
void uninstall_process_system(pulse_asset_state_o* state, ecs_world_t* world);
void cancel_active_loads(pulse_asset_state_o* state);

} // namespace pulse_asset_internal