// ============================================================
// PulseEngine Launcher (plugin package loader)
//
// 这个 launcher 不链接任何具体的 pulse_* 插件，只链接：
//   - pulse_app            创建/运行 app
//   - pulse_package_loader 动态加载 package
//
// 所有插件（包括 example-snake 本身）都通过 package 列表在运行时
// 加载。当前 package 列表先硬编码在代码里，后续再拆成 JSON 配置。
// ============================================================

#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_package_loader.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "pulse-launcher",
        .enable_restapi = true,
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // ---- 硬编码的 package 列表（之后会改为 JSON） ----
    // 依赖关系用于让 package loader 按拓扑序加载。
    static const char* window_deps[] = { "PulseInputPlugin" };
    static const char* graphics_deps[] = { "PulseWindowPlugin", "PulseAssetPlugin" };
    static const char* renderer_deps[] = {
        "PulseWindowPlugin",
        "PulseGraphicPlugin",
        "PulseTransformPlugin",
    };
    static const char* imgui_deps[] = {
        "PulseWindowPlugin",
        "PulseInputPlugin",
        "PulseAssetPlugin",
        "PulseGraphicPlugin",
    };
    static const char* snake_deps[] = {
        "PulseWindowPlugin",
        "PulseInputPlugin",
        "PulseAssetPlugin",
        "PulseTransformPlugin",
        "PulseGraphicPlugin",
        "PulseRendererPlugin",
        "PulseImguiPlugin",
    };

    // 配置暂时全部为 nullptr，使用各插件的默认配置。
    // 之后 JSON 化时，这里可以替换为从 json 解析出的 config buffer。
    PulsePackageListEntry packages[] = {
        { "PulseInputPlugin",    "pulse_input.dll",    nullptr, 0, 0, nullptr },
        { "PulseWindowPlugin",   "pulse_window.dll",   nullptr, 0, 1, window_deps },
        { "PulseAssetPlugin",    "pulse_asset.dll",    nullptr, 0, 0, nullptr },
        { "PulseTransformPlugin","pulse_transform.dll",nullptr, 0, 0, nullptr },
        { "PulseGraphicPlugin",  "pulse_graphics.dll", nullptr, 0, 2, graphics_deps },
        { "PulseRendererPlugin", "pulse_renderer.dll", nullptr, 0, 3, renderer_deps },
        { "PulseImguiPlugin",    "pulse_imgui.dll",    nullptr, 0, 4, imgui_deps },
        { "PulseSnakePlugin",    "example_snake.dll",  nullptr, 0, 7, snake_deps },
    };

    EPulsePackageLoadResult load_result =
        pulse_package_loader_load_packages(app, packages, 8);
    if (load_result != PULSE_PACKAGE_LOAD_RESULT_OK) {
        fprintf(stderr, "failed to load packages, result=%d\n", (int)load_result);
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        return 1;
    }

    pulse_app_run(app);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    printf("Pulse launcher exited.\n");
    return 0;
}
