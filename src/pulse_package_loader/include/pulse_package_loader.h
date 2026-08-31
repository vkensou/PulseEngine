#pragma once

#ifndef PULSE_PACKAGE_LOADER_API_HEADER_GUARD
#define PULSE_PACKAGE_LOADER_API_HEADER_GUARD
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
#include <stdint.h>  // uint32_t
#include "pulse_platform.h"
#include "pulse_app.h"
#include "pulse_config.h"

#if defined(PULSE_PACKAGE_LOADER_MODULE_BUILD)
#  define PULSE_PACKAGE_LOADER_API PULSE_EXPORT
#else
#  define PULSE_PACKAGE_LOADER_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif



typedef enum EPulsePackageLoadResult
{
    PULSE_PACKAGE_LOAD_RESULT_OK,             /** ( 0)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_LIBRARY_NOT_FOUND, /** ( 3)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_ENTRY_NOT_FOUND, /** ( 4)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED, /** ( 5)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_DUPLICATE_PACKAGE, /** ( 6)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_MISSING_DEPENDENCY, /** ( 7)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_CIRCULAR_DEPENDENCY, /** ( 8)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_UNKNOWN_RUNTIME, /** ( 9)                                */
    PULSE_PACKAGE_LOAD_RESULT_ERROR_INTERNAL, /** (10)                                */

    PULSE_PACKAGE_LOAD_RESULT_COUNT

} EPulsePackageLoadResult;




DEFINE_PULSE_OBJECT(PulsePackageLoader)

typedef EPulseResult (*PulseProcPackageRegisterFn)(PulseAppId app, [[pulse::optional]] PulseConfig* config);

/**
 * Opaque config tree from pulse_config.h
 *
 */
struct PulseConfig;
typedef struct PulseConfig PulseConfig;

typedef struct PulsePackageListEntry
{
    const char*          name;
    [[pulse::optional]]
    PulseConfig*         config;

} PulsePackageListEntry;


[[pulse::optional]] PULSE_PACKAGE_LOADER_API PulsePackageLoaderId pulse_package_loader_create(PulseAppId app);
PULSE_PACKAGE_LOADER_API EPulsePackageLoadResult pulse_package_loader_load_packages(PulsePackageLoaderId loader, Pulse_Array_Param(const char*, search_paths), Pulse_Array_Param(const PulsePackageListEntry, entries));
PULSE_PACKAGE_LOADER_API void pulse_package_loader_register_static_package(PulsePackageLoaderId loader, const char* name, PulseProcPackageRegisterFn register_fn);
PULSE_PACKAGE_LOADER_API void pulse_package_loader_cleanup(PulsePackageLoaderId loader);

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
#endif // PULSE_PACKAGE_LOADER_API_HEADER_GUARD
