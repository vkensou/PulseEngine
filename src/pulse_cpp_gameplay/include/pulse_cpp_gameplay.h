#pragma once

// ============================================================
// pulse_gameplay：C++ gameplay module 接入辅助头
//
// 这里集中放 C++ 游戏模块接入需要的：
//   - pulse:: 基础设施（ecsext.hpp）
//   - ModuleContext（module_importer.h）
//   - 常用接入 helper（初始化事件基础、构造 ModuleContext、插件状态等）
//
// 注意：各插件内部的 register_components 已经负责把 C 组件 id 绑定到 C++ 类型，
//       gameplay module 无需再手动绑定。
// ============================================================

#include "ecsext.hpp"
#include "module_importer.h"
#include "pulse_app.h"

namespace pulse {

// C++ gameplay 插件常用的运行时状态：持有事件中心，随插件生命周期存活。
struct GameplayModuleState
{
    EventCenter eventCenter;
};

// 初始化 ecsext.hpp 依赖的事件单例基础。
inline void init_gameplay_base(flecs::world& world)
{
    auto singleHolder = world.singleton<pulse::SingleHolder>();
    singleHolder.add<pulse::EventTag>();
}

// 把 pulse_app_add_plugin 的结果转换为 package 注册结果。
inline EPulseResult to_package_result(EPulseAppAddPluginResult result)
{
    switch (result) {
        case PULSE_APP_ADD_PLUGIN_RESULT_OK:
            return PULSE_RESULT_OK;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT:
            return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_STATE:
            return PULSE_RESULT_ERROR_INVALID_STATE;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN:
            return PULSE_RESULT_ERROR_DUPLICATE_PLUGIN;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_PLUGIN_BUILD_FAILED:
            return PULSE_RESULT_ERROR_INTERNAL;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY:
            return PULSE_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY:
            return PULSE_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY;
        default:
            return PULSE_RESULT_ERROR_INTERNAL;
    }
}

// 构造标准 ModuleContext。
inline ModuleContext make_module_context(
    flecs::world world,
    flecs::entity_t imgui_phase,
    EventCenter* event_manager)
{
    return ModuleContext{
        .world = world,
        .initPipeline = flecs::OnStart,
        .updatePipeline = flecs::OnUpdate,
        .postUpdatePipeline = flecs::PostUpdate,
        .renderPipeline = flecs::OnStore,
        .imguiPipeline = imgui_phase,
        .eventManager = event_manager,
    };
}

} // namespace pulse