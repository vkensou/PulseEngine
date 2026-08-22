#include <stdio.h>
#include "pulse_package_loader.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-simple" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulsePackageListEntry entries[] = {
        { "PulseInputPlugin", "pulse_input.dll", nullptr, 0, 0, nullptr },
        { "PulseTransformPlugin", "pulse_transform.dll", nullptr, 0, 0, nullptr },
    };

    assert(pulse_package_loader_load_packages(app, entries, 2) == PULSE_PACKAGE_LOAD_RESULT_OK);
    assert(pulse_app_has_plugin(app, "PulseInputPlugin"));
    assert(pulse_app_has_plugin(app, "PulseTransformPlugin"));

    // Loading the same package again is a duplicate.
    PulsePackageListEntry dup = { "PulseInputPlugin", "pulse_input.dll", nullptr, 0, 0, nullptr };
    assert(pulse_package_loader_load_packages(app, &dup, 1) == PULSE_PACKAGE_LOAD_RESULT_ERROR_DUPLICATE_PACKAGE);
    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    return 0;
}
