// ============================================================
// PulseEngine Snake（新插件体系版）
//
// 游戏模块 snake.h / snake.cpp 使用原生 pulse API：
//   - 资源：main 只发起异步加载请求并填入 SnakeAssets 单例，
//     就绪后的初始化由模块内的 prepareGameSystem 完成
//   - 渲染：实体直接挂 PulseLocalTransform / PulseRenderable，
//     由 pulse_renderer 插件自动渲染
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

#include "snake.h"        // 游戏模块
#include "snake_module.h" // importModule

static PulseAppId g_app = nullptr;

// ============================================================
// 辅助系统
// ============================================================

// ============================================================
// UI 系统：替代 legacy 的 onImguiSystem（无需 imgui）
//   分数 → 窗口标题栏；GameOver 后按 Enter/R 发 RestartEvent
// ============================================================

static ecs_entity_t g_ui_window_entity = 0;

static void uiSystem(flecs::iter& it, size_t i, const SnakeGame& snakeGame, const Score& score)
{
	(void)i;
	auto world = it.world();

	// 分数 → 标题栏（经组件同步到窗口，勿裸调 SDL_SetWindowTitle，
	// 否则会被 PulseWindowPostFrameSystem 以组件 title 为准覆盖）
	if (g_ui_window_entity)
	{
		char title[192];
		if (snakeGame.playing)
			snprintf(title, sizeof(title), "Snake - Score: %d", score.value);
		else
			snprintf(title, sizeof(title), "Game Over! Score: %d - Press Enter to Restart", score.value);
		pulse_window_set_title(g_app, g_ui_window_entity, title);
	}

	// GameOver 后按 Enter/R 重启
	if (!snakeGame.playing)
	{
		if (pulse_input_key_just_pressed(g_app, SDL_SCANCODE_RETURN) ||
			pulse_input_key_just_pressed(g_app, SDL_SCANCODE_R))
		{
			printf("Restart!\n");
			pulse::event_writer<RestartEvent> restartEvent(world);
			restartEvent.broadcast();
		}
	}
}

static void installAdapterSystems(flecs::world& world)
{
	// 替代 legacy onImguiSystem：标题栏分数 + Enter/R 重启
	world.system<const SnakeGame, const Score>("SnakeUISystem")
		.kind(flecs::OnUpdate)
		.each(uiSystem);
}

// ============================================================
// main
// ============================================================

static void requestAssets(PulseAppId app, PulseShaderRequest& out_shader, PulseMeshRequest& out_mesh)
{
	// ---- shader（异步，不等待）----
	// 注意：pulse_create_shader_from_file 异步持有 desc 指针（loader 在后续
	// 帧才使用），desc 及其引用的数组/状态必须长期有效（static）——
	// 栈上临时对象会在 requestAssets 返回后悬垂，导致随机内存损坏。
	static CGPUBlendAttachmentState blend_attachments = {
		.enable = false,
		.src_factor = CGPU_BLEND_FACTOR_ONE,
		.dst_factor = CGPU_BLEND_FACTOR_ZERO,
		.src_alpha_factor = CGPU_BLEND_FACTOR_ONE,
		.dst_alpha_factor = CGPU_BLEND_FACTOR_ZERO,
		.blend_op = CGPU_BLEND_OP_ADD,
		.blend_alpha_op = CGPU_BLEND_OP_ADD,
		.color_mask = CGPU_COLOR_MASK_RGBA,
	};
	static PulseShaderProperty shader_props[] = {
		{ .name = "vpMatrix", .type = PULSE_SHADER_PROPERTY_TYPE_MAT4,   .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL, .set = 0, .binding = 0, .offset = 0, .size = 64 },
		{ .name = "albedo",   .type = PULSE_SHADER_PROPERTY_TYPE_FLOAT4, .role = PULSE_SHADER_PROPERTY_ROLE_MATERIAL,     .set = 1, .binding = 0, .offset = 0, .size = 16 },
		{ .name = "wMatrix",  .type = PULSE_SHADER_PROPERTY_TYPE_MAT4,   .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL, .set = 2, .binding = 0, .offset = 0, .size = 64 },
	};
	static PulseShaderCreateFromFileDesc shader_desc = {
		.vert_path = "color.vert.spv",
		.frag_path = "color.frag.spv",
		.blend_desc = {
			.attachment_count = 1,
			.p_attachments = &blend_attachments,
			.alpha_to_coverage = false,
			.independent_blend = false,
		},
		.depth_desc = {
			.depth_test = true,
			.depth_write = true,
			.depth_op = CGPU_COMPARE_OP_GREATER_EQUAL,
			.stencil_test = false,
		},
		.rasterizer_state = {
			.cull_mode = CGPU_CULL_MODE_BACK,
			.front_face = CGPU_FRONT_FACE_CLOCK_WISE,
		},
		.property_count = 3,
		.p_properties = shader_props,
	};
	out_shader = pulse_create_shader_from_file(app, &shader_desc);

	// ---- mesh（异步，不等待）----
	out_mesh = pulse_load_mesh(app, "Quad.obj");
}

