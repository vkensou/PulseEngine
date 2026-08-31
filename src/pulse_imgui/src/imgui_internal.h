#pragma once

#include "pulse_imgui.h"
#include "pulse_app.h"
#include "pulse_window.h"
#include "pulse_input.h"
#include "pulse_asset.h"
#include "pulse_graphics.h"

#include <imgui.h>
#include <SDL3/SDL.h>

#include <chrono>
#include <vector>

namespace pulse_imgui_internal {

struct pulse_imgui_platform_data;

struct pulse_imgui_plugin_state {
    PulseAppId app = nullptr;
    PulseImguiPluginDesc desc{};
    char* ini_filename = nullptr;

    ImGuiContext* context = nullptr;
    ecs_entity_t frame_system = 0;
    ecs_entity_t imgui_phase = 0;
    std::chrono::steady_clock::time_point last_frame_time{};

    pulse_imgui_platform_data* platform_data = nullptr;

    // 每帧 UI 结果（render record callback 使用）
    ImDrawData* draw_data = nullptr;
    bool frame_open = false;

    // 渲染资源（全部通过 pulse_graphics 公共 API 管理）
    PulseShaderHandle shader{};
    PulseMaterialHandle material{};
    PulseMeshHandle mesh{};
    PulseTextureRequest font_request{};
    PulseTextureHandle font_texture{};
    PulseSamplerHandle font_sampler{};
    bool font_ready = false;
};

typedef struct pulse_imgui_state_resource {
    pulse_imgui_plugin_state* state;
} pulse_imgui_state_resource;

extern ECS_COMPONENT_DECLARE(pulse_imgui_state_resource);

pulse_imgui_plugin_state* state_from_world(ecs_world_t* world);
pulse_imgui_plugin_state* state_from_app(PulseAppId app);

// 解析当前渲染目标窗口（不缓存单窗口状态，未来可扩展为多窗口）。
ecs_entity_t imgui_get_window_entity(ecs_world_t* world, pulse_imgui_plugin_state* state);

// imgui_context.cpp
ecs_entity_t create_imgui_phase(ecs_world_t* world, pulse_imgui_plugin_state* state);
ecs_entity_t install_imgui_frame_system(ecs_world_t* world, pulse_imgui_plugin_state* state);

// imgui_platform.cpp
bool imgui_platform_init(ecs_world_t* world, pulse_imgui_plugin_state* state);
void imgui_platform_new_frame(pulse_imgui_plugin_state* state);
void imgui_platform_shutdown(pulse_imgui_plugin_state* state);
void install_imgui_input(ecs_world_t* world, pulse_imgui_plugin_state* state);

// imgui_render.cpp
EPulseResult imgui_render_init(PulseAppId app, pulse_imgui_plugin_state* state);
void imgui_render_shutdown(PulseAppId app, pulse_imgui_plugin_state* state);

} // namespace pulse_imgui_internal
