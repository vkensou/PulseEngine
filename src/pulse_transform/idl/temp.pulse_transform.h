#pragma once

#ifndef PULSE_TRANSFORM_API_HEADER_GUARD
#define PULSE_TRANSFORM_API_HEADER_GUARD

#include <stdint.h>
#include "pulse_platform.h"
#include "pulse_app.h"

#include "pulse_math.h"

#if defined(PULSE_TRANSFORM_MODULE_BUILD)
#  define PULSE_TRANSFORM_API PULSE_EXPORT
#else
#  define PULSE_TRANSFORM_API PULSE_IMPORT
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

// ECS declarations
PULSE_TRANSFORM_API extern ECS_COMPONENT_DECLARE(PulseLocalTransform);
PULSE_TRANSFORM_API extern ECS_COMPONENT_DECLARE(PulseWorldTransform);
PULSE_TRANSFORM_API extern ECS_COMPONENT_DECLARE(PulseShowMatrix);

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_TRANSFORM_API_HEADER_GUARD
