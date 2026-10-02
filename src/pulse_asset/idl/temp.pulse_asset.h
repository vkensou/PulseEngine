#pragma once

#ifndef PULSE_ASSET_API_HEADER_GUARD
#define PULSE_ASSET_API_HEADER_GUARD
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-attributes"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wattributes"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:5030)
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

#ifdef __cplusplus
extern "C" {
#endif

$cconsts

#define PULSE_DEFINE_ASSET_HANDLE_CONVERSION(name, type_enum, handle_type) \
    static inline PulseAssetHandle pulse_##name##_to_handle(handle_type v) { \
        PulseAssetHandle h = { type_enum, v.index, v.generation }; \
        return h; \
    }

#define PULSE_DEFINE_ASSET_REQUEST_CONVERSION(name, type_enum, request_type) \
    static inline PulseAssetRequest pulse_##name##_request_to_asset_request(request_type r) { \
        PulseAssetRequest out = { type_enum, r.index, r.generation }; \
        return out; \
    }

#define PULSE_DEFINE_ASSET_CONVERSIONS(name, type_enum, handle_type, request_type) \
    PULSE_DEFINE_ASSET_HANDLE_CONVERSION(name, type_enum, handle_type) \
    PULSE_DEFINE_ASSET_REQUEST_CONVERSION(name, type_enum, request_type)

#define PULSE_DEFINE_ASSET_HANDLE_TYPE(handle_type, request_type) \
    typedef struct { uint32_t index; uint32_t generation; } handle_type; \
    typedef struct { uint32_t index; uint32_t generation; } request_type;

#define PULSE_DEFINE_ASSET_GET_HANDLE(name, handle_type, request_type) \
    static inline handle_type pulse_##name##_get_handle(PulseAppId app, request_type request) { \
        PulseAssetHandle h = pulse_asset_system_get_handle(pulse_get_asset_system(app), pulse_##name##_request_to_asset_request(request)); \
        handle_type out; \
        if (!pulse_asset_handle_is_valid(h)) { out.index = 0; out.generation = 0; } \
        else { out.index = h.index; out.generation = h.generation; } \
        return out; \
    }

#define PULSE_DEFINE_ASSET_STATUS(name, request_type) \
    static inline bool pulse_##name##_is_ready(PulseAppId app, request_type request) { \
        return pulse_asset_system_is_ready(pulse_get_asset_system(app), pulse_##name##_request_to_asset_request(request)); \
    } \
    static inline bool pulse_##name##_is_alive(PulseAppId app, request_type request) { \
        return pulse_asset_system_is_alive(pulse_get_asset_system(app), pulse_##name##_request_to_asset_request(request)); \
    } \
    static inline const char* pulse_##name##_get_error(PulseAppId app, request_type request) { \
        return pulse_asset_system_get_error(pulse_get_asset_system(app), pulse_##name##_request_to_asset_request(request)); \
    }

#define PULSE_DEFINE_ASSET_TYPE(name, type_enum, handle_type, request_type) \
    typedef struct { uint32_t index; uint32_t generation; } handle_type; \
    typedef struct { uint32_t index; uint32_t generation; } request_type; \
    PULSE_DEFINE_ASSET_CONVERSIONS(name, type_enum, handle_type, request_type) \
    PULSE_DEFINE_ASSET_GET_HANDLE(name, handle_type, request_type) \
    PULSE_DEFINE_ASSET_STATUS(name, request_type)

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

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
#endif // PULSE_ASSET_API_HEADER_GUARD
