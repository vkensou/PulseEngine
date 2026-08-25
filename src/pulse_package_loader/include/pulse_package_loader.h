#pragma once

#ifndef PULSE_PACKAGE_LOADER_API_HEADER_GUARD
#define PULSE_PACKAGE_LOADER_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
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






typedef EPulseResult (*PulseProcPackageRegisterFn)(PulseAppId app, PulseConfig* config);

/**
 * Opaque config tree from pulse_config.h
 *
 */
struct PulseConfig;
typedef struct PulseConfig PulseConfig;

typedef struct PulsePackageListEntry
{
    const char*          name;
    PulseConfig*         config;

} PulsePackageListEntry;


PULSE_PACKAGE_LOADER_API EPulsePackageLoadResult pulse_package_loader_load_packages(PulseAppId app, uint32_t search_path_count, const char** p_search_paths, uint32_t entry_count, const PulsePackageListEntry* p_entries);
PULSE_PACKAGE_LOADER_API void pulse_package_loader_register_static_package(PulseAppId app, const char* name, PulseProcPackageRegisterFn register_fn);
PULSE_PACKAGE_LOADER_API void pulse_package_loader_cleanup(PulseAppId app);

#ifdef __cplusplus
}
#endif

#endif // PULSE_PACKAGE_LOADER_API_HEADER_GUARD