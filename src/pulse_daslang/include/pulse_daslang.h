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

/**
 * Constants
 *
 */
#define PULSE_DASLANG_PLUGIN_DESC_VERSION 2u




/**
 * Plugin descriptor
 *
 */
typedef struct PulseDaslangPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;

} PulseDaslangPluginDesc;


/**
 * Functions
 *
 */
PULSE_DASLANG_API PulseDaslangPluginDesc pulse_daslang_plugin_desc_default(void);
PULSE_DASLANG_API EPulseAppAddPluginResult pulse_add_daslang_plugin(PulseAppId app, const PulseDaslangPluginDesc* desc);
PULSE_DASLANG_API bool pulse_load_module(PulseAppId app, const char* script_path);

#ifdef __cplusplus
}
#endif

#endif // PULSE_DASLANG_API_HEADER_GUARD
