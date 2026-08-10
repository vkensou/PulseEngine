#include "snake_module.h"
#include "snake.h"

static void snakeGameStateMachineWrapper(
	flecs::iter& it)
{
	auto world = it.world();
	world.get_mut<pulse::StateMachine<SnakeGameState>>().tick(world);
}
struct loadSnakeResourcesSystemWrapperState
{
	flecs::query<PulseWindow, PulsePrimaryWindow> primaryWindowQuery;
};
static void loadSnakeResourcesSystemWrapper(
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
static void handleSnakeInputSystemWrapper(
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
static void scheduleSnakeMoveSystemWrapper(
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
static void executeSnakeMoveSystemWrapper(
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
static void syncSnakeBodyPositionSystemWrapper(
	flecs::iter& it, size_t i
	, SnakeBodies& snake
)
{
	auto world = it.world();
	syncSnakeBodyPositionSystem(snake);
}
static void eatAppleSystemWrapper(
	pulse::event_reader<AppleEatenEvent> appleEatenEvent, flecs::world& world
)
{
	pulse::command_buffer command_buffer(world);
	eatAppleSystem(appleEatenEvent, command_buffer);
}
static void increaseScoreSystemWrapper(
	pulse::event_reader<AppleEatenEvent> appleEatenEvent, flecs::world& world
	, flecs::query<Score>& scoreQuery
)
{
	scoreQuery.each([&](Score& score)
		{
			increaseScoreSystem(appleEatenEvent, score);
		});
}
static void spawnAppleSystemWrapper(
	pulse::event_reader<AppleEatenEvent> appleEatenEvent, flecs::world& world
	, flecs::query<const SnakeBodies>& snakeQuery
)
{
	pulse::command_buffer command_buffer(world);
	auto borderQuery = pulse::singleton_query<const Border>(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	spawnAppleSystem(appleEatenEvent, command_buffer, snakeQuery, borderQuery, resourcesQuery);
}
static void onGameOverSystemWrapper(
	pulse::event_reader<GameOverEvent> gameOverEvent, flecs::world& world
	, flecs::query<SnakeBodies>& snakeQuery
	, flecs::query<IsApple>& appleQuery
)
{
	pulse::command_buffer command_buffer(world);
	auto state = pulse::system_state_machine<SnakeGameState>(world);
	onGameOverSystem(gameOverEvent, command_buffer, state, snakeQuery, appleQuery);
}
static void restartSystemWrapper(
	pulse::event_reader<RestartEvent> restartEvent, flecs::world& world
)
{
	pulse::command_buffer command_buffer(world);
	auto borderQuery = pulse::singleton_query<const Border>(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	auto state = pulse::system_state_machine<SnakeGameState>(world);
	restartSystem(restartEvent, command_buffer, state, borderQuery, resourcesQuery);
}
struct snakeUISystemWrapperState
{
	flecs::query<PulseWindow, PulsePrimaryWindow> primaryWindowQuery;
};
static void snakeUISystemWrapper(
	flecs::iter& it, size_t i
	, const Score& score
)
{
	auto world = it.world();
	auto app = pulse_get_app_from_world(world.c_ptr());
	auto state = pulse::system_state_machine<SnakeGameState>(world);
	auto& keyboardQuery = world.get<const PulseKeyboardInput>();
	auto restartEvent = pulse::event_writer<RestartEvent>(world);
	auto systemState = it.system().get<snakeUISystemWrapperState>();
	snakeUISystem(app, score, systemState.primaryWindowQuery, state, pulse::res<const PulseKeyboardInput>(keyboardQuery), restartEvent);
}

void importModule(pulse::ModuleContext* moduleContext)
{
	pulse::registerResource<SnakeAssets>(moduleContext->world, "Snake Assets", SnakeAssets{});
	moduleContext->world.set<pulse::StateMachine<SnakeGameState>>(pulse::StateMachine<SnakeGameState>{});
	auto& stateMachine = moduleContext->world.get_mut<pulse::StateMachine<SnakeGameState>>();
	moduleContext->world.system("SnakeGameStateMachine")
		.kind(moduleContext->updatePipeline)
		.immediate()
		.run(snakeGameStateMachineWrapper);
	auto loadSnakeResourcesSystem = moduleContext->world.system("LoadSnakeResources")
		.kind(moduleContext->updatePipeline)
		.run(loadSnakeResourcesSystemWrapper);
	loadSnakeResourcesSystem.set<loadSnakeResourcesSystemWrapperState>({ .primaryWindowQuery = moduleContext->world.query<PulseWindow, PulsePrimaryWindow>() });
	stateMachine.reg(loadSnakeResourcesSystem, { SnakeGameState::UnInitialized, SnakeGameState::Loading });
	auto handleSnakeInputSystem = moduleContext->world.system<const SnakeInput, Facing4W, SnakeMove>("HandleSnakeInput")
		.kind(moduleContext->updatePipeline)
		.each(handleSnakeInputSystemWrapper);
	stateMachine.reg(handleSnakeInputSystem, { SnakeGameState::Gaming });
	auto scheduleSnakeMoveSystem = moduleContext->world.system<const Facing4W, SnakeMove>("ScheduleSnakeMove")
		.kind(moduleContext->updatePipeline)
		.each(scheduleSnakeMoveSystemWrapper);
	stateMachine.reg(scheduleSnakeMoveSystem, { SnakeGameState::Gaming });
	auto syncSnakeBodyPositionSystem = moduleContext->world.system<SnakeBodies>("SyncSnakeBodyPosition")
		.kind(moduleContext->updatePipeline)
		.each(syncSnakeBodyPositionSystemWrapper);
	stateMachine.reg(syncSnakeBodyPositionSystem, { SnakeGameState::Gaming });
	auto snakeUISystem = moduleContext->world.system<const Score>("SnakeUI")
		.kind(moduleContext->imguiPipeline)
		.each(snakeUISystemWrapper);
	snakeUISystem.set<snakeUISystemWrapperState>({ .primaryWindowQuery = moduleContext->world.query<PulseWindow, PulsePrimaryWindow>() });
	stateMachine.reg(snakeUISystem, { SnakeGameState::Gaming, SnakeGameState::GameOver });
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
	stateMachine.init(moduleContext->world, SnakeGameState::UnInitialized);
}