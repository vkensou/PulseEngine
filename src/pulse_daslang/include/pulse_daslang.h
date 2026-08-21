#pragma once

#ifndef PULSE_DASLANG_API_HEADER_GUARD
#define PULSE_DASLANG_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "pulse_app.h"

#ifndef PULSE_API
#define PULSE_API
#endif

/**
 * Constants
 *
 */
#define PULSE_DASLANG_PLUGIN_DESC_VERSION 1u




/**
 * Plugin descriptor
 *
 */
typedef struct PulseDaslangPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    const char*          root_path;

} PulseDaslangPluginDesc;


/**
 * Functions
 *
 */
PULSE_API PulseDaslangPluginDesc pulse_daslang_plugin_desc_default(void);
PULSE_API EPulseResult pulse_add_daslang_plugin(PulseAppId app, const PulseDaslangPluginDesc* desc);
PULSE_API bool pulse_load_module(PulseAppId app, const char* script_path);

#ifdef __cplusplus
}
#endif

#endif // PULSE_DASLANG_API_HEADER_GUARD
