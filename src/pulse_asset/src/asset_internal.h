#pragma once

#include "pulse_asset.h"

#include <cstdint>
#include <deque>
#include <list>
#include <memory_resource>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#define flecs_STATIC
#include <flecs.h>

namespace pulse::asset {

class AssetSystem;
class AssetRegistry;
class AssetStorage;
class AssetSlot;
class LoadJob;

constexpr const char* kPluginName = "PulseAssetPlugin";

PulseAssetHandle invalid_handle();
bool is_invalid_handle(PulseAssetHandle handle);
bool handles_equal(PulseAssetHandle a, PulseAssetHandle b);

class PooledBlock final {
public:
    void* data = nullptr;
    uint32_t size = 0;
    uint32_t align = 0;
    std::pmr::memory_resource* resource = nullptr;

    PooledBlock() = default;
    ~PooledBlock();

    PooledBlock(const PooledBlock&) = delete;
    PooledBlock& operator=(const PooledBlock&) = delete;

    PooledBlock(PooledBlock&& other) noexcept;
    PooledBlock& operator=(PooledBlock&& other) noexcept;

    void reset();
    bool allocate(std::pmr::memory_resource* allocator, uint32_t byte_size, uint32_t alignment, bool zero_memory);
    bool copy(std::pmr::memory_resource* allocator, const void* bytes, uint32_t byte_size, uint32_t alignment);
};

class AssetIo final {
public:
    static std::pmr::string normalize_path(const char* path, std::pmr::memory_resource* resource);
    static std::pmr::string normalize_extension(const char* extension, std::pmr::memory_resource* resource);
    static std::pmr::vector<std::pmr::string> parse_extensions(const char* extensions, std::pmr::memory_resource* resource);
    static std::pmr::string extension_from_path(const std::pmr::string& path, std::pmr::memory_resource* resource);
    static std::pmr::string join_path(const std::pmr::string& root_path, const std::pmr::string& path, std::pmr::memory_resource* resource);
    static std::optional<std::pmr::vector<uint8_t>> read_file(const char* filename, std::pmr::memory_resource* resource);
};

class AssetLoader final {
public:
    PulseAssetLoaderDesc desc{};
    std::pmr::vector<std::pmr::string> extensions;

    explicit AssetLoader(std::pmr::memory_resource* resource);

    bool is_builder() const;
};

class AssetType final {
public:
    PulseAssetTypeDesc desc{};
    std::pmr::deque<AssetLoader> loaders;
    std::pmr::unordered_map<std::pmr::string, AssetLoader*> extension_loaders;

    explicit AssetType(std::pmr::memory_resource* resource);

    AssetType(const AssetType&) = delete;
    AssetType& operator=(const AssetType&) = delete;
    AssetType(AssetType&&) noexcept = default;
    AssetType& operator=(AssetType&&) noexcept = default;

    bool has_loader_for_any(const std::pmr::vector<std::pmr::string>& extension_list) const;
    AssetLoader* find_builder_loader();
    AssetLoader* find_extension_loader(const std::pmr::string& extension);
    EPulseResult add_loader(const PulseAssetLoaderDesc& loader_desc, std::pmr::vector<std::pmr::string>&& extension_list, std::pmr::memory_resource* resource);
};

class AssetRegistry final {
public:
    explicit AssetRegistry(std::pmr::memory_resource* resource);

    EPulseResult register_type(const PulseAssetTypeDesc* desc);
    EPulseResult register_loader(const PulseAssetLoaderDesc* desc);
    AssetType* find_type(uint64_t type_id);
    AssetLoader* find_loader(uint64_t type_id, const std::pmr::string& path);
    AssetLoader* find_builder_loader(uint64_t type_id);

private:
    std::pmr::memory_resource* resource_ = nullptr;
    std::pmr::unordered_map<uint64_t, AssetType> types_;
};

struct PathKey {
    uint64_t type_id = 0;
    std::pmr::string path;

    PathKey(uint64_t type_id, const std::pmr::string& path, std::pmr::memory_resource* resource);

    bool operator==(const PathKey& other) const;
};

struct PathKeyHash {
    size_t operator()(const PathKey& key) const;
};

class PathCache final {
public:
    explicit PathCache(std::pmr::memory_resource* resource);

    PulseAssetHandle find(uint64_t type_id, const std::pmr::string& path) const;
    void store(uint64_t type_id, const std::pmr::string& path, PulseAssetHandle handle);
    void erase_if_matches(PulseAssetHandle handle, const std::pmr::string& path);
    void clear();

private:
    std::pmr::memory_resource* resource_ = nullptr;
    std::pmr::unordered_map<PathKey, PulseAssetHandle, PathKeyHash> entries_;
};

class AssetSlot final {
public:
    uint32_t generation = 1;
    EPulseAssetState state = PULSE_ASSET_STATE_EMPTY;
    uint32_t pin_count = 0;
    PooledBlock data;
    std::pmr::string path;
    std::pmr::string error;
    uint64_t version = 0;
    bool constructed = false;
    bool retiring_load_job = false;
    std::pmr::vector<PulseAssetHandle> dependencies;
    std::pmr::vector<PulseAssetHandle> dependents;

