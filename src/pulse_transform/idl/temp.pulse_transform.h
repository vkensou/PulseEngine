#pragma once

#ifndef PULSE_TRANSFORM_API_HEADER_GUARD
#define PULSE_TRANSFORM_API_HEADER_GUARD

#include <stdint.h>
#include "pulse_app.h"

#include "pulse_math.h"

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

// ECS declarations
extern ECS_COMPONENT_DECLARE(PulseLocalTransform);
extern ECS_COMPONENT_DECLARE(PulseWorldTransform);
extern ECS_COMPONENT_DECLARE(PulseShowMatrix);

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_TRANSFORM_API_HEADER_GUARD
