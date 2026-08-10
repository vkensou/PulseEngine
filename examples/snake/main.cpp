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

	auto graphics_desc = pulse_graphics_plugin_desc_default();
	graphics_desc.enable_debug_layer = true;
	graphics_desc.enable_gpu_based_validation = true;
	assert(pulse_add_graphics_plugin(app, &graphics_desc) == PULSE_RESULT_OK);

	assert(pulse_add_renderer_plugin(app) == PULSE_RESULT_OK);

	// ---- world ----
	flecs::world world = flecs::world(pulse_app_world(app));

	// ---- 事件系统基础 ----
	auto singleHolder = world.singleton<pulse::SingleHolder>();
	singleHolder.add<pulse::EventTag>();

	// ---- 把 C++ 类型绑定到插件注册的 C 组件 id，模块才能用 C++ API 设置 ----
	world.component<PulseLocalTransform>("PulseLocalTransform", true, ecs_id(PulseLocalTransform));
	world.component<PulseRenderable>("PulseRenderable", true, ecs_id(PulseRenderable));
	world.component<PulseCamera>("PulseCamera", true, ecs_id(PulseCamera));
	world.component<PulseWindow>("PulseWindow", true, ecs_id(PulseWindow));
	world.component<PulsePrimaryWindow>("PulsePrimaryWindow", true, ecs_id(PulsePrimaryWindow));
	world.component<PulseKeyboardInput>("PulseKeyboardInput", true, ecs_id(PulseKeyboardInput));

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
