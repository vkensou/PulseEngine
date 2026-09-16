#pragma once

#ifndef PULSE_MATH_API_HEADER_GUARD
#define PULSE_MATH_API_HEADER_GUARD
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

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_MATH_MODULE_BUILD)
#  define PULSE_MATH_API PULSE_EXPORT
#else
#  define PULSE_MATH_API PULSE_IMPORT
#endif

// Use radians as the default angle unit
#define HANDMADE_MATH_USE_RADIANS

#include "HandmadeMath.h"

#ifdef __cplusplus
extern "C" {
#endif

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

$ccomponents

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
#endif // PULSE_MATH_API_HEADER_GUARD
