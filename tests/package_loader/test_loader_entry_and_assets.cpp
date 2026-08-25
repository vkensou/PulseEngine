// Verifies the package.json "assets": true feature of the package loader:
// the package directory (and its <dir>/assets subdirectory, if present) is
// registered as a pulse_vfs content root so assets shipped with the package
// can be resolved by their relative path.
//
// Positive control: pkg_assets declares "assets": true and ships an assets/
// directory in the source tree, so package.json and assets/marker.dat must be
// resolvable through pulse_vfs.
//
// Negative control: pkg_custom_entry declares no "assets" field, so nothing in
// its directory may be resolvable through pulse_vfs.
#include <stdio.h>
#include "pulse_package_loader.h"
#include "pulse_vfs.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-entry-and-assets" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulsePackageListEntry entries[] = {
        { "pkg_assets", nullptr },
        { "pkg_custom_entry", nullptr },
    };
    const char* search_paths[] = { "src", "tests/package_loader" };
    assert(pulse_package_loader_load_packages(app, 2, search_paths, 2, entries) == PULSE_PACKAGE_LOAD_RESULT_OK);
    assert(pulse_app_has_plugin(app, "AssetsEntryPlugin"));
    assert(pulse_app_has_plugin(app, "CustomEntryPlugin"));

    // pkg_assets has "assets": true -> its package dir and assets/ are content roots.
    assert(pulse_vfs_file_exists("package.json"));
    assert(pulse_vfs_file_exists("marker.dat"));

    // pkg_custom_entry has no "assets" field -> its package dir is NOT a root.
    assert(!pulse_vfs_file_exists("test-pkg-custom-entry.dll"));

    // Unknown files resolve nowhere.
    assert(!pulse_vfs_file_exists("no_such_package_file.dat"));

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    printf("Package loader assets test passed!\n");
    return 0;
}