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



typedef enum EPulseConfigType
{
    PULSE_CONFIG_TYPE_NONE,                   /** ( 0)                                */
    PULSE_CONFIG_TYPE_BOOL,                   /** ( 1)                                */
    PULSE_CONFIG_TYPE_INT,                    /** ( 2)                                */
    PULSE_CONFIG_TYPE_DOUBLE,                 /** ( 3)                                */
    PULSE_CONFIG_TYPE_STRING,                 /** ( 4)                                */
    PULSE_CONFIG_TYPE_OBJECT,                 /** ( 5)                                */
    PULSE_CONFIG_TYPE_ARRAY,                  /** ( 6)                                */

    PULSE_CONFIG_TYPE_COUNT

} EPulseConfigType;








/**
 * Opaque config tree handles
 *
 */
struct PulseConfig;
typedef struct PulseConfig PulseConfig;

struct PulseConfigArray;
typedef struct PulseConfigArray PulseConfigArray;


PULSE_CONFIG_API PulseConfig* pulse_config_create(void);
PULSE_CONFIG_API PulseConfig* pulse_config_create_from_json(const char* json, size_t len);
PULSE_CONFIG_API PulseConfig* pulse_config_create_from_json_file(const char* path);
PULSE_CONFIG_API void pulse_config_addref(PulseConfig* cfg);
PULSE_CONFIG_API void pulse_config_release(PulseConfig* cfg);
PULSE_CONFIG_API void pulse_config_free_string(char* str);
PULSE_CONFIG_API bool pulse_config_has(const PulseConfig* cfg, [[pulse::optional]] const char* key);
PULSE_CONFIG_API EPulseConfigType pulse_config_get_type(const PulseConfig* cfg, [[pulse::optional]] const char* key);
PULSE_CONFIG_API bool pulse_config_get_bool(const PulseConfig* cfg, [[pulse::optional]] const char* key, bool default_value);
PULSE_CONFIG_API int64_t pulse_config_get_int(const PulseConfig* cfg, [[pulse::optional]] const char* key, int64_t default_value);
PULSE_CONFIG_API double pulse_config_get_double(const PulseConfig* cfg, [[pulse::optional]] const char* key, double default_value);
PULSE_CONFIG_API [[pulse::optional]] const char* pulse_config_get_string(const PulseConfig* cfg, [[pulse::optional]] const char* key, [[pulse::optional]] const char* default_value);
PULSE_CONFIG_API [[pulse::optional]] PulseConfig* pulse_config_get_obj(const PulseConfig* cfg, [[pulse::optional]] const char* key);
PULSE_CONFIG_API [[pulse::optional]] PulseConfigArray* pulse_config_get_array(const PulseConfig* cfg, [[pulse::optional]] const char* key);
PULSE_CONFIG_API size_t pulse_config_array_count(const PulseConfigArray* arr);
PULSE_CONFIG_API [[pulse::optional]] PulseConfig* pulse_config_array_get(const PulseConfigArray* arr, size_t index);
PULSE_CONFIG_API void pulse_config_set_bool(PulseConfig* cfg, const char* key, bool value);
PULSE_CONFIG_API void pulse_config_set_int(PulseConfig* cfg, const char* key, int64_t value);
PULSE_CONFIG_API void pulse_config_set_double(PulseConfig* cfg, const char* key, double value);
PULSE_CONFIG_API void pulse_config_set_string(PulseConfig* cfg, const char* key, [[pulse::optional]] const char* value);
PULSE_CONFIG_API void pulse_config_set_obj(PulseConfig* cfg, const char* key, [[pulse::optional]] PulseConfig* value);
PULSE_CONFIG_API void pulse_config_set_array(PulseConfig* cfg, const char* key, [[pulse::optional]] PulseConfigArray* value);
PULSE_CONFIG_API bool pulse_config_remove(PulseConfig* cfg, [[pulse::optional]] const char* key);
PULSE_CONFIG_API [[pulse::optional]] char* pulse_config_to_json(const PulseConfig* cfg, [[pulse::optional]] size_t* out_len);
PULSE_CONFIG_API [[pulse::optional]] char* pulse_config_to_json_pretty(const PulseConfig* cfg, [[pulse::optional]] size_t* out_len);
PULSE_CONFIG_API [[pulse::optional]] PulseConfig* pulse_config_merge([[pulse::optional]] const PulseConfig* defaults, [[pulse::optional]] const PulseConfig* overrides);
PULSE_CONFIG_API [[pulse::optional]] const char* pulse_config_last_error(void);

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
