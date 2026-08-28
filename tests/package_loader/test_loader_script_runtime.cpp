// Verifies script package dispatch: a native package can provide a script
// runtime through pulse_package_get_runtimes, and a package whose manifest
// declares a non-native "type" is handed to the matching runtime handler with
// name / package_dir / script_file / config. An unknown type fails with
// PULSE_PACKAGE_LOAD_RESULT_ERROR_UNKNOWN_RUNTIME.
#include <stdio.h>

#include "pulse_config.h"
#include "pulse_package_loader.h"
#include "pulse_vfs.h"

int main() {
    // Unknown runtime type must fail.
    {
        PulseAppDesc app_desc = { .name = "loader-script-runtime-unknown" };
        PulseAppId app = pulse_create_app(&app_desc);
        assert(app != nullptr);
        PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
        assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

        PulsePackageLoaderId loader = pulse_package_loader_create(app);
        assert(loader != nullptr);

        PulsePackageListEntry entries[] = {
            { "pkg_unknown_runtime", nullptr },
        };
        const char* search_paths[] = { "tests/package_loader" };
        EPulsePackageLoadResult r = pulse_package_loader_load_packages(loader, 1, search_paths, 1, entries);
        assert(r == PULSE_PACKAGE_LOAD_RESULT_ERROR_UNKNOWN_RUNTIME);

        pulse_destroy_app(app);
        pulse_package_loader_cleanup(loader);
    }

    // Provider + script package: the handler validates every info field and
    // then registers a marker plugin.
    {
        PulseAppDesc app_desc = { .name = "loader-script-runtime" };
        PulseAppId app = pulse_create_app(&app_desc);
        assert(app != nullptr);
        PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
        assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

        PulsePackageLoaderId loader = pulse_package_loader_create(app);
        assert(loader != nullptr);

        PulseConfig* cfg = pulse_config_create();
        pulse_config_set_string(cfg, "greeting", "hello");

        PulsePackageListEntry entries[] = {
            { "pkg_script_runtime", nullptr },
            { "pkg_mockscript", cfg },
        };
        const char* search_paths[] = { "tests/package_loader" };
        EPulsePackageLoadResult r = pulse_package_loader_load_packages(loader, 2, search_paths, 2, entries);
        pulse_config_release(cfg);
        assert(r == PULSE_PACKAGE_LOAD_RESULT_OK);

        // Registered by the mock handler after validating all info fields.
        assert(pulse_app_has_plugin(app, "MockScriptLoaded"));
        // "assets": true mounted the script package dir as a VFS content root.
        assert(pulse_vfs_exists("assets/mock_marker.dat"));

        pulse_destroy_app(app);
        pulse_package_loader_cleanup(loader);
    }

    printf("Package loader script runtime test passed!\n");
    return 0;
}
