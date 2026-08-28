// ============================================================
// PulseDaslang 包测试：
//   通过 package_loader 加载 pulse_daslang 与一个 daslang 脚本包，
//   验证 loader -> pulse_package_get_runtimes -> 阶段二注入 ->
//   entry 标记 -> PostBuild 编译执行 importModule 的完整链路。
//   loader 阶段一负责 mount（manifest assets:true），阶段二枚举并集
//   快照注入全部 .das（含 daslang 自身 stdlib），PostBuild 统一编译
//   script_file 入口。无窗口。
// ============================================================

#include <assert.h>
#include <stdio.h>

#include <flecs.h> // C++ API - must be included before pulse headers

#include "pulse_app.h"
#include "pulse_daslang.h"
#include "pulse_package_loader.h"
#include "pulse_vfs.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "test-daslang-package",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulsePackageLoaderId loader = pulse_package_loader_create(app);
    assert(loader != nullptr);

    PulsePackageListEntry entries[] = {
        { "pulse_asset", nullptr },
        { "pulse_daslang", nullptr },
        { "pkg_das_test", nullptr },
    };
    const char* search_paths[] = { "src", "tests/daslang" };
    EPulsePackageLoadResult load_result = pulse_package_loader_load_packages(loader, search_paths, 2, entries, 3);
    if (load_result != PULSE_PACKAGE_LOAD_RESULT_OK)
    {
        printf("Package load failed: %s (result=%d)\n", pulse_app_last_error(app), (int)load_result);
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(loader);
        return -1;
    }

    assert(pulse_app_has_plugin(app, "pulse_asset"));
    assert(pulse_app_has_plugin(app, "pulse_daslang"));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    // das_test.das 是 manifest 的 script_file（入口）：PostBuild 统一编译并
    // 执行 importModule，应创建带 DasTestMarker 组件的实体。
    ecs_world_t* world = pulse_app_world(app);
    ecs_id_t marker_id = ecs_lookup(world, "DasTestMarker");
    assert(marker_id != 0);

    // second.das 不是入口，PostBuild 不编译；此时缓存里应有它的注入文本，
    // "仅编译已注入脚本"的语义使 pulse_load_module 能同步编译它。
    assert(pulse_load_module(app, "second.das"));
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    ecs_id_t second_id = ecs_lookup(world, "DasSecondMarker");
    assert(second_id != 0);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(loader);
    printf("Daslang package test passed!\n");
    return 0;
}
