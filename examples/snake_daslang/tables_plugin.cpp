#include <cstdio>

#include "pulse_config.h"
#include "pulse_cpp_gameplay.h"
#include "pulse_datatable.h"

#include "tables_generated.h"

namespace {

constexpr const char* kPluginName = "snake_daslang_tables";

EPulsePluginBuildResult snake_daslang_tables_build(PulseAppId app, void* ctx)
{
    (void)ctx;
    if (!app)
    {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    PulseDataTableSystemId system = pulse_get_data_table_system(app);
    if (!system)
    {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE;
    }
    if (pulse_tables::RegisterSchemas(system) != PULSE_RESULT_OK)
    {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }
    printf("snake_daslang: data table schemas registered\n");
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulseAppAddPluginResult add_tables_plugin(PulseAppId app)
{
    if (!app)
    {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, kPluginName))
    {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    static const char* dependencies[] = {
        "pulse_datatable",
    };

    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = 1,
        .name = kPluginName,
        .ctx = nullptr,
        .build = snake_daslang_tables_build,
        .post_build = nullptr,
        .shutdown = nullptr,
        .dependency_count = 1,
        .dependencies = dependencies,
    };

    return pulse_app_add_plugin(app, &plugin_desc);
}

} // namespace

extern "C" PULSE_EXPORT EPulseResult pulse_package_register(PulseAppId app, PulseConfig* config)
{
    if (config)
    {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return pulse::to_package_result(add_tables_plugin(app));
}
