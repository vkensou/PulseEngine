#pragma once

#ifndef PULSE_APP_API_HEADER_GUARD
#define PULSE_APP_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t

#define FLECS_NO_CPP
#include <flecs.h>

#ifndef PULSE_API
#define PULSE_API
#endif

$cconsts

#define DEFINE_PULSE_OBJECT(name) typedef struct name* name##Id;

typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;
typedef struct ecs_world_t ecs_world_t;

$cenums

$cflags

$cids

typedef struct PulsePluginDesc PulsePluginDesc;

$cfuncptrs

$cstructs

$c99decl

#ifdef __cplusplus
}
#endif

#endif // PULSE_APP_API_HEADER_GUARD
