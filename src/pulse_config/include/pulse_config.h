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
PULSE_CONFIG_API bool pulse_config_has(const PulseConfig* cfg, const char* key);
PULSE_CONFIG_API EPulseConfigType pulse_config_get_type(const PulseConfig* cfg, const char* key);
PULSE_CONFIG_API bool pulse_config_get_bool(const PulseConfig* cfg, const char* key, bool default_value);
PULSE_CONFIG_API int64_t pulse_config_get_int(const PulseConfig* cfg, const char* key, int64_t default_value);
PULSE_CONFIG_API double pulse_config_get_double(const PulseConfig* cfg, const char* key, double default_value);
PULSE_CONFIG_API const char* pulse_config_get_string(const PulseConfig* cfg, const char* key, const char* default_value);
PULSE_CONFIG_API PulseConfig* pulse_config_get_obj(const PulseConfig* cfg, const char* key);
PULSE_CONFIG_API PulseConfigArray* pulse_config_get_array(const PulseConfig* cfg, const char* key);
PULSE_CONFIG_API size_t pulse_config_array_count(const PulseConfigArray* arr);
PULSE_CONFIG_API PulseConfig* pulse_config_array_get(const PulseConfigArray* arr, size_t index);
PULSE_CONFIG_API void pulse_config_set_bool(PulseConfig* cfg, const char* key, bool value);
PULSE_CONFIG_API void pulse_config_set_int(PulseConfig* cfg, const char* key, int64_t value);
PULSE_CONFIG_API void pulse_config_set_double(PulseConfig* cfg, const char* key, double value);
PULSE_CONFIG_API void pulse_config_set_string(PulseConfig* cfg, const char* key, const char* value);
PULSE_CONFIG_API void pulse_config_set_obj(PulseConfig* cfg, const char* key, PulseConfig* value);
PULSE_CONFIG_API void pulse_config_set_array(PulseConfig* cfg, const char* key, PulseConfigArray* value);
PULSE_CONFIG_API bool pulse_config_remove(PulseConfig* cfg, const char* key);
PULSE_CONFIG_API char* pulse_config_to_json(const PulseConfig* cfg, size_t* out_len);
PULSE_CONFIG_API char* pulse_config_to_json_pretty(const PulseConfig* cfg, size_t* out_len);
PULSE_CONFIG_API PulseConfig* pulse_config_merge(const PulseConfig* defaults, const PulseConfig* overrides);
PULSE_CONFIG_API const char* pulse_config_last_error(void);

#ifdef __cplusplus
}
#endif

#endif // PULSE_CONFIG_API_HEADER_GUARD
