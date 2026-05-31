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




DEFINE_PULSE_OBJECT(PulseApp)

typedef struct PulsePluginDesc PulsePluginDesc;

typedef EPulseResult (*PulseProcRunnerFn)(PulseAppId app, void* ctx);
typedef EPulseResult (*PulseProcSubappExtractFn)(PulseAppId app, PulseAppId subapp, void* ctx);
typedef EPulseResult (*PulseProcPluginBuildFn)(PulseAppId app, void* ctx);
typedef void (*PulseProcPluginShutdownFn)(PulseAppId app, void* ctx);

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


PULSE_API PulseAppId pulse_create_app(const char* name);
PULSE_API void pulse_destroy_app(PulseAppId app);
PULSE_API EPulseResult pulse_app_run(PulseAppId _this);
PULSE_API EPulseResult pulse_app_update(PulseAppId _this);
PULSE_API EPulseResult pulse_app_set_runner(PulseAppId _this, PulseProcRunnerFn runner, void* ctx);
PULSE_API EPulseResult pulse_app_add_plugin(PulseAppId _this, const PulsePluginDesc* desc);
PULSE_API bool pulse_app_has_plugin(PulseAppId _this, const char* name);
PULSE_API ecs_world_t* pulse_app_world(PulseAppId _this);
PULSE_API const char* pulse_app_last_error(PulseAppId _this);
PULSE_API EPulseResult pulse_app_insert_subapp(PulseAppId _this, const char* name, PulseAppId subapp);
PULSE_API PulseAppId pulse_app_get_subapp(PulseAppId _this, const char* name);
PULSE_API PulseAppId pulse_app_remove_subapp(PulseAppId _this, const char* name);
PULSE_API EPulseResult pulse_app_set_subapp_extract(PulseAppId _this, const char* name, PulseProcSubappExtractFn extract, void* ctx);
PULSE_API EPulseResult pulse_app_extract_subapps(PulseAppId _this);

#ifdef __cplusplus
}
#endif

#endif // PULSE_APP_API_HEADER_GUARD
