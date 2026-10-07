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

#define PULSE_TYPE_PREFAB_SCRIPT UINT64_C(0x2001)










typedef struct PulsePrefabData
{
    ecs_entity_t         root;
    [[pulse::optional]] [[pulse::owner]]
    ecs_entity_t*        entities;
    uint32_t             entity_count;

} PulsePrefabData;

/**
 * Entities created by evaluating a .flecs script, owned by the asset and deleted on unload
 *
 */
typedef struct PulsePrefabScriptData
{
    ecs_entity_t         script;

} PulsePrefabScriptData;




// ---- asset type definitions ----

PULSE_DEFINE_ASSET_TYPE(prefab, PULSE_TYPE_PREFAB, PulsePrefabHandle, PulsePrefabRequest)
PULSE_DEFINE_ASSET_TYPE(prefab_script, PULSE_TYPE_PREFAB_SCRIPT, PulsePrefabScriptHandle, PulsePrefabScriptRequest)

PULSE_PREFAB_API EPulseAppAddPluginResult pulse_add_prefab_plugin(PulseAppId app);
PULSE_PREFAB_API PulsePrefabRequest pulse_load_prefab(PulseAppId app, const char* filepath);
PULSE_PREFAB_API ecs_entity_t pulse_prefab_get_root(PulseAppId app, PulsePrefabHandle prefab);
PULSE_PREFAB_API ecs_entity_t pulse_prefab_instantiate(PulseAppId app, PulsePrefabHandle prefab);
PULSE_PREFAB_API PulsePrefabScriptRequest pulse_load_prefab_script(PulseAppId app, const char* filepath);
PULSE_PREFAB_API ecs_entity_t pulse_prefab_script_get_entity(PulseAppId app, PulsePrefabScriptHandle script, const char* name);

/**
 * Instantiates a flecs script template or prefab. Templates get the given props assigned (values parsed per member type: number, bool, enum constant name, asset path; asset paths must be loaded already, the call requests the load and fails otherwise), prefabs are instantiated with IsA and take no props. entity 0 creates a new entity, otherwise the props are set on the existing entity and the template rebuilds. Returns 0 on failure, see pulse_prefab_last_error
 *
 * @param[in] app
 * @param[in] prefab
 * @param[in] entity
 * @param[in] names
 * @param[in] values
 *
 */
PULSE_PREFAB_API ecs_entity_t pulse_prefab_script_instantiate(PulseAppId app, ecs_entity_t prefab, ecs_entity_t entity, Pulse_Array_Param(const char*, names), Pulse_Array_Param(const char*, values));
PULSE_PREFAB_API const char* pulse_prefab_last_error(void);

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
