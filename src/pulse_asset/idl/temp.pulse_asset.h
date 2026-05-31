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

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_ASSET_API_HEADER_GUARD
