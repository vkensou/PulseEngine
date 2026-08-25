// Verifies the package.json "entry" feature of the package loader: the entry
// symbol selects a custom DLL export as the register entry point instead of the
// default "pulse_package_register".
#include <stdio.h>
#include "pulse_package_loader.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-custom-entry" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulsePackageListEntry entries[] = {
        { "pkg_custom_entry", nullptr },
    };
    const char* search_paths[] = { "src", "tests/package_loader" };
    assert(pulse_package_loader_load_packages(app, 2, search_paths, 1, entries) == PULSE_PACKAGE_LOAD_RESULT_OK);
    // The custom "entry" symbol from package.json was used to register this plugin.
    assert(pulse_app_has_plugin(app, "CustomEntryPlugin"));

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    return 0;
}