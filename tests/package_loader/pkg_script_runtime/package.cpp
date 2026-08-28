// Test-only package library for tests/package_loader/test_loader_script_runtime.cpp.
//
// Provides a mock script runtime ("mockscript") through the
// pulse_package_get_runtimes protocol so the loader can dispatch script
// packages to it. Under the injection protocol the runtime exposes:
//   load         - 阶段二单文件注入回调（记录注入计数）
//   load_package - 包入口标记回调（校验 loader 传参并注册插件）
#include <string.h>

#include "pulse_app.h"
#include "pulse_config.h"
#include "pulse_script_register.h"

namespace {

int g_injected_files = 0;

EPulseResult mockscript_load(PulseAppId app, const char* path, const char* text, uint64_t text_size) {
    if (!app || !path || !path[0] || (!text && text_size > 0)) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    if (text_size == 0) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    ++g_injected_files;
    return PULSE_RESULT_OK;
}

EPulseResult mockscript_load_package(PulseAppId app, const char* script_file) {
    if (!app || !script_file) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;

    // Validate every field the loader promised to hand over. Any mismatch
    // fails the load with ErrorRegisterFailed, which fails the test.
    if (!script_file || strcmp(script_file, "main.mock") != 0) return PULSE_RESULT_ERROR_INTERNAL;

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
        { "mockscript", "mock", mockscript_load, mockscript_load_package },
    };
    if (!out_runtimes) return 0;
    *out_runtimes = runtimes;
    return 1;
}
