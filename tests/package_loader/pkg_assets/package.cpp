// Test-only package library for tests/package_loader/test_loader_entry_and_assets.cpp.
//
// Exercises the package.json "assets": true feature of the package loader: the
// package directory is registered as a pulse_vfs content root, so files next to
// the package can be resolved by relative path.
#include "pulse_app.h"
#include "pulse_config.h"

extern "C" PULSE_EXPORT EPulseResult pulse_package_register(PulseAppId app, PulseConfig* config) {
    (void)config;
    PulsePluginDesc desc = {};
    desc.struct_size = sizeof(PulsePluginDesc);
    desc.version = PULSE_PLUGIN_DESC_VERSION;
    desc.plugin_version = 1;
    desc.name = "AssetsEntryPlugin";
    EPulseAppAddPluginResult r = pulse_app_add_plugin(app, &desc);
    return r == PULSE_APP_ADD_PLUGIN_RESULT_OK ? PULSE_RESULT_OK : PULSE_RESULT_ERROR_INTERNAL;
}