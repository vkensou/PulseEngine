// ============================================================
// PulseEngine Snake（daslang 版）—— 纯宿主
//
// 游戏逻辑全部位于 snake_module.das：
//   - pulse_daslang 插件在 build 阶段编译脚本并调用 importModule
//   - 组件/资源/状态机/系统注册均发生在脚本内
// ============================================================

#include <assert.h>
#include <stdio.h>

#include <flecs.h> // C++ API - must be included before pulse headers

#include "pulse_app.h"
#include "pulse_window.h"
#include "pulse_input.h"
#include "pulse_asset.h"
#include "pulse_transform.h"
#include "pulse_graphics.h"
#include "pulse_renderer.h"
#include "pulse_imgui.h"
#include "pulse_daslang.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "pulse-snake-daslang-example",
        .enable_restapi = true,
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // ---- 插件 ----
    assert(pulse_add_input_plugin(app) == PULSE_RESULT_OK);

    auto window_desc = pulse_window_plugin_desc_default();
    window_desc.primary_window.title = "PulseEngine Snake Daslang";
    window_desc.primary_window.width = 800;
    window_desc.primary_window.height = 600;
    window_desc.primary_window.resizable = false;
    assert(pulse_add_window_plugin(app, &window_desc) == PULSE_RESULT_OK);

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    asset_desc.root_path = "examples/snake/assets";
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_RESULT_OK);

    assert(pulse_add_transform_plugin(app) == PULSE_RESULT_OK);

    auto graphics_desc = pulse_graphics_plugin_desc_default();
    graphics_desc.enable_debug_layer = true;
    graphics_desc.enable_gpu_based_validation = true;
    assert(pulse_add_graphics_plugin(app, &graphics_desc) == PULSE_RESULT_OK);

    assert(pulse_add_renderer_plugin(app) == PULSE_RESULT_OK);

    assert(pulse_add_imgui_plugin(app, nullptr) == PULSE_RESULT_OK);

    // ---- daslang 游戏模块 ----
    auto daslang_desc = pulse_daslang_plugin_desc_default();
    daslang_desc.root_path = "examples/asset";
    // 插件只初始化 daScript 环境；脚本通过 pulse_load_module 动态加载。
    EPulseResult daslang_result = pulse_add_daslang_plugin(app, &daslang_desc);
    if (daslang_result != PULSE_RESULT_OK)
    {
        printf("Daslang plugin failed: %s\n", pulse_app_last_error(app));
        pulse_destroy_app(app);
        return -1;
    }

    // 动态加载并执行 das 游戏模块
    if (!pulse_load_module(app, "examples/snake_daslang/snake_module.das"))
    {
        printf("Daslang load module failed: %s\n", pulse_app_last_error(app));
        pulse_destroy_app(app);
        return -1;
    }

    pulse_app_run(app);

    pulse_destroy_app(app);
    printf("Snake daslang example exited.\n");
    return 0;
}
