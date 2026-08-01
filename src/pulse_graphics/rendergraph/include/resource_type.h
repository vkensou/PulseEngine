#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t pulse_index_t;

#define PULSE_MAX_INDEX (UINT32_MAX - 2)

typedef struct pulse_texture_handle_t {
    pulse_index_t index;
} pulse_texture_handle_t;

typedef struct pulse_buffer_handle_t {
    pulse_index_t index;
} pulse_buffer_handle_t;

#ifdef __cplusplus
}
#endif
