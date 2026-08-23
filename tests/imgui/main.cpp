#include <assert.h>
#include <stdio.h>

#include <flecs.h>

#include <imgui.h>

#include "pulse_app.h"
#include "pulse_input.h"
#include "pulse_window.h"
#include "pulse_asset.h"
#include "pulse_graphics.h"
#include "pulse_imgui.h"

// imgui 插件必须在 graphics 插件之后注册（build 阶段校验）。
static void test_missing_graphics(void) {
    PulseAppDesc app_desc = {
        .name = "test-imgui-missing-graphics",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    auto window_desc = pulse_window_plugin_desc_default();
    assert(pulse_add_window_plugin(app, &window_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(pulse_add_imgui_plugin(app, nullptr) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY);

    pulse_destroy_app(app);
}

int main(void) {
    test_missing_graphics();

    PulseAppDesc app_desc = {
        .name = "test-imgui",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    auto window_desc = pulse_window_plugin_desc_default();
    window_desc.primary_window.title = "test-imgui";
    window_desc.primary_window.width = 640;
    window_desc.primary_window.height = 480;
    assert(pulse_add_window_plugin(app, &window_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    asset_desc.root_path = "examples/snake/assets";
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    auto graphics_desc = pulse_graphics_plugin_desc_default();
    assert(pulse_add_graphics_plugin(app, &graphics_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // 注册 + 重复注册校验
    assert(pulse_add_imgui_plugin(app, nullptr) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_add_imgui_plugin(app, nullptr) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN);

    // context 与 imgui phase 在 build 阶段创建（add 时立即执行）
    ImGuiContext* imgui_ctx = pulse_imgui_get_context(app);
    assert(imgui_ctx != nullptr);
    ecs_entity_t phase = pulse_imgui_get_phase(app);
    assert(phase != 0);

    // 平台后端（clipboard / IME / 鼠标光标 / open-url）在 add 时立即就绪
    ImGui::SetCurrentContext(imgui_ctx);
    {
        ImGuiIO& io = ImGui::GetIO();
        // 截图测试不能依赖仓库根目录的 imgui.ini：显式禁用 Ini 文件的读写，
        // 窗口位置/尺寸由下方 SetNextWindowPos/SetNextWindowSize 固定。
        io.IniFilename = nullptr;
        assert(io.BackendPlatformUserData != nullptr);
        assert((io.BackendFlags & ImGuiBackendFlags_HasMouseCursors) != 0);
        assert((io.BackendFlags & ImGuiBackendFlags_HasSetMousePos) != 0);
        ImGuiPlatformIO& pio = ImGui::GetPlatformIO();
        assert(pio.Platform_GetClipboardTextFn != nullptr);
        assert(pio.Platform_SetClipboardTextFn != nullptr);
        assert(pio.Platform_SetImeDataFn != nullptr);
        assert(pio.Platform_OpenInShellFn != nullptr);
        assert(ImGui::GetMainViewport()->PlatformHandle != nullptr);
    }

    // 用户侧用法（bevy_mod_imgui 风格）：自己注册 system 画控件。
    // 帧窗口从 PreUpdate（NewFrame）开到 PreStore（Render），
    // 挂普通阶段（OnUpdate）即可，不需要任何 imgui 专用标记；
    // 也支持挂 pulse_imgui_get_phase(app) 显式排在游戏逻辑之后。
    flecs::world world = flecs::world(pulse_app_world(app));
    bool show_demo_window = true;
    world.system("imgui_test_ui_onupdate")
        .kind(flecs::PostUpdate)
        .run([&show_demo_window](flecs::iter& it) {
            // 固定窗口位置和尺寸，避免受 imgui.ini 影响。
            ImGui::SetNextWindowPos(ImVec2(38, 99), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(380, 194), ImGuiCond_Always);
            ImGui::Begin("imgui test (OnUpdate)");
            ImGui::Text("Hello, PulseEngine imgui!");
            ImGui::End();
            // ImGui::ShowDemoWindow(&show_demo_window);
        });
    world.system("imgui_test_ui_phase")
        .kind(phase)
        .run([](flecs::iter& it) {
            // 固定窗口位置和尺寸，避免受 imgui.ini 影响。
            ImGui::SetNextWindowPos(ImVec2(150, 183), ImGuiCond_Always);
            ImGui::SetNextWindowSize(ImVec2(419, 238), ImGuiCond_Always);
            ImGui::Begin("imgui test (phase)");
            ImGui::Text("Drawn after game logic.");
            ImGui::End();
        });

    pulse_app_run(app);

    pulse_destroy_app(app);

    printf("All tests passed!\n");
    return 0;
}
