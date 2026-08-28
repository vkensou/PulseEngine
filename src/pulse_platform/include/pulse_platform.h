#pragma once

#ifndef PULSE_PLATFORM_HEADER_GUARD
#define PULSE_PLATFORM_HEADER_GUARD

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Export/import qualifiers for shared libraries (Qt-style).
// Each plugin header picks one of them with its own build marker,
// e.g.:
//
//   #if defined(PULSE_WINDOW_MODULE_BUILD)
//   #  define PULSE_WINDOW_API PULSE_EXPORT
//   #else
//   #  define PULSE_WINDOW_API PULSE_IMPORT
//   #endif

#if defined(_MSC_VER)
#  define PULSE_EXPORT __declspec(dllexport)
#  define PULSE_IMPORT __declspec(dllimport)
#elif defined(__CYGWIN__)
#  define PULSE_EXPORT __attribute__((dllexport))
#  define PULSE_IMPORT __attribute__((dllimport))
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#  define PULSE_EXPORT __attribute__((visibility("default")))
#  define PULSE_IMPORT __attribute__((visibility("default")))
#else
#  define PULSE_EXPORT
#  define PULSE_IMPORT
#endif

#ifdef __cplusplus
}
#endif

#ifndef PULSE_FORCEINLINE
#if defined(_MSC_VER) && !defined(__clang__)
#define PULSE_FORCEINLINE __forceinline
#else
#define PULSE_FORCEINLINE inline __attribute__((always_inline))
#endif
#endif

#define DEFINE_PULSE_OBJECT(name) typedef struct name* name##Id; typedef const struct name* Const_##name##Id;

#define Pulse_Array(T, field) T* p_##field; size_t field##_count
#define Pulse_Array_Param(T, param) T* p_##param, size_t param##_count
#define Pulse_Blob(field) const void* p_##field; size_t field##_size
#define Pulse_Blob_Param(param) const void* p_##param, size_t param##_size

#endif
