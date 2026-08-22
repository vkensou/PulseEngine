#pragma once

#ifndef PULSE_APP_API_HEADER_GUARD
#define PULSE_APP_API_HEADER_GUARD

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

#endif // PULSE_APP_API_HEADER_GUARD