    explicit AssetSlot(std::pmr::memory_resource* resource);

    AssetSlot(const AssetSlot&) = delete;
    AssetSlot& operator=(const AssetSlot&) = delete;
    AssetSlot(AssetSlot&&) noexcept = default;
    AssetSlot& operator=(AssetSlot&&) noexcept = default;
};

class AssetBucket final {
public:
    AssetType* type = nullptr;
    std::pmr::vector<AssetSlot> slots;
    std::pmr::vector<uint32_t> free_indices;

    explicit AssetBucket(std::pmr::memory_resource* resource);

    AssetBucket(const AssetBucket&) = delete;
    AssetBucket& operator=(const AssetBucket&) = delete;
    AssetBucket(AssetBucket&&) noexcept = default;
    AssetBucket& operator=(AssetBucket&&) noexcept = default;
};

struct AssetSlotAllocation {
    PulseAssetHandle handle{};
    AssetSlot* slot = nullptr;
};

struct AssetBucketSlot {
    AssetBucket& bucket;
    AssetSlot& slot;
};

struct ConstAssetBucketSlot {
    const AssetBucket& bucket;
    const AssetSlot& slot;
};

class DependencyGraph final {
public:
    void evaluate(const AssetStorage& storage, const std::pmr::vector<PulseAssetDependency>& dependencies, bool& out_failed, bool& out_ready) const;
    void commit(AssetStorage& storage, PulseAssetHandle handle, const std::pmr::vector<PulseAssetDependency>& dependencies) const;
    void detach_committed_dependencies(AssetStorage& storage, PulseAssetHandle handle, AssetSlot& slot) const;
    void pin_committed_dependencies(AssetStorage& storage, const AssetSlot& slot) const;
    void unpin_committed_dependencies(AssetStorage& storage, const AssetSlot& slot) const;
};

class AssetStorage final {
public:
    AssetStorage(std::pmr::memory_resource* resource, AssetRegistry& registry);

    AssetBucket* ensure_bucket(uint64_t type_id);
    std::optional<AssetBucketSlot> get_slot(PulseAssetHandle handle);
    std::optional<ConstAssetBucketSlot> get_slot(PulseAssetHandle handle) const;
    AssetSlotAllocation allocate_slot(uint64_t type_id, const std::pmr::string& path);
    void destroy_slot(AssetBucket& bucket, AssetSlot& slot, PulseAssetHandle handle);
    void destroy_all_assets();
    void force_destroy_assets(uint64_t type_id);
    void try_unload_slot(AssetBucketSlot bucket_slot, PulseAssetHandle handle);
    bool cached_slot_can_be_reused(PulseAssetHandle handle) const;
    PulseAssetHandle find_cached(uint64_t type_id, const std::pmr::string& path) const;
    void cache_path(uint64_t type_id, const std::pmr::string& path, PulseAssetHandle handle);

    DependencyGraph& dependencies() { return dependency_graph_; }
    const DependencyGraph& dependencies() const { return dependency_graph_; }

private:
    std::pmr::memory_resource* resource_ = nullptr;
    AssetRegistry& registry_;
    PathCache path_cache_;
    DependencyGraph dependency_graph_;
    std::pmr::unordered_map<uint64_t, AssetBucket> buckets_;

    void detach_dependents_from_slot(PulseAssetHandle handle, AssetSlot& slot);
};

class LoadSource final {
public:
    EPulseAssetLoadSource kind = PULSE_ASSET_LOAD_SOURCE_FILE;
    std::pmr::vector<uint8_t> memory_data;

    explicit LoadSource(std::pmr::memory_resource* resource);
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

class LoadJob final {
public:
    PulseAssetHandle handle{};
    LoadJobPhase phase = LoadJobPhase::PendingRead;
    LoadSource source;
    std::pmr::vector<uint8_t> bytes;
    AssetLoader* loader = nullptr;
    PooledBlock loader_state;
    bool loader_constructed = false;
    PooledBlock settings;
    std::pmr::vector<PulseAssetDependency> dependencies;
    PulseAssetLoadTask ctx{};
    LoadJobOutcome outcome = LoadJobOutcome::None;

    explicit LoadJob(std::pmr::memory_resource* resource);

    LoadJob(const LoadJob&) = delete;
    LoadJob& operator=(const LoadJob&) = delete;
    LoadJob(LoadJob&&) noexcept = default;
    LoadJob& operator=(LoadJob&&) noexcept = default;

