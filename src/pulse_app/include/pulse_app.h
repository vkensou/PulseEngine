#pragma once

#ifndef PULSE_APP_API_HEADER_GUARD
#define PULSE_APP_API_HEADER_GUARD
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

#include <flecs.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t
#include "pulse_platform.h"

#if defined(PULSE_APP_MODULE_BUILD)
#  define PULSE_APP_API PULSE_EXPORT
#else
#  define PULSE_APP_API PULSE_IMPORT
#endif

#define PULSE_PLUGIN_DESC_VERSION 2u


typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;
typedef struct ecs_world_t ecs_world_t;

typedef enum EPulseResult
{
    PULSE_RESULT_OK,                          /** ( 0)                                */
    PULSE_RESULT_ERROR_INVALID_ARGUMENT,      /** ( 1)                                */
    PULSE_RESULT_ERROR_INVALID_STATE,         /** ( 2)                                */
    PULSE_RESULT_ERROR_DUPLICATE_PLUGIN,      /** ( 3)                                */
    PULSE_RESULT_ERROR_DUPLICATE_SUBAPP,      /** ( 4)                                */
    PULSE_RESULT_ERROR_NOT_FOUND,             /** ( 5)                                */
    PULSE_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY, /** ( 6)                                */
    PULSE_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY, /** ( 7)                                */
    PULSE_RESULT_ERROR_INTERNAL,              /** ( 8)                                */

    PULSE_RESULT_COUNT

} EPulseResult;

typedef enum EPulseAppRunResult
{
    PULSE_APP_RUN_RESULT_OK,                  /** ( 0)                                */
    PULSE_APP_RUN_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_RUN_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_RUN_RESULT_ERROR_PLUGIN_BUILD_FAILED, /** ( 3)                                */
    PULSE_APP_RUN_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY, /** ( 4)                                */
    PULSE_APP_RUN_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY, /** ( 5)                                */
    PULSE_APP_RUN_RESULT_ERROR_PLUGIN_POST_BUILD_FAILED, /** ( 6)                                */
    PULSE_APP_RUN_RESULT_ERROR_SUBAPP_POST_BUILD_FAILED, /** ( 7)                                */
    PULSE_APP_RUN_RESULT_ERROR_SUBAPP_PREPARE_FAILED, /** ( 8)                                */
    PULSE_APP_RUN_RESULT_ERROR_SUBAPP_EXTRACT_FAILED, /** ( 9)                                */
    PULSE_APP_RUN_RESULT_ERROR_SUBAPP_UPDATE_FAILED, /** (10)                                */
    PULSE_APP_RUN_RESULT_ERROR_INTERNAL,      /** (11)                                */

    PULSE_APP_RUN_RESULT_COUNT

} EPulseAppRunResult;

typedef enum EPulseAppPrepareResult
{
    PULSE_APP_PREPARE_RESULT_OK,              /** ( 0)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_PLUGIN_BUILD_FAILED, /** ( 3)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY, /** ( 4)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY, /** ( 5)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_PLUGIN_POST_BUILD_FAILED, /** ( 6)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_SUBAPP_POST_BUILD_FAILED, /** ( 7)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_SUBAPP_PREPARE_FAILED, /** ( 8)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_INTERNAL,  /** ( 9)                                */

    PULSE_APP_PREPARE_RESULT_COUNT

} EPulseAppPrepareResult;

typedef enum EPulseAppUpdateResult
{
    PULSE_APP_UPDATE_RESULT_OK,               /** ( 0)                                */
    PULSE_APP_UPDATE_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_UPDATE_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_UPDATE_RESULT_ERROR_SUBAPP_EXTRACT_FAILED, /** ( 3)                                */
    PULSE_APP_UPDATE_RESULT_ERROR_SUBAPP_UPDATE_FAILED, /** ( 4)                                */
    PULSE_APP_UPDATE_RESULT_ERROR_INTERNAL,   /** ( 5)                                */

    PULSE_APP_UPDATE_RESULT_COUNT

} EPulseAppUpdateResult;

typedef enum EPulseAppSetRunnerResult
{
    PULSE_APP_SET_RUNNER_RESULT_OK,           /** ( 0)                                */
    PULSE_APP_SET_RUNNER_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_SET_RUNNER_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_SET_RUNNER_RESULT_ERROR_INTERNAL, /** ( 3)                                */

    PULSE_APP_SET_RUNNER_RESULT_COUNT

} EPulseAppSetRunnerResult;

typedef enum EPulseAppAddPluginResult
{
    PULSE_APP_ADD_PLUGIN_RESULT_OK,           /** ( 0)                                */
    PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN, /** ( 3)                                */
    PULSE_APP_ADD_PLUGIN_RESULT_ERROR_PLUGIN_BUILD_FAILED, /** ( 4)                                */
    PULSE_APP_ADD_PLUGIN_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY, /** ( 5)                                */
    PULSE_APP_ADD_PLUGIN_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY, /** ( 6)                                */
    PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INTERNAL, /** ( 7)                                */

    PULSE_APP_ADD_PLUGIN_RESULT_COUNT

} EPulseAppAddPluginResult;

typedef enum EPulseAppInsertSubappResult
{
    PULSE_APP_INSERT_SUBAPP_RESULT_OK,        /** ( 0)                                */
    PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_DUPLICATE_SUBAPP, /** ( 3)                                */
    PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INTERNAL, /** ( 4)                                */

    PULSE_APP_INSERT_SUBAPP_RESULT_COUNT

} EPulseAppInsertSubappResult;

