#pragma once

#ifndef PULSE_ASSET_API_HEADER_GUARD
#define PULSE_ASSET_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t, uint64_t
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_ASSET_MODULE_BUILD)
#  define PULSE_ASSET_API PULSE_EXPORT
#else
#  define PULSE_ASSET_API PULSE_IMPORT
#endif

$cconsts

typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;

$cenums

$cflags

$cids

// Forward declarations for types used by function pointers
struct PulseAssetLoadTask;
typedef struct PulseAssetLoadTask PulseAssetLoadTask;

$cfuncptrs

$cstructs

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

// Inline helpers for PulseAssetRequest value type
static inline PulseAssetRequest pulse_asset_request_make_invalid(void) {
    PulseAssetRequest request = {0, PULSE_ASSET_INVALID_INDEX, 0};
    return request;
}

static inline bool pulse_asset_request_is_valid(PulseAssetRequest request) {
    return request.type_id != 0 &&
        request.index != PULSE_ASSET_INVALID_INDEX &&
        request.generation != 0;
}

static inline bool pulse_asset_request_equals(PulseAssetRequest a, PulseAssetRequest b) {
    return a.type_id == b.type_id &&
        a.index == b.index &&
        a.generation == b.generation;
}

// Inline helpers for PulseAssetDepRef value type
static inline PulseAssetDepRef pulse_asset_dep_ref_make_invalid(void) {
    PulseAssetDepRef ref = {0, PULSE_ASSET_INVALID_INDEX, 0};
    return ref;
}

static inline bool pulse_asset_dep_ref_is_valid(PulseAssetDepRef ref) {
    return ref.type_id != 0 &&
        ref.index != PULSE_ASSET_INVALID_INDEX &&
        ref.generation != 0;
}

static inline bool pulse_asset_dep_ref_equals(PulseAssetDepRef a, PulseAssetDepRef b) {
    return a.type_id == b.type_id &&
        a.index == b.index &&
        a.generation == b.generation;
}

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_ASSET_API_HEADER_GUARD
