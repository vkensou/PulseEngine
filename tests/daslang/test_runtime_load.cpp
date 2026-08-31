// ============================================================
// PulseDaslang 运行时编译测试：
//   prepare（PostBuild）之后由 pulse_load_module 从注入缓存同步编译
//   未在 PostBuild 编译的脚本（second.das 不是 manifest 入口），
//   调用返回时 importModule 已执行、DasSecondMarker 立即存在——
//   验证「编译与加载分离」：脚本加载（注入）只发生在包加载阶段，
//   PostBuild 后只允许编译已注入的脚本（缓存 miss 即失败）。
// ============================================================

#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"
#include "pulse_daslang.h"

#include "daslang_inject_helper.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "test-daslang-runtime-load",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_vfs_mount("src/pulse_daslang", "/", false));
    assert(pulse_vfs_mount("tests/daslang/pkg_das_test", "/", false));

    auto daslang_desc = pulse_daslang_plugin_desc_default();
    assert(pulse_add_daslang_plugin(app, &daslang_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // 注入全部 .das；das_test.das 将作为入口（prepare 前 pulse_load_module）。
    assert(daslang_inject_helper::inject_all_das(app) > 0);
    assert(pulse_load_module(app, "das_test.das"));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    ecs_world_t* world = pulse_app_world(app);

    // PostBuild 已编译并执行 das_test.das 的 importModule。
    assert(ecs_lookup(world, "DasTestMarker") != 0);

    // second.das 已注入但未在 PostBuild 编译（非入口）。
    assert(ecs_lookup(world, "DasSecondMarker") == 0);

    // PostBuild 后：从缓存同步编译——调用返回时 marker 已存在（无需逐帧等待）。
    assert(pulse_load_module(app, "second.das"));
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    ecs_id_t second_id = ecs_lookup(world, "DasSecondMarker");
    assert(second_id != 0);

    // 未注入的路径无法编译（缓存 miss 即失败）。
    assert(!pulse_load_module(app, "no_such_script.das"));

    pulse_destroy_app(app);
    printf("Daslang runtime load test passed!\n");
    return 0;
}
