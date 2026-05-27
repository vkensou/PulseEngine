#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "pulse_app.h"

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_ASSET_PLUGIN_DESC_VERSION 1u
#define PULSE_ASSET_TYPE_DESC_VERSION 1u
#define PULSE_ASSET_LOADER_DESC_VERSION 1u
#define PULSE_ASSET_LOAD_DESC_VERSION 1u
#define PULSE_ASSET_MEMORY_LOAD_DESC_VERSION 1u
#define PULSE_ASSET_BUILD_DESC_VERSION 1u
#define PULSE_ASSET_INVALID_INDEX UINT32_MAX

typedef struct pulse_asset_handle {
    uint64_t type_id;
    uint32_t index;
    uint32_t generation;
} pulse_asset_handle;

typedef struct pulse_asset_ref {
    pulse_asset_handle handle;
    void* ptr;
} pulse_asset_ref;

typedef enum pulse_dependency_flags {
    PULSE_DEP_REQUIRED = 0x0,
    PULSE_DEP_OPTIONAL = 0x1,
} pulse_dependency_flags_t;

typedef enum pulse_asset_load_flags {
    PULSE_ASSET_LOAD_DEFAULT = 0x0,
    PULSE_ASSET_LOAD_SKIP_CACHE = 0x1,
} pulse_asset_load_flags_t;

typedef struct pulse_asset_dependency {
    pulse_asset_handle handle;
    pulse_dependency_flags_t flags;
} pulse_asset_dependency_t;

typedef struct pulse_asset_load_dependency_hint pulse_asset_load_dependency_hint;

typedef enum pulse_asset_state {
    PULSE_ASSET_STATE_EMPTY = 0,
    PULSE_ASSET_STATE_WAITING_LOAD,
    PULSE_ASSET_STATE_LOADING,
    PULSE_ASSET_STATE_WAITING_DEPENDENCIES,
    PULSE_ASSET_STATE_PROCESSING,
    PULSE_ASSET_STATE_LOADED,
    PULSE_ASSET_STATE_FAILED,
    PULSE_ASSET_STATE_PENDING_DELETE,
} pulse_asset_state_t;

typedef enum pulse_asset_loader_status {
    PULSE_ASSET_LOADER_PENDING = 0,
    PULSE_ASSET_LOADER_DONE,
    PULSE_ASSET_LOADER_FAILED,
    PULSE_ASSET_LOADER_WAIT_DEPENDENCIES,
} pulse_asset_loader_status_t;

typedef enum pulse_asset_load_source {
    PULSE_ASSET_LOAD_SOURCE_FILE = 0,
    PULSE_ASSET_LOAD_SOURCE_MEMORY,
    PULSE_ASSET_LOAD_SOURCE_BUILDER,
} pulse_asset_load_source_t;

typedef struct pulse_asset_plugin_desc {
    uint32_t struct_size;
    uint32_t version;
    const char* root_path;
    uint32_t max_requests_per_update;
} pulse_asset_plugin_desc;

typedef void (*pulse_asset_destroy_fn)(void* ptr, void* user_data);

typedef struct pulse_asset_type_desc {
    uint32_t struct_size;
    uint32_t version;
    uint64_t type_id;
    uint32_t size;
    uint32_t align;
    pulse_asset_destroy_fn destroy;
    void* user_data;
} pulse_asset_type_desc;

typedef struct pulse_asset_load_task {
    pulse_app_t app;
    uint64_t type_id;
    const char* path;
    const uint8_t* bytes;
    uint64_t byte_size;
    const pulse_asset_dependency* dependencies;
    uint32_t dependency_count;
    pulse_asset_handle handle;
    void* user_data;
    void* out_asset;
    const void* settings;
    pulse_asset_load_dependency_hint* dependency_hint;
    pulse_asset_load_source_t source;
} pulse_asset_load_task;

typedef pulse_result_t (*pulse_asset_loader_ctor_fn)(
    void* loader,
    const pulse_asset_load_task* ctx
);

typedef void (*pulse_asset_loader_dtor_fn)(
    void* loader,
    const pulse_asset_load_task* ctx
);

typedef pulse_asset_loader_status_t (*pulse_asset_loader_step_fn)(
    void* state,
    const pulse_asset_load_task* ctx,
    const char** out_error
);

typedef struct pulse_asset_loader_desc {
    uint32_t struct_size;
    uint32_t version;
    uint64_t type_id;
    const char* extensions;
    pulse_asset_loader_ctor_fn ctor;
    pulse_asset_loader_dtor_fn dtor;
    pulse_asset_loader_step_fn step;
    uint32_t loader_size;
    uint32_t loader_align;
    uint32_t settings_size;
    uint32_t settings_align;
    void* user_data;
} pulse_asset_loader_desc;

typedef struct pulse_asset_load_desc {
    uint32_t struct_size;
    uint32_t version;
    uint64_t type_id;
    const char* path;
    const pulse_asset_dependency* dependencies;
    uint32_t dependency_count;
    const void* settings;
    pulse_asset_load_flags_t flags;
} pulse_asset_load_desc;

typedef struct pulse_asset_memory_load_desc {
    uint32_t struct_size;
    uint32_t version;
    uint64_t type_id;
    const char* path;
    const void* data;
    uint64_t size;
    const pulse_asset_dependency* dependencies;
    uint32_t dependency_count;
    const void* settings;
    pulse_asset_load_flags_t flags;
} pulse_asset_memory_load_desc;

typedef struct pulse_asset_build_desc {
    uint32_t struct_size;
    uint32_t version;
    uint64_t type_id;
    const char* name;
    const pulse_asset_dependency* dependencies;
    uint32_t dependency_count;
    const void* settings;
} pulse_asset_build_desc;

pulse_asset_plugin_desc pulse_asset_plugin_desc_default(void);

pulse_result_t pulse_asset_add_plugin(
    pulse_app_t app,
    const pulse_asset_plugin_desc* desc
);

pulse_result_t pulse_asset_register_type(
    pulse_app_t app,
    const pulse_asset_type_desc* desc
);

pulse_result_t pulse_asset_register_loader(
    pulse_app_t app,
    const pulse_asset_loader_desc* desc
);

pulse_asset_handle pulse_asset_load(
    pulse_app_t app,
    const pulse_asset_load_desc* desc
);

pulse_asset_handle pulse_asset_load_from_memory(
    pulse_app_t app,
    const pulse_asset_memory_load_desc* desc
);

pulse_asset_handle pulse_asset_build(
    pulse_app_t app,
    const pulse_asset_build_desc* desc
);

pulse_result_t pulse_asset_add_load_dependency(
    const pulse_asset_load_task* ctx,
    pulse_asset_handle dependency,
    pulse_dependency_flags_t flags
);

pulse_asset_state_t pulse_asset_get_state(
    pulse_app_t app,
    pulse_asset_handle handle
);

bool pulse_asset_is_available(
    pulse_app_t app,
    pulse_asset_handle handle
);

const char* pulse_asset_get_error(
    pulse_app_t app,
    pulse_asset_handle handle
);

bool pulse_asset_acquire(
    pulse_app_t app,
    pulse_asset_handle handle,
    pulse_asset_ref* out_ref
);

void pulse_asset_release(
    pulse_app_t app,
    pulse_asset_ref* ref
);

void pulse_asset_unload(
    pulse_app_t app,
    pulse_asset_handle handle
);

void pulse_asset_mark_modified(
    pulse_app_t app,
    pulse_asset_handle handle
);

#ifdef __cplusplus
}
#endif
