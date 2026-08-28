#pragma once

#ifndef PULSE_VFS_API_HEADER_GUARD
#define PULSE_VFS_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint64_t
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_VFS_MODULE_BUILD)
#  define PULSE_VFS_API PULSE_EXPORT
#else
#  define PULSE_VFS_API PULSE_IMPORT
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

#endif // PULSE_VFS_API_HEADER_GUARD