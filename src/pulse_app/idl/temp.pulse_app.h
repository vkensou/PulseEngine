#pragma once

#ifndef PULSE_APP_API_HEADER_GUARD
#define PULSE_APP_API_HEADER_GUARD
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

#define FLECS_NO_CPP
#include <flecs.h>

#if defined(PULSE_APP_MODULE_BUILD)
#  define PULSE_APP_API PULSE_EXPORT
#else
#  define PULSE_APP_API PULSE_IMPORT
#endif

$cconsts

typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;
typedef struct ecs_world_t ecs_world_t;

$cenums

$cflags

$cids

typedef struct PulsePluginDesc PulsePluginDesc;

$cfuncptrs

$cstructs

PULSE_APP_API extern ECS_COMPONENT_DECLARE(PulseTimer);

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
#endif // PULSE_APP_API_HEADER_GUARD
