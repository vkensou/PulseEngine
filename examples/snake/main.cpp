// ============================================================
// PulseEngine Snake（新插件体系版）—— 纯宿主
//
// main 对 snake 模块内部一无所知：
//   - 只负责 app 生命周期与插件装配
//   - 游戏的全部注册（组件、相机、资源加载、状态机、UI）
//     都在 importModule（snake_module.cpp）内完成
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

#include "ecsext.hpp"     // pulse::EventCenter
#include "snake_module.h" // importModule

int main(void)
{
	PulseAppId app = pulse_create_app("pulse-snake-example");
	assert(app != nullptr);

	// ---- 插件 ----
	assert(pulse_add_input_plugin(app) == PULSE_RESULT_OK);

	auto window_desc = pulse_window_plugin_desc_default();
	window_desc.primary_window.title = "PulseEngine Snake";
	window_desc.primary_window.width = 800;
	window_desc.primary_window.height = 600;
	window_desc.primary_window.resizable = false;
	assert(pulse_add_window_plugin(app, &window_desc) == PULSE_RESULT_OK);

	PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
	asset_desc.root_path = "examples/snake/assets";
	assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_RESULT_OK);

	assert(pulse_add_transform_plugin(app) == PULSE_RESULT_OK);

    const char* per_draw_shader_properties[] = {
        "wMatrix",
    };

    auto graphics_desc = pulse_graphics_plugin_desc_default();
	graphics_desc.enable_debug_layer = true;
	graphics_desc.enable_gpu_based_validation = true;
    graphics_desc.per_draw_shader_property_count = 1;
    graphics_desc.p_per_draw_shader_properties = per_draw_shader_properties;
    assert(pulse_add_graphics_plugin(app, &graphics_desc) == PULSE_RESULT_OK);

	assert(pulse_add_renderer_plugin(app) == PULSE_RESULT_OK);

	// ---- world ----
	flecs::world world = flecs::world(pulse_app_world(app));

	// ---- 事件系统基础 ----
	auto singleHolder = world.singleton<pulse::SingleHolder>();
	singleHolder.add<pulse::EventTag>();

	// ---- 把 C++ 类型绑定到插件注册的 C 组件 id，模块才能用 C++ API 设置 ----
	// pulse-app
	world.component<PulseTimer>("PulseTimer", true, ecs_id(PulseTimer));
	// pulse-input
	world.component<PulseKeyboardInput>("PulseKeyboardInput", true, ecs_id(PulseKeyboardInput));
	world.component<PulseMouseInput>("PulseMouseInput", true, ecs_id(PulseMouseInput));
	world.component<PulseMouseMotion>("PulseMouseMotion", true, ecs_id(PulseMouseMotion));
	world.component<PulseMouseScroll>("PulseMouseScroll", true, ecs_id(PulseMouseScroll));
	world.component<PulseKeyEvent>("PulseKeyEvent", true, ecs_id(PulseKeyEvent));
	world.component<PulseMouseButtonEvent>("PulseMouseButtonEvent", true, ecs_id(PulseMouseButtonEvent));
	world.component<PulseMouseScrollEvent>("PulseMouseScrollEvent", true, ecs_id(PulseMouseScrollEvent));
	// pulse-window
	world.component<PulseWindow>("PulseWindow", true, ecs_id(PulseWindow));
	world.component<PulseSdlWindow>("PulseSdlWindow", true, ecs_id(PulseSdlWindow));
	world.component<PulsePrimaryWindow>("PulsePrimaryWindow", true, ecs_id(PulsePrimaryWindow));
	// pulse-graphics
	world.component<PulseRenderer>("PulseRenderer", true, ecs_id(PulseRenderer));
	world.component<PulseSurface>("PulseSurface", true, ecs_id(PulseSurface));
	world.component<PulseSwapchain>("PulseSwapchain", true, ecs_id(PulseSwapchain));
	// pulse-transform
	world.component<PulseLocalTransform>("PulseLocalTransform", true, ecs_id(PulseLocalTransform));
	world.component<PulseWorldTransform>("PulseWorldTransform", true, ecs_id(PulseWorldTransform));
	world.component<PulseShowMatrix>("PulseShowMatrix", true, ecs_id(PulseShowMatrix));
	// pulse-renderer
	world.component<PulseCamera>("PulseCamera", true, ecs_id(PulseCamera));
	world.component<PulseLight>("PulseLight", true, ecs_id(PulseLight));
	world.component<PulseRenderable>("PulseRenderable", true, ecs_id(PulseRenderable));

	// ---- 游戏模块 ----
	pulse::EventCenter eventCenter;
	pulse::ModuleContext moduleContext = {
		.world = world,
		.initPipeline = flecs::OnStart,
		.updatePipeline = flecs::OnUpdate,
		.postUpdatePipeline = flecs::PostUpdate,
		.renderPipeline = flecs::OnStore,
		.imguiPipeline = flecs::OnUpdate,
		.eventManager = &eventCenter,
	};
	importModule(&moduleContext);

	pulse_app_run(app);

	pulse_destroy_app(app);
	printf("Snake example exited.\n");
	return 0;
}