typedef enum EPulseAppSetSubappExtractResult
{
    PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_OK,   /** ( 0)                                */
    PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_NOT_FOUND, /** ( 3)                                */
    PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INTERNAL, /** ( 4)                                */

    PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_COUNT

} EPulseAppSetSubappExtractResult;

typedef enum EPulseRunnerResult
{
    PULSE_RUNNER_RESULT_OK,                   /** ( 0)                                */
    PULSE_RUNNER_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_RUNNER_RESULT_ERROR_INVALID_STATE,  /** ( 2)                                */
    PULSE_RUNNER_RESULT_ERROR_SUBAPP_EXTRACT_FAILED, /** ( 3)                                */
    PULSE_RUNNER_RESULT_ERROR_SUBAPP_UPDATE_FAILED, /** ( 4)                                */
    PULSE_RUNNER_RESULT_ERROR_INTERNAL,       /** ( 5)                                */

    PULSE_RUNNER_RESULT_COUNT

} EPulseRunnerResult;

typedef enum EPulseSubappExtractResult
{
    PULSE_SUBAPP_EXTRACT_RESULT_OK,           /** ( 0)                                */
    PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INTERNAL, /** ( 3)                                */

    PULSE_SUBAPP_EXTRACT_RESULT_COUNT

} EPulseSubappExtractResult;

typedef enum EPulsePluginBuildResult
{
    PULSE_PLUGIN_BUILD_RESULT_OK,             /** ( 0)                                */
    PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_PLUGIN_BUILD_RESULT_ERROR_DUPLICATE_PLUGIN, /** ( 3)                                */
    PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL, /** ( 4)                                */

    PULSE_PLUGIN_BUILD_RESULT_COUNT

} EPulsePluginBuildResult;




DEFINE_PULSE_OBJECT(PulseApp)

typedef struct PulsePluginDesc PulsePluginDesc;

typedef EPulseRunnerResult (*PulseProcRunnerFn)(PulseAppId app, [[pulse::optional]] void* ctx);
typedef EPulseSubappExtractResult (*PulseProcSubappExtractFn)(PulseAppId app, PulseAppId subapp, [[pulse::optional]] void* ctx);
typedef EPulsePluginBuildResult (*PulseProcPluginBuildFn)(PulseAppId app, [[pulse::optional]] void* ctx);
typedef void (*PulseProcPluginShutdownFn)(PulseAppId app, [[pulse::optional]] void* ctx);

typedef struct PulseAppDesc
{
    [[pulse::optional]]
    const char*          name;
    bool                 enable_restapi;

} PulseAppDesc;

typedef struct PulsePluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    uint32_t             plugin_version;
    const char*          name;
    [[pulse::optional]] [[pulse::retain]]
    void*                ctx;
    PulseProcPluginBuildFn build;
    PulseProcPluginBuildFn post_build;
    PulseProcPluginShutdownFn shutdown;
    uint32_t             dependency_count;
    const char**         dependencies;

} PulsePluginDesc;

struct PulseApp;
typedef struct PulseApp PulseApp;


/**
 * Per-frame timing (singleton component, updated every frame by the time system)
 *
 */
typedef struct PulseTimer
{
    float                delta_time;
    float                time_since_startup;
    double               delta_time_double;
    double               time_since_startup_double;
    int32_t              fps;

} PulseTimer;
PULSE_APP_API extern ECS_COMPONENT_DECLARE(PulseTimer);


[[pulse::optional]] [[pulse::owner]] PULSE_APP_API PulseAppId pulse_create_app(const PulseAppDesc* desc);
PULSE_APP_API void pulse_destroy_app([[pulse::owner]] PulseAppId app);
PULSE_APP_API EPulseAppRunResult pulse_app_run(PulseAppId _this);
PULSE_APP_API EPulseAppPrepareResult pulse_app_prepare(PulseAppId _this);
PULSE_APP_API EPulseAppUpdateResult pulse_app_update(PulseAppId _this);
PULSE_APP_API void pulse_app_teardown(PulseAppId _this);
PULSE_APP_API void pulse_app_finish(PulseAppId _this);
PULSE_APP_API bool pulse_app_should_quit(Const_PulseAppId _this);
PULSE_APP_API EPulseAppSetRunnerResult pulse_app_set_runner(PulseAppId _this, PulseProcRunnerFn runner, [[pulse::optional]] [[pulse::retain]] void* ctx);
PULSE_APP_API EPulseAppAddPluginResult pulse_app_add_plugin(PulseAppId _this, const PulsePluginDesc* desc);
PULSE_APP_API bool pulse_app_has_plugin(Const_PulseAppId _this, const char* name);
PULSE_APP_API ecs_world_t* pulse_app_world(PulseAppId _this);
PULSE_APP_API const char* pulse_app_last_error(Const_PulseAppId _this);
PULSE_APP_API EPulseAppInsertSubappResult pulse_app_insert_subapp(PulseAppId _this, const char* name, PulseAppId subapp);
PULSE_APP_API PulseAppId pulse_app_get_subapp(Const_PulseAppId _this, const char* name);
[[pulse::owner]] PULSE_APP_API PulseAppId pulse_app_remove_subapp(PulseAppId _this, const char* name);
PULSE_APP_API EPulseAppSetSubappExtractResult pulse_app_set_subapp_extract(PulseAppId _this, const char* name, PulseProcSubappExtractFn extract, [[pulse::optional]] [[pulse::retain]] void* ctx);
PULSE_APP_API PulseAppId pulse_get_app_from_world(ecs_world_t* world);

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
#endif // PULSE_APP_API_HEADER_GUARD
