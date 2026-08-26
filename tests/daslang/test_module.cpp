// ============================================================
// PulseDaslang 模块测试：
//   1. das 脚本能被编译、模拟并执行 importModule；
//   2. importModule 创建的 DasTestMarker 组件确实写入了值。
// ============================================================

#include <assert.h>
#include <stdio.h>

#include <flecs.h> // C++ API - must be included before pulse headers

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"
#include "pulse_daslang.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "test-daslang-module",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_vfs_add_content_root("tests/daslang/pkg_das_test") == PULSE_VFS_RESULT_OK);
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
 
    // ---- daslang 插件 ----
    auto daslang_desc = pulse_daslang_plugin_desc_default();
    // 脚本随后在下方通过 pulse_load_module 动态加载。
    EPulseAppAddPluginResult daslang_result = pulse_add_daslang_plugin(app, &daslang_desc);
    if (daslang_result != PULSE_APP_ADD_PLUGIN_RESULT_OK)
    {
        pulse_destroy_app(app);
        return -1;
    }

    assert(pulse_app_has_plugin(app, "pulse_daslang"));

    // 动态加载并执行 das 脚本
    if (!pulse_load_module(app, "tests/daslang/pkg_das_test/das_test.das"))
    {
        pulse_destroy_app(app);
        return -1;
    }

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