int main(void)
{
	PulseAppId app = pulse_create_app("pulse-snake-example");
	assert(app != nullptr);
	g_app = app;

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

	// ---- 发起资源加载请求（异步，模块内等待就绪）----
	PulseShaderRequest shader_request{};
	PulseMeshRequest mesh_request{};
	requestAssets(app, shader_request, mesh_request);

	// ---- world ----
	flecs::world world = flecs::world(pulse_app_world(app));

	// 把 C++ 类型绑定到插件注册的 C 组件 id，游戏模块才能用 C++ API 设置
	world.component<PulseLocalTransform>("PulseLocalTransform", true, ecs_id(PulseLocalTransform));
	world.component<PulseRenderable>("PulseRenderable", true, ecs_id(PulseRenderable));
	world.component<PulseCamera>("PulseCamera", true, ecs_id(PulseCamera));

	// 显式注册游戏组件（必须在 run 前、主 world 可写时注册——
	// 若等到系统里（stage）首次使用才自动注册，会在 readonly 状态下
	// 修改组件注册表，导致随机内存损坏/abort）
	world.component<SnakeBodies>();
	world.component<Facing4W>();
	world.component<SnakeMove>();
	world.component<SnakeInput>();
	world.component<SnakeGame>();
	world.component<Border>();
	world.component<Score>();
	world.component<SnakeResources>();
	world.component<IsApple>();
	world.component<AppleEatenEvent>();
	world.component<SnakeMoveIntentEvent>();
	world.component<GameOverEvent>();
	world.component<RestartEvent>();

	// ---- 相机（新引擎渲染路径）----
	auto camera = world.entity("camera");
	camera.set<PulseLocalTransform>({ .translation = HMM_V3(0.5f, 0.5f, -38.f),
									  .rotation = HMM_Q_Identity, .scale = HMM_V3_One });
	camera.set<PulseCamera>({ .window_entity = pulse_window_get_primary(app),
							  .fov = 45.f, .near_plane = 0.1f, .far_plane = 1000.f });

	// ---- 事件系统基础（legacy main 同款）----
	auto snakeApp = world.singleton<pulse::SingleHolder>();
	snakeApp.add<pulse::EventTag>();

	// ---- 资源请求单例（main 只填请求，就绪判断在模块内）----
	SnakeAssets snakeAssets = {
		.app = app,
		.shader = shader_request,
		.mesh = mesh_request,
	};
	pulse::registerResource<SnakeAssets>(world, "Snake Assets", std::move(snakeAssets));

	// ---- 适配系统 ----
	g_ui_window_entity = pulse_window_get_primary(app);
	installAdapterSystems(world);

	// ---- 游戏模块（wrapper + 注册器，legacy 生成规则）----
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