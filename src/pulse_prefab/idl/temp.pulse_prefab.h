#pragma once

#ifndef PULSE_PREFAB_API_HEADER_GUARD
#define PULSE_PREFAB_API_HEADER_GUARD
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

#include <stdint.h>
#include "pulse_platform.h"
#include "pulse_app.h"

#include "pulse_asset.h"

#if defined(PULSE_PREFAB_MODULE_BUILD)
#  define PULSE_PREFAB_API PULSE_EXPORT
#else
#  define PULSE_PREFAB_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

$ccomponents

// ---- inline helpers for asset handle types ----

static inline PulseAssetHandle pulse_prefab_to_handle(PulsePrefabHandle prefab) {
    PulseAssetHandle h = { PULSE_TYPE_PREFAB, prefab.index, prefab.generation };
    return h;
}
static inline PulseAssetRequest pulse_prefab_request_to_asset_request(PulsePrefabRequest prefab) {
    PulseAssetRequest r = { PULSE_TYPE_PREFAB, prefab.index, prefab.generation };
    return r;
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
#endif // PULSE_PREFAB_API_HEADER_GUARD
