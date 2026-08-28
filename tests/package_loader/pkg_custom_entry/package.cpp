// Test-only package library for tests/package_loader/test_loader_custom_entry.cpp.
//
// Exercises the package.json "entry" feature of the package loader: the entry
// symbol picks a custom DLL export as the register entry point instead of the
// default "pulse_package_register".
#include "pulse_app.h"
#include "pulse_config.h"

extern "C" PULSE_EXPORT EPulseResult custom_package_register(PulseAppId app, PulseConfig* config) {
    (void)config;
    PulsePluginDesc desc = {};
    desc.struct_size = sizeof(PulsePluginDesc);
    desc.version = PULSE_PLUGIN_DESC_VERSION;
    desc.plugin_version = 1;
    desc.name = "CustomEntryPlugin";
    EPulseAppAddPluginResult r = pulse_app_add_plugin(app, &desc);
    return r == PULSE_APP_ADD_PLUGIN_RESULT_OK ? PULSE_RESULT_OK : PULSE_RESULT_ERROR_INTERNAL;
}