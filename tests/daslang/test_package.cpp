// ============================================================
// PulseDaslang 包测试：
//   通过 package_loader 加载 pulse_daslang 与一个 daslang 脚本包，
//   验证 loader -> pulse_package_get_runtimes -> handler ->
//   pulse_load_module 的完整链路（无窗口）。
// ============================================================

#include <assert.h>
#include <stdio.h>

#include <flecs.h> // C++ API - must be included before pulse headers

#include "pulse_app.h"
#include "pulse_package_loader.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "test-daslang-package",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulsePackageListEntry entries[] = {
        { "pulse_asset", nullptr },
        { "pulse_daslang", nullptr },
        { "pkg_das_test", nullptr },
    };
    const char* search_paths[] = { "src", "tests/daslang" };
    EPulsePackageLoadResult load_result = pulse_package_loader_load_packages(app, 2, search_paths, 3, entries);
    if (load_result != PULSE_PACKAGE_LOAD_RESULT_OK)
    {
        printf("Package load failed: %s (result=%d)\n", pulse_app_last_error(app), (int)load_result);
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        return -1;
    }

    assert(pulse_app_has_plugin(app, "pulse_asset"));
    assert(pulse_app_has_plugin(app, "pulse_daslang"));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    // das_test.das 的 importModule 会创建带 DasTestMarker 组件的实体。
    ecs_world_t* world = pulse_app_world(app);
    ecs_id_t marker_id = ecs_lookup(world, "DasTestMarker");
    assert(marker_id != 0);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    printf("Daslang package test passed!\n");
    return 0;
}
