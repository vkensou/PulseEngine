#pragma once

#ifndef PULSE_APP_API_HEADER_GUARD
#define PULSE_APP_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t

#define FLECS_NO_CPP
#include <flecs.h>

#ifndef PULSE_API
#define PULSE_API
#endif

#ifndef PULSE_FORCEINLINE
#if defined(_MSC_VER) && !defined(__clang__)
#define PULSE_FORCEINLINE __forceinline
#else
#define PULSE_FORCEINLINE inline __attribute__((always_inline))
#endif
#endif

#define PULSE_PLUGIN_DESC_VERSION 1u


#define DEFINE_PULSE_OBJECT(name) typedef struct name* name##Id;

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
    PULSE_RESULT_ERROR_INTERNAL,              /** ( 6)                                */

    PULSE_RESULT_COUNT

} EPulseResult;

typedef enum EPulseAppRunResult
{
    PULSE_APP_RUN_RESULT_OK,                  /** ( 0)                                */
    PULSE_APP_RUN_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_RUN_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_RUN_RESULT_ERROR_PLUGIN_BUILD_FAILED, /** ( 3)                                */
    PULSE_APP_RUN_RESULT_ERROR_PLUGIN_POST_BUILD_FAILED, /** ( 4)                                */
    PULSE_APP_RUN_RESULT_ERROR_SUBAPP_POST_BUILD_FAILED, /** ( 5)                                */
    PULSE_APP_RUN_RESULT_ERROR_SUBAPP_PREPARE_FAILED, /** ( 6)                                */
    PULSE_APP_RUN_RESULT_ERROR_SUBAPP_EXTRACT_FAILED, /** ( 7)                                */
    PULSE_APP_RUN_RESULT_ERROR_SUBAPP_UPDATE_FAILED, /** ( 8)                                */
    PULSE_APP_RUN_RESULT_ERROR_INTERNAL,      /** ( 9)                                */

    PULSE_APP_RUN_RESULT_COUNT

} EPulseAppRunResult;

typedef enum EPulseAppPrepareResult
{
    PULSE_APP_PREPARE_RESULT_OK,              /** ( 0)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_INVALID_ARGUMENT, /** ( 1)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_INVALID_STATE, /** ( 2)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_PLUGIN_BUILD_FAILED, /** ( 3)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_PLUGIN_POST_BUILD_FAILED, /** ( 4)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_SUBAPP_POST_BUILD_FAILED, /** ( 5)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_SUBAPP_PREPARE_FAILED, /** ( 6)                                */
    PULSE_APP_PREPARE_RESULT_ERROR_INTERNAL,  /** ( 7)                                */

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
    PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INTERNAL, /** ( 5)                                */

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

typedef EPulseRunnerResult (*PulseProcRunnerFn)(PulseAppId app, void* ctx);
typedef EPulseSubappExtractResult (*PulseProcSubappExtractFn)(PulseAppId app, PulseAppId subapp, void* ctx);
typedef EPulsePluginBuildResult (*PulseProcPluginBuildFn)(PulseAppId app, void* ctx);
typedef void (*PulseProcPluginShutdownFn)(PulseAppId app, void* ctx);

typedef struct PulseAppDesc
{
    const char*          name;
    bool                 enable_restapi;

} PulseAppDesc;

typedef struct PulsePluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    const char*          name;
    void*                ctx;
    PulseProcPluginBuildFn build;
    PulseProcPluginBuildFn post_build;
    PulseProcPluginShutdownFn shutdown;

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


extern ECS_COMPONENT_DECLARE(PulseTimer);

PULSE_API PulseAppId pulse_create_app(const PulseAppDesc* desc);
PULSE_API void pulse_destroy_app(PulseAppId app);
PULSE_API EPulseAppRunResult pulse_app_run(PulseAppId _this);
PULSE_API EPulseAppPrepareResult pulse_app_prepare(PulseAppId _this);
PULSE_API EPulseAppUpdateResult pulse_app_update(PulseAppId _this);
PULSE_API void pulse_app_teardown(PulseAppId _this);
PULSE_API void pulse_app_finish(PulseAppId _this);
PULSE_API bool pulse_app_should_quit(PulseAppId _this);
PULSE_API EPulseAppSetRunnerResult pulse_app_set_runner(PulseAppId _this, PulseProcRunnerFn runner, void* ctx);
PULSE_API EPulseAppAddPluginResult pulse_app_add_plugin(PulseAppId _this, const PulsePluginDesc* desc);
PULSE_API bool pulse_app_has_plugin(PulseAppId _this, const char* name);
PULSE_API ecs_world_t* pulse_app_world(PulseAppId _this);
PULSE_API const char* pulse_app_last_error(PulseAppId _this);
PULSE_API EPulseAppInsertSubappResult pulse_app_insert_subapp(PulseAppId _this, const char* name, PulseAppId subapp);
PULSE_API PulseAppId pulse_app_get_subapp(PulseAppId _this, const char* name);
PULSE_API PulseAppId pulse_app_remove_subapp(PulseAppId _this, const char* name);
PULSE_API EPulseAppSetSubappExtractResult pulse_app_set_subapp_extract(PulseAppId _this, const char* name, PulseProcSubappExtractFn extract, void* ctx);
PULSE_API PulseAppId pulse_get_app_from_world(ecs_world_t* world);

#ifdef __cplusplus
}
#endif

#endif // PULSE_APP_API_HEADER_GUARD
