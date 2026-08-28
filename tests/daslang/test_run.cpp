// ============================================================
// PulseDaslang 运行测试：加入 daslang 插件后 app 的
// pulse_app_run（默认 runner，单帧）应正常完成。
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
        .name = "test-daslang-run",
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

    assert(pulse_app_run(app) == PULSE_APP_RUN_RESULT_OK);

    pulse_destroy_app(app);
    return 0;
}
