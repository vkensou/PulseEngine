#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "flecs.h"
#include "pulse_app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SDL_Window SDL_Window;

#define PULSE_WINDOW_PLUGIN_DESC_VERSION 1u

typedef enum pulse_window_flags {
    PULSE_WINDOW_FLAG_RESIZABLE = 1u << 0,
    PULSE_WINDOW_FLAG_EXTERNAL_GRAPHICS_CONTEXT = 1u << 1,
} pulse_window_flags_t;

typedef enum pulse_window_plugin_flags {
    PULSE_WINDOW_PLUGIN_CREATE_PRIMARY = 1u << 0,
    PULSE_WINDOW_PLUGIN_INSTALL_RUNNER = 1u << 1,
    PULSE_WINDOW_PLUGIN_EXIT_ON_PRIMARY_CLOSE = 1u << 2,
} pulse_window_plugin_flags_t;

#define PULSE_WINDOW_DEFAULT_FLAGS \
    (PULSE_WINDOW_FLAG_RESIZABLE | PULSE_WINDOW_FLAG_EXTERNAL_GRAPHICS_CONTEXT)

#define PULSE_WINDOW_PLUGIN_DEFAULT_FLAGS \
    (PULSE_WINDOW_PLUGIN_CREATE_PRIMARY | \
     PULSE_WINDOW_PLUGIN_INSTALL_RUNNER | \
     PULSE_WINDOW_PLUGIN_EXIT_ON_PRIMARY_CLOSE)

typedef struct pulse_window_desc {
    uint32_t struct_size;
    const char* title;
    int32_t width;
    int32_t height;
    uint32_t flags;
    bool primary;
} pulse_window_desc;

typedef struct pulse_window_plugin_desc {
    uint32_t struct_size;
    uint32_t version;
    pulse_window_desc primary_window;
    uint32_t flags;
    uint32_t sdl_init_flags;
} pulse_window_plugin_desc;

typedef struct pulse_window {
    int32_t width;
    int32_t height;
    bool resized;
    bool close_requested;
} pulse_window;

typedef struct pulse_sdl_window {
    SDL_Window* handle;
    uint32_t window_id;
    void* native_view;
} pulse_sdl_window;

extern ECS_COMPONENT_DECLARE(pulse_window);
extern ECS_COMPONENT_DECLARE(pulse_sdl_window);
extern ECS_TAG_DECLARE(PulsePrimaryWindow);

pulse_window_desc        pulse_window_desc_default(void);
pulse_window_plugin_desc pulse_window_plugin_desc_default(void);

pulse_result_t          pulse_window_add_plugin(
    pulse_app_t app,
    const pulse_window_plugin_desc* desc
);

pulse_result_t pulse_window_create(
    pulse_app_t app,
    const pulse_window_desc* desc,
    ecs_entity_t* out_entity
);

pulse_result_t pulse_window_destroy(pulse_app_t app, ecs_entity_t entity);
pulse_result_t pulse_window_poll_events(pulse_app_t app);
pulse_result_t pulse_window_sync(pulse_app_t app);
pulse_result_t pulse_window_request_close(pulse_app_t app);

bool         pulse_window_should_close(pulse_app_t app);
ecs_entity_t pulse_window_primary(pulse_app_t app);
SDL_Window*  pulse_window_get_sdl_window(pulse_app_t app, ecs_entity_t entity);
void*        pulse_window_get_native_view(pulse_app_t app, ecs_entity_t entity);

#ifdef __cplusplus
}
#endif
