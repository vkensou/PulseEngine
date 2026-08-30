#include "imgui_internal.h"

namespace pulse_imgui_internal {

namespace {

void sync_display_size(const PulseWindow* window) {
    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2((float)window->width, (float)window->height);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
}

void sync_input(pulse_imgui_plugin_state* state) {
    ImGuiIO& io = ImGui::GetIO();

    // 修饰键：imgui 需要持续状态。优先 SDL 实时状态，
    // 输入插件未注册时退化到不发送（业务上 pulse_imgui 应在 pulse_input 之后安装）。
    io.AddKeyEvent(
        ImGuiMod_Ctrl,
        (SDL_GetModState() & SDL_KMOD_CTRL) != 0);
    io.AddKeyEvent(
        ImGuiMod_Shift,
        (SDL_GetModState() & SDL_KMOD_SHIFT) != 0);
    io.AddKeyEvent(
        ImGuiMod_Alt,
        (SDL_GetModState() & SDL_KMOD_ALT) != 0);
    io.AddKeyEvent(
        ImGuiMod_Super,
        (SDL_GetModState() & SDL_KMOD_GUI) != 0);
}

// 字体纹理走 pulse_graphics 异步上传管线，就绪后借出 PulseTextureData 供绘制使用。
void poll_font_texture(pulse_imgui_plugin_state* state) {
    if (state->font_ready) {
        return;
    }
    if (!pulse_texture_is_alive(state->app, state->font_request)) {
        return;
    }
    if (!pulse_texture_is_ready(state->app, state->font_request)) {
        return;
    }

    PulseTextureHandle handle = pulse_texture_get_handle(state->app, state->font_request);
    if (!pulse_asset_handle_is_valid(pulse_texture_to_handle(handle))) {
        return;
    }

    state->font_texture = handle;
    state->font_ready = true;
}

// EcsPreUpdate 拆成 2 个系统，按依赖顺序执行：
//   1. PulseImguiDisplaySyncSystem：查询窗口组件 + PulseImguiContext，
//      同步 io.DisplaySize / FramebufferScale。
//   2. PulseImguiBeginSystem：同步输入/光标，然后打开 imgui 帧窗口（NewFrame）。
// 帧窗口一直开到 EcsPreStore（end 系统 Render 收帧），期间任何常规阶段
// （OnUpdate/PostUpdate，或挂 PulseImguiPhase）的用户系统都能直接画控件——
// 与 bevy_mod_imgui 的模型一致（NewFrame 在 PreUpdate、Render 在最后）。
void imgui_display_sync_system_run(ecs_iter_t* it) {
    PulseWindow* window = ecs_field(it, PulseWindow, 0);
    PulseImguiContext* imgui_context = ecs_field(it, PulseImguiContext, 1);

    if (!imgui_context || !imgui_context->context) {
        return;
    }

    ImGui::SetCurrentContext(imgui_context->context);
    sync_display_size(window);
}

void imgui_begin_system_run(ecs_iter_t* it) {
    pulse_imgui_state_resource* resource =
        ecs_field(it, pulse_imgui_state_resource, 0);
    pulse_imgui_plugin_state* state = resource ? resource->state : nullptr;
    if (!state || !state->app || !state->context) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();

    state->draw_data = nullptr;
    sync_input(state);
    imgui_platform_new_frame(state);
    poll_font_texture(state);

    // 用稳态时钟自算 DeltaTime（不依赖 PulseTimer：PreUpdate 阶段内
    // 与 PulseTimeSystem 无顺序保证，避免滞后一帧）。
    auto now = std::chrono::steady_clock::now();
    float delta = std::chrono::duration<float>(now - state->last_frame_time).count();
    state->last_frame_time = now;
    if (delta > 0.0f && delta < 1.0f) {
        io.DeltaTime = delta;
    }

    ImGui::NewFrame();
    state->frame_open = true;
}

void imgui_end_system_run(ecs_iter_t* it) {
    pulse_imgui_state_resource* resource =
        ecs_field(it, pulse_imgui_state_resource, 0);
    pulse_imgui_plugin_state* state = resource ? resource->state : nullptr;
    if (!state || !state->context || !state->frame_open) {
        return;
    }

    ImGui::SetCurrentContext(state->context);
    ImGui::Render();
    state->draw_data = ImGui::GetDrawData();

    state->frame_open = false;
}

ecs_entity_t make_phase(
    ecs_world_t* world,
    const char* name,
    ecs_entity_t depends_on
) {
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = name;
    ecs_entity_t phase = ecs_entity_init(world, &entity_desc);
    if (!phase) {
        return 0;
    }
    // 与内置 phase 相同的构成：EcsPhase 组件 + (EcsDependsOn, 父 phase)。
    // 主 pipeline 会匹配所有带 (EcsPhase, *) pair 的系统并按 DependsOn 级联排序。
    ecs_add_id(world, phase, EcsPhase);
    if (depends_on) {
        ecs_add_pair(world, phase, EcsDependsOn, depends_on);
    }
    return phase;
}

ecs_entity_t install_system(
    ecs_world_t* world,
    const char* name,
    ecs_entity_t phase,
    ecs_iter_action_t callback
) {
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = name;
    ecs_entity_t system_entity = ecs_entity_init(world, &entity_desc);

    ecs_system_desc_t system_desc{};
    system_desc.entity = system_entity;
    system_desc.phase = phase;
    // 直接查询 imgui 状态单例，不再通过 ctx 传裸指针。
    system_desc.query.terms[0].id = ecs_id(pulse_imgui_state_resource);
    system_desc.query.cache_kind = EcsQueryCacheAuto;
    system_desc.callback = callback;
    return ecs_system_init(world, &system_desc);
}

ecs_entity_t install_display_sync_system(ecs_world_t* world) {
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "PulseImguiDisplaySyncSystem";
    ecs_entity_t system_entity = ecs_entity_init(world, &entity_desc);

    ecs_system_desc_t system_desc{};
    system_desc.entity = system_entity;
    system_desc.phase = EcsPreUpdate;
    system_desc.query.terms[0].id = ecs_id(PulseWindow);
    system_desc.query.terms[1].id = ecs_id(PulseImguiContext);
    system_desc.query.cache_kind = EcsQueryCacheAuto;
    system_desc.callback = imgui_display_sync_system_run;
    return ecs_system_init(world, &system_desc);
}

} // namespace

ecs_entity_t create_imgui_phase(ecs_world_t* world, pulse_imgui_plugin_state* state) {
    if (!world || !state || state->imgui_phase) {
        return state ? state->imgui_phase : 0;
    }

    // 推荐专用 phase：depends on EcsPostUpdate（游戏逻辑之后、渲染之前）。
    // 注意：帧窗口从 PreUpdate 就打开了，用户系统挂普通阶段（OnUpdate 等）
    // 同样可以画控件，此 phase 只是把"画在游戏逻辑之后"固化成显式约定。
    ecs_entity_t phase = make_phase(world, "PulseImguiPhase", EcsPostUpdate);
    // 必须加这一句，这样可以使phase是PulseImguiPhase的system稳定的在PulseImguiEndSystem之前运行
    ecs_add_pair(world, EcsPreStore, EcsDependsOn, phase);
    if (!phase) {
        return 0;
    }
    state->imgui_phase = phase;

    return phase;
}

ecs_entity_t install_imgui_frame_system(ecs_world_t* world, pulse_imgui_plugin_state* state) {
    if (!world || !state || state->frame_system) {
        return state ? state->frame_system : 0;
    }

    // PreUpdate 内先同步窗口尺寸，再同步输入/打开 NewFrame。
    ecs_entity_t display_sync_system = install_display_sync_system(world);
    state->frame_system = install_system(
        world, "PulseImguiBeginSystem", EcsPreUpdate, imgui_begin_system_run);
    ecs_add_pair(world, state->frame_system, EcsDependsOn, display_sync_system);

    // End（Render）在渲染链（OnStore）之前收帧。
    install_system(
        world, "PulseImguiEndSystem", EcsPreStore, imgui_end_system_run);

    return state->frame_system;
}

} // namespace pulse_imgui_internal
