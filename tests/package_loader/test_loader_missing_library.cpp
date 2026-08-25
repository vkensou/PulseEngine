#include <stdio.h>
#include "pulse_package_loader.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-missing-lib" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // No package.json for this name anywhere and it is not registered as a
    // static package, so the loader cannot locate a loadable library.
    PulsePackageListEntry bad = { "BadPackage", nullptr };
    assert(pulse_package_loader_load_packages(app, &bad, 1) == PULSE_PACKAGE_LOAD_RESULT_ERROR_LIBRARY_NOT_FOUND);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    return 0;
}
