#pragma once

#ifndef PULSE_DASLANG_API_HEADER_GUARD
#define PULSE_DASLANG_API_HEADER_GUARD
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

#include <stdbool.h>
#include <stdint.h>
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_DASLANG_MODULE_BUILD)
#  define PULSE_DASLANG_API PULSE_EXPORT
#else
#  define PULSE_DASLANG_API PULSE_IMPORT
#endif

$cconsts
$cenums
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
#endif // PULSE_DASLANG_API_HEADER_GUARD
