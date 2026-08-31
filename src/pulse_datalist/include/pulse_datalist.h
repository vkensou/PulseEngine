#pragma once

#ifndef PULSE_DATALIST_API_HEADER_GUARD
#define PULSE_DATALIST_API_HEADER_GUARD
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

#if defined(PULSE_DATALIST_MODULE_BUILD)
#  define PULSE_DATALIST_API PULSE_EXPORT
#else
#  define PULSE_DATALIST_API PULSE_IMPORT
#endif



typedef enum EPulseDatalistType
{
    PULSE_DATALIST_TYPE_NIL,                  /** ( 0)                                */
    PULSE_DATALIST_TYPE_BOOL,                 /** ( 1)                                */
    PULSE_DATALIST_TYPE_INT,                  /** ( 2)                                */
    PULSE_DATALIST_TYPE_DOUBLE,               /** ( 3)                                */
    PULSE_DATALIST_TYPE_STRING,               /** ( 4)                                */
    PULSE_DATALIST_TYPE_LIST,                 /** ( 5)                                */
    PULSE_DATALIST_TYPE_MAP,                  /** ( 6)                                */
    PULSE_DATALIST_TYPE_MIXED,                /** ( 7)                                */

    PULSE_DATALIST_TYPE_COUNT

} EPulseDatalistType;








struct PulseDatalist;
typedef struct PulseDatalist PulseDatalist;


[[pulse::optional]] PULSE_DATALIST_API PulseDatalist* pulse_datalist_create_from_text(const char* text, size_t len);
[[pulse::optional]] PULSE_DATALIST_API PulseDatalist* pulse_datalist_create_from_text_list(const char* text, size_t len);
[[pulse::optional]] PULSE_DATALIST_API PulseDatalist* pulse_datalist_create_from_text_file(const char* path);
PULSE_DATALIST_API void pulse_datalist_addref(PulseDatalist* node);
PULSE_DATALIST_API void pulse_datalist_release([[pulse::owner]] PulseDatalist* node);
PULSE_DATALIST_API void pulse_datalist_free_string([[pulse::owner]] char* str);
PULSE_DATALIST_API EPulseDatalistType pulse_datalist_get_type(const PulseDatalist* node, [[pulse::optional]] const char* key);
PULSE_DATALIST_API bool pulse_datalist_has(const PulseDatalist* node, [[pulse::optional]] const char* key);
PULSE_DATALIST_API bool pulse_datalist_get_bool(const PulseDatalist* node, [[pulse::optional]] const char* key, bool default_value);
PULSE_DATALIST_API int64_t pulse_datalist_get_int(const PulseDatalist* node, [[pulse::optional]] const char* key, int64_t default_value);
PULSE_DATALIST_API double pulse_datalist_get_double(const PulseDatalist* node, [[pulse::optional]] const char* key, double default_value);
[[pulse::optional]] PULSE_DATALIST_API const char* pulse_datalist_get_string(const PulseDatalist* node, [[pulse::optional]] const char* key, [[pulse::optional]] const char* default_value);
[[pulse::optional]] PULSE_DATALIST_API PulseDatalist* pulse_datalist_get_obj(const PulseDatalist* node, [[pulse::optional]] const char* key);
PULSE_DATALIST_API size_t pulse_datalist_count(const PulseDatalist* node);
[[pulse::optional]] PULSE_DATALIST_API PulseDatalist* pulse_datalist_get(const PulseDatalist* node, size_t index);
PULSE_DATALIST_API size_t pulse_datalist_object_count(const PulseDatalist* node);
[[pulse::optional]] PULSE_DATALIST_API const char* pulse_datalist_object_key(const PulseDatalist* node, size_t index);
[[pulse::optional]] PULSE_DATALIST_API PulseDatalist* pulse_datalist_object_value(const PulseDatalist* node, size_t index);
[[pulse::optional]] PULSE_DATALIST_API char* pulse_datalist_to_text(const PulseDatalist* node, [[pulse::optional]] size_t* out_len);
[[pulse::optional]] PULSE_DATALIST_API char* pulse_datalist_quote(const char* str, size_t len);
[[pulse::optional]] PULSE_DATALIST_API const char* pulse_datalist_last_error(void);

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
#endif // PULSE_DATALIST_API_HEADER_GUARD