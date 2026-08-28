#include <stdio.h>
#include "pulse_package_loader.h"
#include "pulse_vfs.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-missing-dep" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulsePackageLoaderId loader = pulse_package_loader_create(app);
    assert(loader != nullptr);

    // pulse_window depends on pulse_input; omitting it from the list should be
    // reported before any library is opened.  The loader reads package.json
    // itself, so the caller no longer needs to supply dependencies.
    PulsePackageListEntry entries[] = {
        { "pulse_window", nullptr },
    };

    const char* search_paths[] = { "src", "tests/package_loader" };
    assert(pulse_package_loader_load_packages(loader, 1, search_paths, 1, entries) == PULSE_PACKAGE_LOAD_RESULT_ERROR_MISSING_DEPENDENCY);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(loader);
    return 0;
}
