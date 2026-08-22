#include <stdio.h>
#include "pulse_package_loader.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-config-size" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    uint32_t wrong_size_config = 0;
    PulsePackageListEntry entry = {
        "PulseAssetPlugin",
        "pulse_asset.dll",
        &wrong_size_config,
        sizeof(wrong_size_config),
        0,
        nullptr
    };

    assert(pulse_package_loader_load_packages(app, &entry, 1) == PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    return 0;
}
