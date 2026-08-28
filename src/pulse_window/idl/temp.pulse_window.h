#pragma once

#ifndef PULSE_WINDOW_API_HEADER_GUARD
#define PULSE_WINDOW_API_HEADER_GUARD
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
#include <stdint.h>  // uint32_t
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_WINDOW_MODULE_BUILD)
#  define PULSE_WINDOW_API PULSE_EXPORT
#else
#  define PULSE_WINDOW_API PULSE_IMPORT
#endif

$cconsts

typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;
typedef struct SDL_Window SDL_Window;

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

// ECS declarations
PULSE_WINDOW_API extern ECS_COMPONENT_DECLARE(PulseWindow);
PULSE_WINDOW_API extern ECS_COMPONENT_DECLARE(PulseSdlWindow);
PULSE_WINDOW_API extern ECS_TAG_DECLARE(PulsePrimaryWindowEntity);
PULSE_WINDOW_API extern ECS_TAG_DECLARE(PulseWindowCloseRequested);
PULSE_WINDOW_API extern ECS_TAG_DECLARE(PulseWindowResized);
PULSE_WINDOW_API extern ECS_COMPONENT_DECLARE(PulseTextInputEvent);
PULSE_WINDOW_API extern ECS_COMPONENT_DECLARE(PulseWindowFocusEvent);
PULSE_WINDOW_API extern ECS_COMPONENT_DECLARE(PulseWindowMouseHoverEvent);

struct PulsePrimaryWindow{};

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
#endif // PULSE_WINDOW_API_HEADER_GUARD
