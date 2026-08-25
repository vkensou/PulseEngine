#include <stdio.h>
#include "pulse_package_loader.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-config-rejected" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // pulse_input does not accept a config; passing any non-null
    // PulseConfig should make register fail and loader report failure.
    PulseConfig* cfg = pulse_config_create();
    PulsePackageListEntry entry = {
        "pulse_input",
        cfg
    };

    const char* search_paths[] = { "packages" };
    assert(pulse_package_loader_load_packages(app, 1, search_paths, 1, &entry) == PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED);

    pulse_config_release(cfg);
    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    return 0;
}
