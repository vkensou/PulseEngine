#pragma once

#ifndef PULSE_DATATABLE_API_HEADER_GUARD
#define PULSE_DATATABLE_API_HEADER_GUARD
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
#include <stdint.h>  // int32_t, int64_t, uint32_t, uint64_t
#include "pulse_platform.h"
#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_datalist.h"

#if defined(PULSE_DATATABLE_MODULE_BUILD)
#  define PULSE_DATATABLE_API PULSE_EXPORT
#else
#  define PULSE_DATATABLE_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

$cconsts

$cenums

$cids

// Forward declarations for types used by struct fields and function pointers
struct PulseDataTableColumnDesc;
typedef struct PulseDataTableColumnDesc PulseDataTableColumnDesc;

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
#endif // PULSE_DATATABLE_API_HEADER_GUARD
