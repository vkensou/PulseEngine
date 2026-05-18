#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef uint64_t pulse_app_t;

typedef struct pulse_plugin_desc {
    void*        ctx;
    void       (*build)  (pulse_app_t app, void* ctx);
    bool       (*ready)  (pulse_app_t app, void* ctx);
    void       (*finish) (pulse_app_t app, void* ctx);
    void       (*cleanup)(pulse_app_t app, void* ctx);
    const char*  name;
    bool         unique;
    const char** dependencies;
    int          dependency_count;
} pulse_plugin_desc;

typedef void (*pulse_runner_fn)(pulse_app_t app, void* ctx);

pulse_app_t  pulse_app_create(void);
void         pulse_app_destroy(pulse_app_t app);
void         pulse_app_run(pulse_app_t app);
void         pulse_app_update(pulse_app_t app);
void         pulse_app_set_runner(pulse_app_t app, pulse_runner_fn runner, void* ctx);

void         pulse_app_add_plugin(pulse_app_t app, const pulse_plugin_desc* desc);

void         pulse_app_add_system(pulse_app_t app, ecs_entity_t phase, ecs_system_desc_t* desc);
void         pulse_app_insert_resource(pulse_app_t app, ecs_entity_t component, size_t size, const void* data);
void         pulse_app_init_resource(pulse_app_t app, ecs_entity_t component, size_t size, void (*ctor)(void* out));
ecs_world_t* pulse_app_world(pulse_app_t app);

pulse_app_t  pulse_subapp_create(const char* name);
void         pulse_subapp_destroy(pulse_app_t subapp);
void         pulse_app_insert_subapp(pulse_app_t app, const char* name, pulse_app_t subapp);
pulse_app_t  pulse_app_get_subapp(pulse_app_t app, const char* name);
void         pulse_app_remove_subapp(pulse_app_t app, const char* name);

bool         pulse_app_is_plugin_added(pulse_app_t app, const char* name);
int          pulse_app_plugins_state(pulse_app_t app);
