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

#ifdef __cplusplus
extern "C" {
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

#define PULSE_SCANCODE_COUNT 512

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

// ECS declarations
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseKeyboardInput);
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseInput);
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseMotion);
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseScroll);
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseKeyEvent);
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseButtonEvent);
PULSE_INPUT_API extern ECS_COMPONENT_DECLARE(PulseMouseScrollEvent);

$c99decl

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
