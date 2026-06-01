#pragma once

#ifndef PULSE_ASSET_API_HEADER_GUARD
#define PULSE_ASSET_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t
#include "pulse_app.h"

#ifndef PULSE_API
#define PULSE_API
#endif

/**
 * Constants
 *
 */
#define PULSE_ASSET_PLUGIN_DESC_VERSION 1u

#define PULSE_ASSET_TYPE_DESC_VERSION 1u

#define PULSE_ASSET_LOADER_DESC_VERSION 1u

#define PULSE_ASSET_LOAD_DESC_VERSION 1u

#define PULSE_ASSET_MEMORY_LOAD_DESC_VERSION 1u

#define PULSE_ASSET_BUILD_DESC_VERSION 1u

#define PULSE_ASSET_INVALID_INDEX 0u


typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;

/**
 * Flags: dependency flags (mutually exclusive, but used as bitmask)
 *
 */
typedef enum EPulseLoadDependencyRequirement
{
    PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED, /** ( 0)                                */
    PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL, /** ( 1)                                */

    PULSE_LOAD_DEPENDENCY_REQUIREMENT_COUNT

} EPulseLoadDependencyRequirement;

/**
 * Asset loading state machine
 *
 */
typedef enum EPulseAssetState
{
    PULSE_ASSET_STATE_EMPTY,                  /** ( 0)                                */
    PULSE_ASSET_STATE_WAITING_LOAD,           /** ( 1)                                */
    PULSE_ASSET_STATE_LOADING,                /** ( 2)                                */
    PULSE_ASSET_STATE_WAITING_DEPENDENCIES,   /** ( 3)                                */
    PULSE_ASSET_STATE_PROCESSING,             /** ( 4)                                */
    PULSE_ASSET_STATE_LOADED,                 /** ( 5)                                */
    PULSE_ASSET_STATE_FAILED,                 /** ( 6)                                */
    PULSE_ASSET_STATE_PENDING_DELETE,         /** ( 7)                                */

    PULSE_ASSET_STATE_COUNT

} EPulseAssetState;

/**
 * Loader step result
 *
 */
typedef enum EPulseAssetLoaderStatus
{
    PULSE_ASSET_LOADER_STATUS_PENDING,        /** ( 0)                                */
    PULSE_ASSET_LOADER_STATUS_DONE,           /** ( 1)                                */
    PULSE_ASSET_LOADER_STATUS_FAILED,         /** ( 2)                                */
    PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES, /** ( 3)                                */

    PULSE_ASSET_LOADER_STATUS_COUNT

} EPulseAssetLoaderStatus;

/**
 * Load source type
 *
 */
typedef enum EPulseAssetLoadSource
{
    PULSE_ASSET_LOAD_SOURCE_FILE,             /** ( 0)                                */
    PULSE_ASSET_LOAD_SOURCE_MEMORY,           /** ( 1)                                */
    PULSE_ASSET_LOAD_SOURCE_BUILDER,          /** ( 2)                                */

    PULSE_ASSET_LOAD_SOURCE_COUNT

} EPulseAssetLoadSource;


/**
 * Flags: asset load flags
 *
 */
typedef enum EPulseAssetLoadFlagBits
{
    PULSE_ASSET_LOAD_DEFAULT = 0x00000000,                 /** ( 0)                                */
    PULSE_ASSET_LOAD_SKIP_CACHE = 0x00000001,              /** ( 1)                                */

} EPulseAssetLoadFlagBits;
typedef EPulseFlags EPulseAssetLoadFlags;


DEFINE_PULSE_OBJECT(PulseAssetSystem)

// Forward declarations for types used by function pointers
struct PulseAssetLoadTask;
typedef struct PulseAssetLoadTask PulseAssetLoadTask;

/**
 * Function pointer: asset destroy callback (for type desc)
 *
 * @param[in] ptr
 * @param[in] userData
 *
 */
typedef void (*PulseProcAssetDestroyFn)(void* ptr, void* user_data);
/**
 * Function pointer: loader constructor
 *
 * @param[in] loader
 * @param[in] ctx
 *
 */
typedef EPulseResult (*PulseProcAssetLoaderCtorFn)(void* loader, const PulseAssetLoadTask* ctx);
/**
 * Function pointer: loader destructor
 *
 * @param[in] loader
 * @param[in] ctx
 *
 */
typedef void (*PulseProcAssetLoaderDtorFn)(void* loader, const PulseAssetLoadTask* ctx);
/**
 * Function pointer: loader step / tick
 *
 * @param[in] state
 * @param[in] ctx
 * @param[in] outError
 *
 */
typedef EPulseAssetLoaderStatus (*PulseProcAssetLoaderStepFn)(void* state, const PulseAssetLoadTask* ctx, const char** out_error);

/**
 * Asset identity handle (value type struct)
 *
 */
typedef struct PulseAssetHandle
{
    uint64_t             type_id;
    uint32_t             index;
    uint32_t             generation;

} PulseAssetHandle;

/**
 * Asset acquired reference
 *
 */
typedef struct PulseAssetRef
{
    PulseAssetHandle     handle;
    void*                ptr;

} PulseAssetRef;

/**
 * Asset dependency descriptor
 *
 */
typedef struct PulseAssetDependency
{
    PulseAssetHandle     handle;
    EPulseLoadDependencyRequirement requirement;

} PulseAssetDependency;

/**
 * Opaque forward declaration: load dependency hint (implementation defined)
 *
 */
struct PulseAssetLoadDependencyHint;
typedef struct PulseAssetLoadDependencyHint PulseAssetLoadDependencyHint;

/**
 * Plugin descriptor
 *
 */
typedef struct PulseAssetPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    const char*          root_path;
    uint32_t             max_requests_per_update;

} PulseAssetPluginDesc;

