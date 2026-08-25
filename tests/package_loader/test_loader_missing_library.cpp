#include <stdio.h>
#include "pulse_package_loader.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-missing-lib" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // No package.json for this name in any search path and it is not registered
    // as a static package, so the loader cannot locate a loadable library.
    const char* search_paths[] = { "src", "tests/package_loader", "nowhere" };
    PulsePackageListEntry bad = { "BadPackage", nullptr };
    assert(pulse_package_loader_load_packages(app, 2, search_paths, 1, &bad) == PULSE_PACKAGE_LOAD_RESULT_ERROR_LIBRARY_NOT_FOUND);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    return 0;
}
