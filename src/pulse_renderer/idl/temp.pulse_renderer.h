#pragma once

#ifndef PULSE_RENDERER_API_HEADER_GUARD
#define PULSE_RENDERER_API_HEADER_GUARD

#include <stdint.h>
#include "pulse_platform.h"
#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_graphics.h"

#if defined(PULSE_RENDERER_MODULE_BUILD)
#  define PULSE_RENDERER_API PULSE_EXPORT
#else
#  define PULSE_RENDERER_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

// ECS component declarations
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseCamera);
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseLight);
PULSE_RENDERER_API extern ECS_COMPONENT_DECLARE(PulseRenderable);

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_RENDERER_API_HEADER_GUARD
