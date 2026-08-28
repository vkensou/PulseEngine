#pragma once

#ifndef PULSE_DASLANG_API_HEADER_GUARD
#define PULSE_DASLANG_API_HEADER_GUARD

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

#endif // PULSE_DASLANG_API_HEADER_GUARD
