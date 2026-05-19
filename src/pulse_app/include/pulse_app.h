#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ecs_world_t ecs_world_t;
typedef struct pulse_app_o* pulse_app_t;

typedef enum pulse_result {
    PULSE_OK = 0,
    PULSE_ERROR_INVALID_ARGUMENT,
    PULSE_ERROR_INVALID_STATE,
    PULSE_ERROR_DUPLICATE_PLUGIN,
    PULSE_ERROR_DUPLICATE_SUBAPP,
    PULSE_ERROR_NOT_FOUND,
    PULSE_ERROR_INTERNAL,
} pulse_result_t;

typedef pulse_result_t (*pulse_runner_fn)(pulse_app_t app, void* ctx);

typedef pulse_result_t (*pulse_subapp_extract_fn)(
    pulse_app_t app,
    pulse_app_t subapp,
    void* ctx
);

#define PULSE_PLUGIN_DESC_VERSION 1u

typedef struct pulse_plugin_desc {
    uint32_t struct_size;
    uint32_t version;
    const char* name;
    void* ctx;

    pulse_result_t (*build)(pulse_app_t app, void* ctx);
    pulse_result_t (*post_build)(pulse_app_t app, void* ctx);
    void (*shutdown)(pulse_app_t app, void* ctx);
} pulse_plugin_desc;

pulse_app_t    pulse_app_create(void);
void           pulse_app_destroy(pulse_app_t app);

pulse_result_t pulse_app_run(pulse_app_t app);
pulse_result_t pulse_app_update(pulse_app_t app);

pulse_result_t pulse_app_set_runner(pulse_app_t app, pulse_runner_fn runner, void* ctx);

pulse_result_t pulse_app_add_plugin(pulse_app_t app, const pulse_plugin_desc* desc);
bool           pulse_app_has_plugin(pulse_app_t app, const char* name);

ecs_world_t*   pulse_app_world(pulse_app_t app);
const char*    pulse_app_last_error(pulse_app_t app);

pulse_app_t    pulse_subapp_create(const char* name);
pulse_result_t pulse_app_insert_subapp(pulse_app_t app, const char* name, pulse_app_t subapp);
pulse_app_t    pulse_app_get_subapp(pulse_app_t app, const char* name);
pulse_app_t    pulse_app_remove_subapp(pulse_app_t app, const char* name);

pulse_result_t pulse_app_set_subapp_extract(
    pulse_app_t app,
    const char* name,
    pulse_subapp_extract_fn extract,
    void* ctx
);

pulse_result_t pulse_app_extract_subapps(pulse_app_t app);

#ifdef __cplusplus
}
#endif
