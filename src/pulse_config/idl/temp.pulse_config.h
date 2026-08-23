#pragma once

#ifndef PULSE_CONFIG_API_HEADER_GUARD
#define PULSE_CONFIG_API_HEADER_GUARD

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

#endif // PULSE_CONFIG_API_HEADER_GUARD
