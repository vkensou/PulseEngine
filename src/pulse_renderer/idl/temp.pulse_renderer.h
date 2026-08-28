#pragma once

#ifndef PULSE_RENDERER_API_HEADER_GUARD
#define PULSE_RENDERER_API_HEADER_GUARD
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

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
#endif // PULSE_RENDERER_API_HEADER_GUARD
