// ============================================================
// PulseDaslang 模块测试：
//   1. das 脚本能通过公开注入通道（runtime load 回调）进入缓存；
//   2. prepare 前 pulse_load_module 标记入口，PostBuild 统一编译，
//      importModule 创建的 DasTestMarker 组件写入了值。
// 手动模式（不经 package_loader）：自行 mount 包目录 + 枚举注入。
// ============================================================

#include <assert.h>
#include <stdio.h>

#include <flecs.h> // C++ API - must be included before pulse headers

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"
#include "pulse_daslang.h"

#include "daslang_inject_helper.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "test-daslang-module",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(pulse_vfs_mount("src/pulse_daslang", "/", false));
    assert(pulse_vfs_mount("tests/daslang/pkg_das_test", "/", false));

    auto daslang_desc = pulse_daslang_plugin_desc_default();
    EPulseAppAddPluginResult daslang_result = pulse_add_daslang_plugin(app, &daslang_desc);
    if (daslang_result != PULSE_APP_ADD_PLUGIN_RESULT_OK)
    {
        pulse_destroy_app(app);
        return -1;
    }
    assert(pulse_app_has_plugin(app, "pulse_daslang"));

    // 通过对外注入通道（pulse_package_get_runtimes 的 load 回调）
    // 注入全部 .das（daslang 自带 stdlib + pkg_das_test 脚本）。
    int injected = daslang_inject_helper::inject_all_das(app);
    assert(injected > 0);

    // prepare 之前提交：进 pending，PostBuild 统一编译执行。
    assert(pulse_load_module(app, "das_test.das"));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    // 验证 das 脚本的 importModule 确实执行了：它应创建 DasTestMarker 组件并写值。
    ecs_world_t* world = pulse_app_world(app);
    ecs_id_t marker_id = ecs_lookup(world, "DasTestMarker");
    assert(marker_id != 0);

    ecs_query_desc_t query_desc = {};
    query_desc.terms[0].id = marker_id;
    ecs_query_t* query = ecs_query_init(world, &query_desc);
    assert(query != nullptr);

    struct DasTestMarker
    {
        int value;
    };

    int marker_count = 0;
    ecs_iter_t it = ecs_query_iter(world, query);
    while (ecs_query_next(&it))
    {
        const DasTestMarker* markers = static_cast<const DasTestMarker*>(
            ecs_field_w_size(&it, sizeof(DasTestMarker), 0));
        for (int i = 0; i < it.count; ++i)
        {
            assert(markers[i].value == 123);
            ++marker_count;
        }
    }
    assert(marker_count == 1);
    ecs_query_fini(query);

    pulse_destroy_app(app);
    return 0;
}