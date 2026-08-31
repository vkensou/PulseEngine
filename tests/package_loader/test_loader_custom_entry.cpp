// Verifies the package.json "entry" feature of the package loader: the entry
// symbol selects a custom DLL export as the register entry point instead of the
// default "pulse_package_register".
#include <stdio.h>
#include "pulse_package_loader.h"
#include "pulse_vfs.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-custom-entry" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulsePackageLoaderId loader = pulse_package_loader_create(app);
    assert(loader != nullptr);

    PulsePackageListEntry entries[] = {
        { "pkg_custom_entry", nullptr },
    };
    const char* search_paths[] = { "src", "tests/package_loader" };
    assert(pulse_package_loader_load_packages(loader, search_paths, 2, entries, 1) == PULSE_PACKAGE_LOAD_RESULT_OK);
    // The custom "entry" symbol from package.json was used to register this plugin.
    assert(pulse_app_has_plugin(app, "CustomEntryPlugin"));

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(loader);
    return 0;
}