struct PulseAssetSystem;
typedef struct PulseAssetSystem PulseAssetSystem;

/**
 * Type descriptor
 *
 */
typedef struct PulseAssetTypeDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    uint64_t             type_id;
    uint32_t             size;
    uint32_t             align;
    PulseProcAssetDestroyFn destroy;
    void*                user_data;

} PulseAssetTypeDesc;

/**
 * Load task context (passed to loader callbacks)
 *
 */
typedef struct PulseAssetLoadTask
{
    PulseAssetSystemId   asset_system;
    uint64_t             type_id;
    const char*          path;
    const uint8_t*       bytes;
    uint64_t             byte_size;
    const PulseAssetDependency* dependencies;
    uint32_t             dependency_count;
    PulseAssetHandle     handle;
    void*                user_data;
    void*                out_asset;
    const void*          settings;
    PulseAssetLoadDependencyHint* dependency_hint;
    EPulseAssetLoadSource source;

} PulseAssetLoadTask;

/**
 * Loader descriptor
 *
 */
typedef struct PulseAssetLoaderDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    uint64_t             type_id;
    const char*          extensions;
    PulseProcAssetLoaderCtorFn ctor;
    PulseProcAssetLoaderDtorFn dtor;
    PulseProcAssetLoaderStepFn step;
    uint32_t             loader_size;
    uint32_t             loader_align;
    uint32_t             settings_size;
    uint32_t             settings_align;
    void*                user_data;

} PulseAssetLoaderDesc;

/**
 * File-based load descriptor
 *
 */
typedef struct PulseAssetLoadDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    uint64_t             type_id;
    const char*          path;
    const PulseAssetDependency* dependencies;
    uint32_t             dependency_count;
    const void*          settings;
    EPulseAssetLoadFlags flags;

} PulseAssetLoadDesc;

/**
 * In-memory load descriptor
 *
 */
typedef struct PulseAssetMemoryLoadDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    uint64_t             type_id;
    const char*          path;
    const void*          data;
    uint64_t             size;
    const PulseAssetDependency* dependencies;
    uint32_t             dependency_count;
    const void*          settings;
    EPulseAssetLoadFlags flags;

} PulseAssetMemoryLoadDesc;

/**
 * Builder (procedural) load descriptor
 *
 */
typedef struct PulseAssetBuildDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    uint64_t             type_id;
    const char*          name;
    const PulseAssetDependency* dependencies;
    uint32_t             dependency_count;
    const void*          settings;

} PulseAssetBuildDesc;


// Inline helpers for PulseAssetHandle value type
static inline PulseAssetHandle pulse_asset_handle_make_invalid(void) {
    PulseAssetHandle handle = {0, PULSE_ASSET_INVALID_INDEX, 0};
    return handle;
}

static inline bool pulse_asset_handle_is_valid(PulseAssetHandle handle) {
    return handle.type_id != 0 &&
        handle.index != PULSE_ASSET_INVALID_INDEX &&
        handle.generation != 0;
}

static inline bool pulse_asset_handle_equals(PulseAssetHandle a, PulseAssetHandle b) {
    return a.type_id == b.type_id &&
        a.index == b.index &&
        a.generation == b.generation;
}


/**
 * Functions
 *
 */
PULSE_API PulseAssetPluginDesc pulse_asset_plugin_desc_default(void);
PULSE_API EPulseResult pulse_add_asset_plugin(PulseAppId app, const PulseAssetPluginDesc* desc);
PULSE_API PulseAssetSystemId pulse_get_asset_system(PulseAppId app);
PULSE_API EPulseResult pulse_asset_system_register_type(PulseAssetSystemId _this, const PulseAssetTypeDesc* desc);
PULSE_API EPulseResult pulse_asset_system_register_loader(PulseAssetSystemId _this, const PulseAssetLoaderDesc* desc);
PULSE_API PulseAssetHandle pulse_asset_system_load(PulseAssetSystemId _this, const PulseAssetLoadDesc* desc);
PULSE_API PulseAssetHandle pulse_asset_system_load_from_memory(PulseAssetSystemId _this, const PulseAssetMemoryLoadDesc* desc);
PULSE_API PulseAssetHandle pulse_asset_system_build(PulseAssetSystemId _this, const PulseAssetBuildDesc* desc);
PULSE_API EPulseAssetState pulse_asset_system_get_state(PulseAssetSystemId _this, PulseAssetHandle handle);
PULSE_API bool pulse_asset_system_is_alive(PulseAssetSystemId _this, PulseAssetHandle handle);
PULSE_API bool pulse_asset_system_is_ready(PulseAssetSystemId _this, PulseAssetHandle handle);
PULSE_API const char* pulse_asset_system_get_error(PulseAssetSystemId _this, PulseAssetHandle handle);
PULSE_API bool pulse_asset_system_acquire(PulseAssetSystemId _this, PulseAssetHandle handle, PulseAssetRef* out_ref);
PULSE_API void pulse_asset_system_release(PulseAssetSystemId _this, PulseAssetRef* ref);
PULSE_API void pulse_asset_system_unload(PulseAssetSystemId _this, PulseAssetHandle handle);
PULSE_API void pulse_asset_system_mark_modified(PulseAssetSystemId _this, PulseAssetHandle handle);
PULSE_API void pulse_asset_system_force_unload_assets(PulseAssetSystemId _this, uint64_t type_id);
PULSE_API EPulseResult pulse_asset_load_task_add_dependency(PulseAssetLoadDependencyHint* dependency_hint, PulseAssetHandle dependency, EPulseLoadDependencyRequirement requirement);

#ifdef __cplusplus
}
#endif

#endif // PULSE_ASSET_API_HEADER_GUARD
