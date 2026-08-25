#include <stdio.h>
#include "pulse_package_loader.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-missing-dep" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // pulse_window depends on pulse_input; omitting it from the list should be
    // reported before any library is opened.  The loader reads package.json
    // itself, so the caller no longer needs to supply dependencies.
    PulsePackageListEntry entries[] = {
        { "pulse_window", nullptr },
    };

    const char* search_paths[] = { "src", "tests/package_loader" };
    assert(pulse_package_loader_load_packages(app, 1, search_paths, 1, entries) == PULSE_PACKAGE_LOAD_RESULT_ERROR_MISSING_DEPENDENCY);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    return 0;
}
