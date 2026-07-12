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
extern ECS_COMPONENT_DECLARE(PulseWindow);
extern ECS_COMPONENT_DECLARE(PulseSdlWindow);
extern ECS_TAG_DECLARE(PulsePrimaryWindow);
extern ECS_TAG_DECLARE(PulseWindowCloseRequested);
extern ECS_TAG_DECLARE(PulseWindowResized);

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_WINDOW_API_HEADER_GUARD
