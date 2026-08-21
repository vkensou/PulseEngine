#pragma once

#ifndef PULSE_WINDOW_API_HEADER_GUARD
#define PULSE_WINDOW_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t
#include "pulse_app.h"

#ifndef PULSE_API
#define PULSE_API
#endif

#define PULSE_WINDOW_PLUGIN_DESC_VERSION 1u


typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;
typedef struct SDL_Window SDL_Window;



/**
 * Window plugin flag bits (PulseWindowPluginFlags = EPulseFlags)
 *
 */
typedef enum EPulseWindowPluginFlagBits
{
    PULSE_WINDOW_PLUGIN_CREATE_PRIMARY = 0x00000001,       /** ( 0)  //!< Create primary window on build */
    PULSE_WINDOW_PLUGIN_INSTALL_RUNNER = 0x00000002,       /** ( 1)  //!< Install window event runner */
    PULSE_WINDOW_PLUGIN_EXIT_ON_PRIMARY_CLOSE = 0x00000004, /** ( 2)  //!< Exit app when primary window closes */
    PULSE_WINDOW_PLUGIN_DEFAULT = PULSE_WINDOW_PLUGIN_CREATE_PRIMARY | PULSE_WINDOW_PLUGIN_INSTALL_RUNNER | PULSE_WINDOW_PLUGIN_EXIT_ON_PRIMARY_CLOSE,

} EPulseWindowPluginFlagBits;
typedef EPulseFlags EPulseWindowPluginFlags;






typedef struct PulseWindowDesc
{
    uint32_t             struct_size;
    const char*          title;
    int32_t              width;
    int32_t              height;
    bool                 resizable;
    bool                 external_graphics_context;

} PulseWindowDesc;

typedef struct PulseWindowPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    PulseWindowDesc      primary_window;
    EPulseWindowPluginFlags flags;
    uint32_t             sdl_init_flags;

} PulseWindowPluginDesc;

typedef struct PulseWindow
{
    uint32_t             struct_size;
    const char*          title;
    int32_t              width;
    int32_t              height;
    bool                 resizable;
    bool                 external_graphics_context;

} PulseWindow;

typedef struct PulseSdlWindow
{
    SDL_Window*          handle;
    uint32_t             window_id;
    void*                native_view;

} PulseSdlWindow;

typedef struct PulseTextInputEvent
{
    char                 text[512];
    ecs_entity_t         window;

} PulseTextInputEvent;

typedef struct PulseWindowFocusEvent
{
    bool                 focused;
    ecs_entity_t         window;

} PulseWindowFocusEvent;

typedef struct PulseWindowMouseHoverEvent
{
    bool                 entered;
    ecs_entity_t         window;

} PulseWindowMouseHoverEvent;


// ECS declarations
extern ECS_COMPONENT_DECLARE(PulseWindow);
extern ECS_COMPONENT_DECLARE(PulseSdlWindow);
extern ECS_TAG_DECLARE(PulsePrimaryWindow);
extern ECS_TAG_DECLARE(PulseWindowCloseRequested);
extern ECS_TAG_DECLARE(PulseWindowResized);
extern ECS_COMPONENT_DECLARE(PulseTextInputEvent);
extern ECS_COMPONENT_DECLARE(PulseWindowFocusEvent);
extern ECS_COMPONENT_DECLARE(PulseWindowMouseHoverEvent);

struct PulsePrimaryWindow{};

PULSE_API PulseWindowDesc pulse_window_desc_default(void);
PULSE_API PulseWindowPluginDesc pulse_window_plugin_desc_default(void);
PULSE_API EPulseResult pulse_add_window_plugin(PulseAppId app, const PulseWindowPluginDesc* desc);
PULSE_API ecs_entity_t pulse_window_get_primary(PulseAppId app);
PULSE_API EPulseResult pulse_window_set_title(PulseAppId app, ecs_entity_t entity, const char* title);
PULSE_API void* pulse_window_get_native_view(PulseAppId app, ecs_entity_t entity);

#ifdef __cplusplus
}
#endif

#endif // PULSE_WINDOW_API_HEADER_GUARD
