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

    // ---- 插件（通过 module list + loader 动态加载） ----
    static const char* window_deps[] = { "PulseInputPlugin" };
    static const char* graphics_deps[] = { "PulseWindowPlugin", "PulseAssetPlugin" };
    static const char* renderer_deps[] = { "PulseWindowPlugin", "PulseGraphicPlugin", "PulseTransformPlugin" };
    static const char* imgui_deps[] = { "PulseWindowPlugin", "PulseInputPlugin", "PulseAssetPlugin", "PulseGraphicPlugin" };

    auto window_desc = pulse_window_plugin_desc_default();
    window_desc.primary_window.title = "PulseEngine Snake Daslang";
    window_desc.primary_window.width = 800;
    window_desc.primary_window.height = 600;
    window_desc.primary_window.resizable = false;

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    asset_desc.root_path = "examples/snake/assets";

    auto graphics_desc = pulse_graphics_plugin_desc_default();
    graphics_desc.enable_debug_layer = true;
    graphics_desc.enable_gpu_based_validation = true;

    auto daslang_desc = pulse_daslang_plugin_desc_default();
    daslang_desc.root_path = "examples/asset";

    PulseModuleListEntry modules[] = {
        { "PulseInputPlugin", "pulse_input.dll", nullptr, 0, 0, nullptr },
        { "PulseWindowPlugin", "pulse_window.dll", &window_desc, sizeof(window_desc), 1, window_deps },
        { "PulseAssetPlugin", "pulse_asset.dll", &asset_desc, sizeof(asset_desc), 0, nullptr },
        { "PulseTransformPlugin", "pulse_transform.dll", nullptr, 0, 0, nullptr },
        { "PulseGraphicPlugin", "pulse_graphics.dll", &graphics_desc, sizeof(graphics_desc), 2, graphics_deps },
        { "PulseRendererPlugin", "pulse_renderer.dll", nullptr, 0, 3, renderer_deps },
        { "PulseImguiPlugin", "pulse_imgui.dll", nullptr, 0, 4, imgui_deps },
        { "PulseDaslangPlugin", "pulse_daslang.dll", &daslang_desc, sizeof(daslang_desc), 0, nullptr },
    };
    EPulseModuleLoadResult load_result = pulse_app_load_modules(app, modules, 8);
    if (load_result != PULSE_MODULE_LOAD_RESULT_OK)
    {
        printf("Module load failed: %s\n", pulse_app_last_error(app));
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
