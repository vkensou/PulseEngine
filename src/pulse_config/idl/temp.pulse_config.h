#pragma once

#ifndef PULSE_CONFIG_API_HEADER_GUARD
#define PULSE_CONFIG_API_HEADER_GUARD
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
#include <stdint.h>  // int64_t
#include "pulse_platform.h"

#if defined(PULSE_CONFIG_MODULE_BUILD)
#  define PULSE_CONFIG_API PULSE_EXPORT
#else
#  define PULSE_CONFIG_API PULSE_IMPORT
#endif

$cconsts

$cenums

$cflags

$cids

$cfuncptrs

$cstructs

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
#endif // PULSE_CONFIG_API_HEADER_GUARD
