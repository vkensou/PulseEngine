// Test-only package library for tests/package_loader/test_loader_script_runtime.cpp.
//
// Provides a mock script runtime ("mockscript") through the
// pulse_package_get_runtimes protocol so the loader can dispatch script
// packages to it.
#include <string.h>

#include "pulse_app.h"
#include "pulse_config.h"
#include "pulse_script_register.h"

namespace {

EPulseResult mockscript_load(PulseAppId app, const PulsePackageScriptInfo* info) {
    if (!app || !info) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;

    // Validate every field the loader promised to hand over. Any mismatch
    // fails the load with ErrorRegisterFailed, which fails the test.
    if (!info->name || strcmp(info->name, "pkg_mockscript") != 0) return PULSE_RESULT_ERROR_INTERNAL;
    if (!info->script_file || strcmp(info->script_file, "main.mock") != 0) return PULSE_RESULT_ERROR_INTERNAL;
    if (!info->package_dir) return PULSE_RESULT_ERROR_INTERNAL;

    const size_t len = strlen(info->package_dir);
    const char* suffix = "pkg_mockscript";
    const size_t suffix_len = strlen(suffix);
    if (len < suffix_len || strcmp(info->package_dir + len - suffix_len, suffix) != 0)
        return PULSE_RESULT_ERROR_INTERNAL;

    if (!info->config) return PULSE_RESULT_ERROR_INTERNAL;
    if (strcmp(pulse_config_get_string(info->config, "greeting", ""), "hello") != 0)
        return PULSE_RESULT_ERROR_INTERNAL;

    PulsePluginDesc desc = {};
    desc.struct_size = sizeof(PulsePluginDesc);
    desc.version = PULSE_PLUGIN_DESC_VERSION;
    desc.plugin_version = 1;
    desc.name = "MockScriptLoaded";
    EPulseAppAddPluginResult r = pulse_app_add_plugin(app, &desc);
    return r == PULSE_APP_ADD_PLUGIN_RESULT_OK ? PULSE_RESULT_OK : PULSE_RESULT_ERROR_INTERNAL;
}

} // namespace

extern "C" PULSE_EXPORT EPulseResult pulse_package_register(PulseAppId app, PulseConfig* config) {
    (void)app;
    (void)config;
    return PULSE_RESULT_OK;
}

extern "C" PULSE_EXPORT uint32_t pulse_package_get_runtimes(const PulseScriptRuntimeDesc** out_runtimes) {
    static const PulseScriptRuntimeDesc runtimes[] = {
        { "mockscript", mockscript_load },
    };
    if (!out_runtimes) return 0;
    *out_runtimes = runtimes;
    return 1;
}
