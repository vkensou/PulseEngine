#pragma once

#ifndef PULSE_INPUT_API_HEADER_GUARD
#define PULSE_INPUT_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // int32_t
#include "pulse_app.h"

#ifndef PULSE_API
#define PULSE_API
#endif

#define PULSE_SCANCODE_COUNT 512

#define PULSE_INPUT_PLUGIN_DESC_VERSION 1u










typedef struct PulseKeyboardInput
{
    bool                 pressed[512];
    bool                 just_pressed[512];
    bool                 just_released[512];

} PulseKeyboardInput;

typedef struct PulseMouseInput
{
    uint8_t              state;
    uint8_t              just_pressed;
    uint8_t              just_released;

} PulseMouseInput;

typedef struct PulseMouseMotion
{
    float                delta_x;
    float                delta_y;
    float                x;
    float                y;

} PulseMouseMotion;

typedef struct PulseMouseScroll
{
    float                x;
    float                y;

} PulseMouseScroll;

typedef struct PulseKeyEvent
{
    int32_t              scancode;
    int32_t              keycode;
    uint16_t             mod;
    bool                 pressed;
    bool                 repeat;
    ecs_entity_t         window;

} PulseKeyEvent;

typedef struct PulseMouseButtonEvent
{
    uint8_t              button;
    bool                 pressed;
    float                x;
    float                y;
    bool                 is_touch;
    ecs_entity_t         window;

} PulseMouseButtonEvent;

typedef struct PulseMouseScrollEvent
{
    float                x;
    float                y;
    bool                 is_touch;
    ecs_entity_t         window;

} PulseMouseScrollEvent;


// ECS declarations
extern ECS_COMPONENT_DECLARE(PulseKeyboardInput);
extern ECS_COMPONENT_DECLARE(PulseMouseInput);
extern ECS_COMPONENT_DECLARE(PulseMouseMotion);
extern ECS_COMPONENT_DECLARE(PulseMouseScroll);
extern ECS_COMPONENT_DECLARE(PulseKeyEvent);
extern ECS_COMPONENT_DECLARE(PulseMouseButtonEvent);
extern ECS_COMPONENT_DECLARE(PulseMouseScrollEvent);

PULSE_API EPulseAppAddPluginResult pulse_add_input_plugin(PulseAppId app);
PULSE_API bool pulse_input_is_key_down(PulseAppId app, int32_t scancode);
PULSE_API bool pulse_input_key_just_pressed(PulseAppId app, int32_t scancode);
PULSE_API bool pulse_input_key_just_released(PulseAppId app, int32_t scancode);
PULSE_API bool pulse_input_is_mouse_button_down(PulseAppId app, uint8_t button);
PULSE_API bool pulse_input_mouse_button_just_pressed(PulseAppId app, uint8_t button);
PULSE_API bool pulse_input_mouse_button_just_released(PulseAppId app, uint8_t button);
PULSE_API void pulse_input_get_mouse_position(PulseAppId app, float* out_x, float* out_y);
PULSE_API void pulse_input_get_mouse_delta(PulseAppId app, float* out_dx, float* out_dy);
PULSE_API void pulse_input_get_mouse_scroll(PulseAppId app, float* out_x, float* out_y);

#ifdef __cplusplus
}
#endif

#endif // PULSE_INPUT_API_HEADER_GUARD
