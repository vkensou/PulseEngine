#pragma once

#include <stdint.h>

#include "pulse_app.h"
#include "pulse_config.h"

typedef EPulseResult (*PulseProcScriptPackageLoadFn)(PulseAppId app, const char* script_file);

typedef EPulseResult (*PulseProcScriptLoadFn)(PulseAppId app, const char* path, const char* text, uint64_t text_size);

typedef struct PulseScriptRuntimeDesc {
    const char* type;
    const char* extensions;
    PulseProcScriptLoadFn load;
    PulseProcScriptPackageLoadFn load_package;
} PulseScriptRuntimeDesc;

#define PULSE_PACKAGE_GET_RUNTIMES_SYMBOL "pulse_package_get_runtimes"

typedef uint32_t (*PulseProcPackageGetRuntimesFn)(const PulseScriptRuntimeDesc** out_runtimes);
