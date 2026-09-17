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

#define PULSE_PREFAB_PLUGIN_DESC_VERSION 1u

/**
 * Asset type id range 0x2000+ belongs to pulse_prefab (graphics uses 0x1000+)
 *
 */
#define PULSE_TYPE_PREFAB UINT64_C(0x2000)










typedef struct PulsePrefabHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulsePrefabHandle;

typedef struct PulsePrefabRequest
{
    uint32_t             index;
    uint32_t             generation;

} PulsePrefabRequest;

typedef struct PulsePrefabData
{
    ecs_entity_t         root;
    [[pulse::optional]] [[pulse::owner]]
    ecs_entity_t*        entities;
    uint32_t             entity_count;

} PulsePrefabData;




// ---- inline helpers for asset handle types ----

PULSE_DEFINE_ASSET_CONVERSIONS(prefab,          PULSE_TYPE_PREFAB,          PulsePrefabHandle,          PulsePrefabRequest)

PULSE_PREFAB_API EPulseAppAddPluginResult pulse_add_prefab_plugin(PulseAppId app);
PULSE_PREFAB_API PulsePrefabRequest pulse_load_prefab(PulseAppId app, const char* filepath);
PULSE_PREFAB_API PulsePrefabHandle pulse_prefab_get_handle(PulseAppId app, PulsePrefabRequest request);
PULSE_PREFAB_API bool pulse_prefab_is_ready(PulseAppId app, PulsePrefabRequest request);
PULSE_PREFAB_API bool pulse_prefab_is_alive(PulseAppId app, PulsePrefabRequest request);
PULSE_PREFAB_API ecs_entity_t pulse_prefab_get_root(PulseAppId app, PulsePrefabHandle prefab);
PULSE_PREFAB_API ecs_entity_t pulse_prefab_instantiate(PulseAppId app, PulsePrefabHandle prefab);

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
