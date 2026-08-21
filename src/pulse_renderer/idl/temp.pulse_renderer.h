#pragma once

#ifndef PULSE_RENDERER_API_HEADER_GUARD
#define PULSE_RENDERER_API_HEADER_GUARD

#include <stdint.h>
#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_graphics.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef PULSE_API
#define PULSE_API
#endif

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

// ECS component declarations
extern ECS_COMPONENT_DECLARE(PulseCamera);
extern ECS_COMPONENT_DECLARE(PulseLight);
extern ECS_COMPONENT_DECLARE(PulseRenderable);

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_RENDERER_API_HEADER_GUARD
