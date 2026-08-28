#include <stdio.h>
#include "pulse_package_loader.h"
#include "pulse_vfs.h"

int main() {
    PulseAppDesc app_desc = { .name = "loader-simple" };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulsePackageLoaderId loader = pulse_package_loader_create(app);
    assert(loader != nullptr);

    PulsePackageListEntry entries[] = {
        { "pulse_input", nullptr },
        { "pulse_transform", nullptr },
    };

    // 搜索目录相对运行目录（项目根）解析：包清单来自 src/ 与 tests/
    const char* search_paths[] = { "src", "tests/package_loader" };
    assert(pulse_package_loader_load_packages(loader, search_paths, 1, entries, 2) == PULSE_PACKAGE_LOAD_RESULT_OK);
    assert(pulse_app_has_plugin(app, "pulse_input"));
    assert(pulse_app_has_plugin(app, "pulse_transform"));

    // Loading the same package again is a duplicate.
    PulsePackageListEntry dup = { "pulse_input", nullptr };
    assert(pulse_package_loader_load_packages(loader, search_paths, 1, &dup, 1) == PULSE_PACKAGE_LOAD_RESULT_ERROR_DUPLICATE_PACKAGE);
    pulse_destroy_app(app);
    pulse_package_loader_cleanup(loader);
    return 0;
}
