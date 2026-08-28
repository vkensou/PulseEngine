#include <stdio.h>
#include "pulse_package_loader.h"
#include "pulse_vfs.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-config-rejected" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulsePackageLoaderId loader = pulse_package_loader_create(app);
    assert(loader != nullptr);

    // pulse_input does not accept a config; passing any non-null
    // PulseConfig should make register fail and loader report failure.
    PulseConfig* cfg = pulse_config_create();
    PulsePackageListEntry entry = {
        "pulse_input",
        cfg
    };

    const char* search_paths[] = { "src", "tests/package_loader" };
    assert(pulse_package_loader_load_packages(loader, search_paths, 1, &entry, 1) == PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED);

    pulse_config_release(cfg);
    pulse_destroy_app(app);
    pulse_package_loader_cleanup(loader);
    return 0;
}
