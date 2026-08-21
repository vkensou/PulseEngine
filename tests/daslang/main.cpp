// ============================================================
// PulseDaslang 插件冒烟测试
//
// 这里不测 flecs systems，只验证：
//   1. pulse_daslang 插件可以被创建并加入 app；
//   2. das 脚本能被编译、模拟并执行 importModule；
//   3. 加入插件后 app 仍能正常 update 若干帧。
// ============================================================

#include <assert.h>
#include <stdio.h>

#include <flecs.h> // C++ API - must be included before pulse headers

#include "pulse_app.h"
#include "pulse_daslang.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "test-daslang",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // ---- daslang 插件 ----
    auto daslang_desc = pulse_daslang_plugin_desc_default();
    daslang_desc.root_path = "examples/asset";                 // 复用现有 pulse daslib 目录
    // 脚本随后在下方通过 pulse_load_module 动态加载。
    EPulseResult daslang_result = pulse_add_daslang_plugin(app, &daslang_desc);
    if (daslang_result != PULSE_RESULT_OK)
    {
        printf("Daslang plugin failed: %s\n", pulse_app_last_error(app));
        pulse_destroy_app(app);
        return -1;
    }

    assert(pulse_app_has_plugin(app, "PulseDaslangPlugin"));

    // 动态加载并执行 das 脚本
    if (!pulse_load_module(app, "tests/daslang/das_test.das"))
    {
        printf("Daslang load module failed: %s\n", pulse_app_last_error(app));
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

    assert(pulse_app_run(app) == PULSE_RESULT_OK);

    pulse_destroy_app(app);

    printf("PulseDaslang tests passed!\n");
    return 0;
}
