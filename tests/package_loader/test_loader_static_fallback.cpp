#include <stdio.h>
#include "pulse_package_loader.h"

static EPulseResult fake_static_register(PulseAppId app, PulseConfig*) {
    PulsePluginDesc desc = {};
    desc.struct_size = sizeof(PulsePluginDesc);
    desc.version = PULSE_PLUGIN_DESC_VERSION;
    desc.plugin_version = 1;
    desc.name = "StaticFakePlugin";
    EPulseAppAddPluginResult r = pulse_app_add_plugin(app, &desc);
    return r == PULSE_APP_ADD_PLUGIN_RESULT_OK ? PULSE_RESULT_OK : PULSE_RESULT_ERROR_INTERNAL;
}

int main() {
    PulseAppDesc app_desc = { .name = "loader-static" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    pulse_package_loader_register_static_package(app, "StaticFake", fake_static_register);
    PulsePackageListEntry entry = { "StaticFake", nullptr };
    assert(pulse_package_loader_load_packages(app, &entry, 1) == PULSE_PACKAGE_LOAD_RESULT_OK);
    assert(pulse_app_has_plugin(app, "StaticFakePlugin"));

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    return 0;
}
