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

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

// ECS declarations
extern ECS_COMPONENT_DECLARE(PulseKeyboardInput);
extern ECS_COMPONENT_DECLARE(PulseMouseInput);
extern ECS_COMPONENT_DECLARE(PulseMouseMotion);
extern ECS_COMPONENT_DECLARE(PulseMouseScroll);
extern ECS_COMPONENT_DECLARE(PulseKeyEvent);
extern ECS_COMPONENT_DECLARE(PulseMouseButtonEvent);

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_INPUT_API_HEADER_GUARD
