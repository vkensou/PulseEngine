// ============================================================
// PulseDaslang 插件冒烟测试：插件创建、加入 app、重复添加检查。
// 不加载 das 脚本（见 test_module.cpp）。
// ============================================================

#include <assert.h>
#include <stdio.h>

#include <flecs.h> // C++ API - must be included before pulse headers

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_daslang.h"
#include "pulse_vfs.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "test-daslang-plugin",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_vfs_mount("src/pulse_daslang", "/", false));

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    auto daslang_desc = pulse_daslang_plugin_desc_default();
    EPulseAppAddPluginResult daslang_result = pulse_add_daslang_plugin(app, &daslang_desc);
    if (daslang_result != PULSE_APP_ADD_PLUGIN_RESULT_OK)
    {
        pulse_destroy_app(app);
        return -1;
    }

    assert(pulse_app_has_plugin(app, "pulse_daslang"));

    // Adding again should fail with DUPLICATE
    auto daslang_desc2 = pulse_daslang_plugin_desc_default();
    assert(pulse_add_daslang_plugin(app, &daslang_desc2) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN);

    pulse_destroy_app(app);
    return 0;
}
