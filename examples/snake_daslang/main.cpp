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
#include "pulse_config.h"
#include "pulse_package_loader.h"
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

    // ---- 插件（通过 loader 动态加载；library/dependencies 由 loader
    //      读取各包自己的 package.json） ----
    PulseConfig* window_cfg = pulse_config_create();
    {
        PulseConfig* pw = pulse_config_create();
        pulse_config_set_string(pw, "title", "PulseEngine Snake Daslang");
        pulse_config_set_int(pw, "width", 800);
        pulse_config_set_int(pw, "height", 600);
        pulse_config_set_bool(pw, "resizable", false);
        pulse_config_set_obj(window_cfg, "primary_window", pw);
        pulse_config_release(pw);
    }

    PulseConfig* asset_cfg = pulse_config_create();
    pulse_config_set_string(asset_cfg, "root_path", "examples/snake/assets");
    pulse_config_set_int(asset_cfg, "max_requests_per_update", 8);

    PulseConfig* graphics_cfg = pulse_config_create();
    pulse_config_set_bool(graphics_cfg, "enable_debug_layer", true);
    pulse_config_set_bool(graphics_cfg, "enable_gpu_based_validation", true);

    PulseConfig* daslang_cfg = pulse_config_create();
    pulse_config_set_string(daslang_cfg, "root_path", "examples/asset");

    PulsePackageListEntry packages[] = {
        { "pulse_input", nullptr },
        { "pulse_window", window_cfg },
        { "pulse_asset", asset_cfg },
        { "pulse_transform", nullptr },
        { "pulse_graphics", graphics_cfg },
        { "pulse_renderer", nullptr },
        { "pulse_imgui", nullptr },
        { "pulse_daslang", daslang_cfg },
    };
    EPulsePackageLoadResult load_result = pulse_package_loader_load_packages(app, packages, 8);
    pulse_config_release(window_cfg);
    pulse_config_release(asset_cfg);
    pulse_config_release(graphics_cfg);
    pulse_config_release(daslang_cfg);

    if (load_result != PULSE_PACKAGE_LOAD_RESULT_OK)
    {
        printf("Package load failed: %s\n", pulse_app_last_error(app));
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        return -1;
    }

    // 动态加载并执行 das 游戏模块
    if (!pulse_load_module(app, "examples/snake_daslang/snake_module.das"))
    {
        printf("Daslang load module failed: %s\n", pulse_app_last_error(app));
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        return -1;
    }

    pulse_app_run(app);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    printf("Snake daslang example exited.\n");
    return 0;
}