    bool is_terminal() const;
    void finish(AssetSlot* slot, LoadJobOutcome next_outcome, const char* error);
    EPulseResult add_dependency(PulseAssetHandle dependency, EPulseLoadDependencyRequirement flags);
};

struct LoadRequest {
    EPulseAssetLoadSource source = PULSE_ASSET_LOAD_SOURCE_FILE;
    uint64_t type_id = 0;
    const char* path_or_name = nullptr;
    const void* settings = nullptr;
    EPulseAssetLoadFlags flags = PULSE_ASSET_LOAD_DEFAULT;
    const void* data = nullptr;
    uint64_t size = 0;
    const PulseAssetDependency* dependencies = nullptr;
    uint32_t dependency_count = 0;
};

class LoadContext final {
public:
    static void refresh(AssetSystem& system, LoadJob& job, AssetSlot& slot);
};

class LoadQueue final {
public:
    using JobList = std::pmr::list<LoadJob>;
    using JobIterator = JobList::iterator;

    explicit LoadQueue(std::pmr::memory_resource* resource);

    JobIterator enqueue(LoadJob&& job);
    void process(AssetSystem& system);
    LoadJobOutcome process_immediate_builder(AssetSystem& system, JobIterator job_it);
    void cancel_all(AssetSystem& system);
    void cancel_type(AssetSystem& system, uint64_t type_id);
    std::optional<AssetBucketSlot> retire_load_job(AssetSystem& system, LoadJob& job);
    void retire_and_erase(AssetSystem& system, JobIterator job_it);

private:
    JobList jobs_;

    void process_job(AssetSystem& system, LoadJob& job);
    void process_pending_read(AssetSystem& system, LoadJob& job, AssetSlot& slot, bool process_immediately);
    void process_waiting_dependencies(AssetSystem& system, LoadJob& job, AssetSlot& slot);
    void process_processing(AssetSystem& system, LoadJob& job, AssetSlot& slot);
    bool construct_job_loader(AssetSystem& system, LoadJob& job, AssetSlot& slot, const char*& out_error);
};

EPulseResult asset_plugin_build_callback(PulseAppId app, void* ctx);
void asset_plugin_shutdown_callback(PulseAppId app, void* ctx);

class AssetSystem final {
public:
    explicit AssetSystem(const PulseAssetPluginDesc& desc);
    ~AssetSystem() = default;

    AssetSystem(const AssetSystem&) = delete;
    AssetSystem& operator=(const AssetSystem&) = delete;

    EPulseResult build(PulseAppId app, ecs_world_t* world);
    void shutdown(PulseAppId app);
    void process_load_requests();

    EPulseResult register_type(const PulseAssetTypeDesc* desc);
    EPulseResult register_loader(const PulseAssetLoaderDesc* desc);
    PulseAssetHandle load(const PulseAssetLoadDesc* desc);
    PulseAssetHandle load_from_memory(const PulseAssetMemoryLoadDesc* desc);
    PulseAssetHandle build_asset(const PulseAssetBuildDesc* desc);
    EPulseAssetState get_state(PulseAssetHandle handle) const;
    const char* get_error(PulseAssetHandle handle) const;
    bool acquire(PulseAssetHandle handle, PulseAssetRef* out_ref);
    void release(PulseAssetRef* ref);
    void unload(PulseAssetHandle handle);
    void mark_modified(PulseAssetHandle handle);
    void force_unload_assets(uint64_t type_id);

    PulseAppId app() const { return app_; }
    uint32_t max_requests_per_update() const { return desc_.max_requests_per_update; }
    const std::pmr::string& root_path() const { return root_path_; }
    std::pmr::memory_resource* resource() { return &memory_pool_; }
    AssetRegistry& registry() { return registry_; }
    const AssetRegistry& registry() const { return registry_; }
    AssetStorage& storage() { return storage_; }
    const AssetStorage& storage() const { return storage_; }
    LoadQueue& load_queue() { return load_queue_; }

private:
    PulseAppId app_ = nullptr;
    PulseAssetPluginDesc desc_{};
    std::pmr::unsynchronized_pool_resource memory_pool_;
    std::pmr::string root_path_;
    ecs_entity_t process_system_ = 0;
    AssetRegistry registry_;
    AssetStorage storage_;
    LoadQueue load_queue_;

    PulseAssetHandle load_impl(const LoadRequest& request);
    bool request_dependencies_are_valid(const LoadRequest& request) const;
    LoadJobPhase choose_initial_phase(const LoadRequest& request, AssetSlot& slot);
    bool copy_request_settings(const AssetLoader& loader, const LoadRequest& request, PooledBlock& out);
    bool init_load_job(LoadJob& job, const LoadRequest& request, PulseAssetHandle handle, AssetLoader* loader, LoadJobPhase phase);
    void install_process_system(ecs_world_t* world);
    void uninstall_process_system(ecs_world_t* world);
    static bool is_load_in_progress_state(EPulseAssetState state);
};

} // namespace pulse::asset

struct PulseAssetLoadDependencyHint {
    pulse::asset::LoadJob* parent;
};
