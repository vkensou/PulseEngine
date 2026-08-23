#pragma once

#ifndef PULSE_PACKAGE_LOADER_API_HEADER_GUARD
#define PULSE_PACKAGE_LOADER_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t
#include "pulse_platform.h"
#include "pulse_app.h"
#include "pulse_config.h"

#if defined(PULSE_PACKAGE_LOADER_MODULE_BUILD)
#  define PULSE_PACKAGE_LOADER_API PULSE_EXPORT
#else
#  define PULSE_PACKAGE_LOADER_API PULSE_IMPORT
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

#endif // PULSE_PACKAGE_LOADER_API_HEADER_GUARD