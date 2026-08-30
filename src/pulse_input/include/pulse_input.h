#pragma once

#ifndef PULSE_INPUT_API_HEADER_GUARD
#define PULSE_INPUT_API_HEADER_GUARD
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
#include <stdint.h>  // int32_t
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_INPUT_MODULE_BUILD)
#  define PULSE_INPUT_API PULSE_EXPORT
#else
#  define PULSE_INPUT_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_SCANCODE_COUNT 512

#define PULSE_INPUT_PLUGIN_DESC_VERSION 1u












typedef struct PulseKeyboardInput
{
    bool                 pressed[512];
    bool                 just_pressed[512];
    bool                 just_released[512];

} PulseKeyboardInput;
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseKeyboardInput);

typedef struct PulseMouseInput
{
    uint8_t              state;
    uint8_t              just_pressed;
    uint8_t              just_released;

} PulseMouseInput;
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseInput);

typedef struct PulseMouseMotion
{
    float                delta_x;
    float                delta_y;
    float                x;
    float                y;

} PulseMouseMotion;
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseMotion);

typedef struct PulseMouseScroll
{
    float                x;
    float                y;

} PulseMouseScroll;
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseScroll);

typedef struct PulseKeyEvent
{
    int32_t              scancode;
    int32_t              keycode;
    uint16_t             mod;
    bool                 pressed;
    bool                 repeat;
    ecs_entity_t         window;

} PulseKeyEvent;
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseKeyEvent);

typedef struct PulseMouseButtonEvent
{
    uint8_t              button;
    bool                 pressed;
    float                x;
    float                y;
    bool                 is_touch;
    ecs_entity_t         window;

} PulseMouseButtonEvent;
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseButtonEvent);

typedef struct PulseMouseScrollEvent
{
    float                x;
    float                y;
    bool                 is_touch;
    ecs_entity_t         window;

} PulseMouseScrollEvent;
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseScrollEvent);

typedef struct PulseMouseMotionEvent
{
    float                x;
    float                y;
    bool                 is_touch;
    ecs_entity_t         window;

} PulseMouseMotionEvent;
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseMotionEvent);


PULSE_INPUT_API EPulseAppAddPluginResult pulse_add_input_plugin(PulseAppId app);
PULSE_INPUT_API bool pulse_input_is_key_down(PulseAppId app, int32_t scancode);
PULSE_INPUT_API bool pulse_input_key_just_pressed(PulseAppId app, int32_t scancode);
PULSE_INPUT_API bool pulse_input_key_just_released(PulseAppId app, int32_t scancode);
PULSE_INPUT_API bool pulse_input_is_mouse_button_down(PulseAppId app, uint8_t button);
PULSE_INPUT_API bool pulse_input_mouse_button_just_pressed(PulseAppId app, uint8_t button);
PULSE_INPUT_API bool pulse_input_mouse_button_just_released(PulseAppId app, uint8_t button);
PULSE_INPUT_API void pulse_input_get_mouse_position(PulseAppId app, [[pulse::out]] float* out_x, [[pulse::out]] float* out_y);
PULSE_INPUT_API void pulse_input_get_mouse_delta(PulseAppId app, [[pulse::out]] float* out_dx, [[pulse::out]] float* out_dy);
PULSE_INPUT_API void pulse_input_get_mouse_scroll(PulseAppId app, [[pulse::out]] float* out_x, [[pulse::out]] float* out_y);

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
#endif // PULSE_INPUT_API_HEADER_GUARD
