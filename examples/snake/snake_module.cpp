#include "snake_module.h"
#include "snake.h"
#include "pulse_window.h"
#include "pulse_input.h"
#include <memory>

// ============================================================
// 状态机
//   注册期把 system/observer 按"启用状态集"登记进状态机单例
//   （StateMachine<SnakeGameState> 以 flecs 单例组件存放在 world 的
//   组件实体上，无文件级全局）；每帧最先运行的状态管理系统（IMMEDIATE）
//   调 tick() 检测状态变化，经 flecs ecs_enable 批量启用/禁用
//   （immediate 使开关当帧生效）。
//   [迁移预留：未来由 generate_module.lua 解析 STATE= /
//    PULSE_ECS_STATE_MACHINE(INIT=...) 自动生成]
// ============================================================

void snakeStateMachineWrapper(
	flecs::iter& it)
{
	auto world = it.world();
	world.get_mut<pulse::StateMachine<SnakeGameState>>().tick(world);
}

struct loadSnakeResourcesSystemWrapperState
{
	flecs::query<PulseWindow, PulsePrimaryWindow> primaryWindowQuery;
};

void loadSnakeResourcesSystemWrapper(
	flecs::iter& it)
{
	auto world = it.world();
	auto app = pulse_get_app_from_world(world.c_ptr());
	auto& assetsQuery = world.get_mut<SnakeAssets>();
	auto state = pulse::system_state_machine<SnakeGameState>(world);
	pulse::command_buffer command_buffer(world);
	auto systemState = it.system().get<loadSnakeResourcesSystemWrapperState>();
	loadSnakeResourcesSystem(app, pulse::res<SnakeAssets>(assetsQuery), state, command_buffer, systemState.primaryWindowQuery);
}
void handleSnakeInputSystemWrapper(
	flecs::iter& it, size_t i
	, const SnakeInput& input
	, Facing4W& direction
	, SnakeMove& move
)
{
	auto world = it.world();
	auto& keyboardQuery = world.get<const PulseKeyboardInput>();
	handleSnakeInputSystem(pulse::res<const PulseKeyboardInput>(keyboardQuery), input, direction, move);
}
void scheduleSnakeMoveSystemWrapper(
	flecs::iter& it, size_t i
	, const Facing4W& direction
	, SnakeMove& move
)
{
	auto world = it.world();
	auto& timerQuery = world.get<const PulseTimer>();
	auto snakeMoveIntentEvent = pulse::event_writer<SnakeMoveIntentEvent>(world);
	auto entity = it.entity(i);
	scheduleSnakeMoveSystem(pulse::res<const PulseTimer>(timerQuery), snakeMoveIntentEvent, entity, direction, move);
}
void executeSnakeMoveSystemWrapper(
	pulse::event_reader<SnakeMoveIntentEvent> snakeMoveIntentEvent, flecs::world& world
	, flecs::query<const IsApple, const PulseLocalTransform>& appleQuery
	, SnakeBodies& snake
)
{
	auto borderQuery = pulse::singleton_query<const Border>(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	auto appleEatenEvent = pulse::event_writer<AppleEatenEvent>(world);
	auto gameOverEvent = pulse::event_writer<GameOverEvent>(world);
	pulse::command_buffer command_buffer(world);
	executeSnakeMoveSystem(snakeMoveIntentEvent, command_buffer, appleQuery, borderQuery, resourcesQuery, appleEatenEvent, gameOverEvent, snake);
}
void syncSnakeBodyPositionSystemWrapper(
	flecs::iter& it, size_t i
	, SnakeBodies& snake
)
{
	auto world = it.world();
	syncSnakeBodyPositionSystem(snake);
}
void eatAppleSystemWrapper(
	pulse::event_reader<AppleEatenEvent> appleEatenEvent, flecs::world& world
)
{
	pulse::command_buffer command_buffer(world);
	eatAppleSystem(appleEatenEvent, command_buffer);
}
void increaseScoreSystemWrapper(
	pulse::event_reader<AppleEatenEvent> appleEatenEvent, flecs::world& world
	, flecs::query<Score>& scoreQuery
)
{
	scoreQuery.each([&](Score& score)
		{
			increaseScoreSystem(appleEatenEvent, score);
		});
}
void spawnAppleSystemWrapper(
	pulse::event_reader<AppleEatenEvent> appleEatenEvent, flecs::world& world
	, flecs::query<const SnakeBodies>& snakeQuery
)
{
	pulse::command_buffer command_buffer(world);
	auto borderQuery = pulse::singleton_query<const Border>(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	spawnAppleSystem(appleEatenEvent, command_buffer, snakeQuery, borderQuery, resourcesQuery);
}
void onGameOverSystemWrapper(
	pulse::event_reader<GameOverEvent> gameOverEvent, flecs::world& world
	, flecs::query<SnakeBodies>& snakeQuery
	, flecs::query<IsApple>& appleQuery
)
{
	pulse::command_buffer command_buffer(world);
	auto state = pulse::system_state_machine<SnakeGameState>(world);
	onGameOverSystem(gameOverEvent, command_buffer, state, snakeQuery, appleQuery);
}
void restartSystemWrapper(
	pulse::event_reader<RestartEvent> restartEvent, flecs::world& world
)
{
	pulse::command_buffer command_buffer(world);
	auto state = pulse::system_state_machine<SnakeGameState>(world);
	auto borderQuery = pulse::singleton_query<const Border>(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	restartSystem(restartEvent, command_buffer, state, borderQuery, resourcesQuery);
}
// 附加查询（生成器 §7：普通 system 的附加 query 走 WrapperState，注册处
// 用 moduleContext->world.query<...>() 创建，与生成器输出格式一致）
struct snakeUISystemWrapperState
{
	flecs::query<PulseWindow, PulsePrimaryWindow> primaryWindowQuery;
};

void snakeUISystemWrapper(
	flecs::iter& it, size_t i
	, const Score& score
)
{
	auto world = it.world();
	auto app = pulse_get_app_from_world(world.c_ptr());
	auto systemState = it.system().get<snakeUISystemWrapperState>();
	auto state = pulse::system_state_machine<SnakeGameState>(world);
	auto& keyboardQuery = world.get<const PulseKeyboardInput>();
	auto restartEvent = pulse::event_writer<RestartEvent>(world);
	snakeUISystem(app, score, systemState.primaryWindowQuery, state, pulse::res<const PulseKeyboardInput>(keyboardQuery), restartEvent);
}

void importModule(pulse::ModuleContext* moduleContext)
{
	// ---- 资源请求容器（请求由 loadSnakeResourcesSystem 自行发起）----
	pulse::registerResource<SnakeAssets>(moduleContext->world, "Snake Assets", SnakeAssets{});

	// ============================================================
	// 系统注册
	// ============================================================

	// 状态机注册表：以 flecs 单例组件存放（组件实体 id 上的数据），非全局变量。
	// 先显式 set 创建单例（本版本 flecs 的 get_mut 要求组件已存在，否则断言）
	moduleContext->world.set<pulse::StateMachine<SnakeGameState>>(pulse::StateMachine<SnakeGameState>{});
	auto& stateMachine = moduleContext->world.get_mut<pulse::StateMachine<SnakeGameState>>();

	// 状态管理系统：每帧最先跑；IMMEDIATE 使其 ecs_enable 结果
	// 对当帧后续系统立即生效（flecs 对 observer 的 disable 同样有效）
	moduleContext->world.system("SnakeStateMachine")
		.kind(moduleContext->updatePipeline)
		.immediate()
		.run(snakeStateMachineWrapper);

	auto loadSnakeResources = moduleContext->world.system("LoadSnakeResources")
		.kind(moduleContext->updatePipeline)
		.run(loadSnakeResourcesSystemWrapper);
	loadSnakeResources.set<loadSnakeResourcesSystemWrapperState>({ .primaryWindowQuery = moduleContext->world.query<PulseWindow, PulsePrimaryWindow>() });
	stateMachine.reg(loadSnakeResources, { SnakeGameState::UnInitialized, SnakeGameState::Loading });

	auto handleSnakeInput = moduleContext->world.system<const SnakeInput, Facing4W, SnakeMove>("HandleSnakeInput")
		.kind(moduleContext->updatePipeline)
		.each(handleSnakeInputSystemWrapper);
	stateMachine.reg(handleSnakeInput, { SnakeGameState::Gaming });

	auto scheduleSnakeMove = moduleContext->world.system<const Facing4W, SnakeMove>("ScheduleSnakeMove")
		.kind(moduleContext->updatePipeline)
		.each(scheduleSnakeMoveSystemWrapper);
	stateMachine.reg(scheduleSnakeMove, { SnakeGameState::Gaming });

	auto syncSnakeBodyPosition = moduleContext->world.system<SnakeBodies>("SyncSnakeBodyPosition")
		.kind(moduleContext->updatePipeline)
		.each(syncSnakeBodyPositionSystemWrapper);
	stateMachine.reg(syncSnakeBodyPosition, { SnakeGameState::Gaming });

	auto snakeUi = moduleContext->world.system<const Score>("SnakeUI")
		.kind(moduleContext->imguiPipeline)
		.each(snakeUISystemWrapper);
	snakeUi.set<snakeUISystemWrapperState>({ .primaryWindowQuery = moduleContext->world.query<PulseWindow, PulsePrimaryWindow>() });
	stateMachine.reg(snakeUi, { SnakeGameState::Gaming, SnakeGameState::GameOver });

	auto snakeMoveIntentDispatcher = std::make_unique<pulse::EntityEventRegister<SnakeMoveIntentEvent, SnakeBodies>>();
	snakeMoveIntentDispatcher->reg(executeSnakeMoveSystemWrapper, moduleContext->world.query<const IsApple, const PulseLocalTransform>());
	stateMachine.reg(snakeMoveIntentDispatcher->observe(moduleContext->world), { SnakeGameState::Gaming });
	moduleContext->eventManager->register_event(std::move(snakeMoveIntentDispatcher));
	auto appleEatenDispatcher = std::make_unique<pulse::EventRegister<AppleEatenEvent>>();
	appleEatenDispatcher->reg(eatAppleSystemWrapper);
	appleEatenDispatcher->reg(increaseScoreSystemWrapper, moduleContext->world.query<Score>());
	appleEatenDispatcher->reg(spawnAppleSystemWrapper, moduleContext->world.query<const SnakeBodies>());
	stateMachine.reg(appleEatenDispatcher->observe(moduleContext->world), { SnakeGameState::Gaming });
	moduleContext->eventManager->register_event(std::move(appleEatenDispatcher));
	auto gameOverDispatcher = std::make_unique<pulse::EventRegister<GameOverEvent>>();
	gameOverDispatcher->reg(onGameOverSystemWrapper, moduleContext->world.query<SnakeBodies>(), moduleContext->world.query<IsApple>());
	stateMachine.reg(gameOverDispatcher->observe(moduleContext->world), { SnakeGameState::Gaming });
	moduleContext->eventManager->register_event(std::move(gameOverDispatcher));
	auto restartDispatcher = std::make_unique<pulse::EventRegister<RestartEvent>>();
	restartDispatcher->reg(restartSystemWrapper);
	stateMachine.reg(restartDispatcher->observe(moduleContext->world), { SnakeGameState::GameOver });
	moduleContext->eventManager->register_event(std::move(restartDispatcher));

	// ---- 状态机初始化：写初值并应用首批开关（run 前 world 可写，立即生效）----
	stateMachine.init(moduleContext->world, SnakeGameState::UnInitialized);
}
