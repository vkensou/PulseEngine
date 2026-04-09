#include "snake_module.h"
#include "snake.h"

void loadSnakeResourcesSystemWrapper(
	flecs::iter& it)
{
	auto world = it.world();
	auto& resourceManagerQuery = world.get_mut<ResourceManager>();
	pulse::command_buffer command_buffer(world);
	loadSnakeResourcesSystem(pulse::res<ResourceManager>(resourceManagerQuery), command_buffer);
}
void initSnakeGameSystemWrapper(
	flecs::iter& it)
{
	auto world = it.world();
	pulse::command_buffer command_buffer(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	initSnakeGameSystem(command_buffer, resourcesQuery);
}
void handleSnakeInputSystemWrapper(
	flecs::iter& it, size_t i
	, const SnakeInput& input
	, Facing4W& direction
	, SnakeMove& move
)
{
	auto world = it.world();
	auto& keyboardStateQuery = world.get<const KeyboardState>();
	handleSnakeInputSystem(pulse::res<const KeyboardState>(keyboardStateQuery), input, direction, move);
}
void scheduleSnakeMoveSystemWrapper(
	flecs::iter& it, size_t i
	, const Facing4W& direction
	, SnakeMove& move
)
{
	auto world = it.world();
	auto& contextQuery = world.get<const UpdateContext>();
	auto snakeMoveIntentEvent = pulse::event_writer<SnakeMoveIntentEvent>(world);
	auto entity = it.entity(i);
	scheduleSnakeMoveSystem(pulse::res<const UpdateContext>(contextQuery), snakeMoveIntentEvent, entity, direction, move);
}
void executeSnakeMoveSystemWrapper(
	pulse::event_reader<SnakeMoveIntentEvent> snakeMoveIntentEvent, flecs::world& world
	, flecs::query<const IsApple, const Position>& appleQuery
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
	onGameOverSystem(gameOverEvent, command_buffer, snakeQuery, appleQuery);
}
void onImguiWrapper(
	flecs::iter& it, size_t i
	, const SnakeGame& snakeGame
	, const Score& score
)
{
	auto world = it.world();
	auto restartEvent = pulse::event_writer<RestartEvent>(world);
	onImguiSystem(snakeGame, score, restartEvent);
}
void restartSystemWrapper(
	pulse::event_reader<RestartEvent> restartEvent, flecs::world& world
)
{
	pulse::command_buffer command_buffer(world);
	auto borderQuery = pulse::singleton_query<const Border>(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	restartSystem(restartEvent, command_buffer, borderQuery, resourcesQuery);
}

void importModule(pulse::ModuleContext* moduleContext)
{
	moduleContext->world.system("LoadSnakeResources")
		.kind(moduleContext->initPipeline)
		.immediate()
		.run(loadSnakeResourcesSystemWrapper);
	moduleContext->world.system("InitSnakeGame")
		.kind(moduleContext->initPipeline)
		.immediate()
		.run(initSnakeGameSystemWrapper);
	moduleContext->world.system<const SnakeInput, Facing4W, SnakeMove>("HandleSnakeInput")
		.kind(moduleContext->updatePipeline)
		.each(handleSnakeInputSystemWrapper);
	moduleContext->world.system<const Facing4W, SnakeMove>("ScheduleSnakeMove")
		.kind(moduleContext->updatePipeline)
		.each(scheduleSnakeMoveSystemWrapper);
	moduleContext->world.system<SnakeBodies>("SyncSnakeBodyPosition")
		.kind(moduleContext->updatePipeline)
		.each(syncSnakeBodyPositionSystemWrapper);
	moduleContext->world.system<const SnakeGame, const Score>("OnImgui")
		.kind(moduleContext->imguiPipeline)
		.immediate()
		.each(onImguiWrapper);
	auto snakeMoveIntentDispatcher = std::make_unique<pulse::EntityEventRegister<SnakeMoveIntentEvent, SnakeBodies>>();
	snakeMoveIntentDispatcher->reg(executeSnakeMoveSystemWrapper, moduleContext->world.query<const IsApple, const Position>());
	snakeMoveIntentDispatcher->observe(moduleContext->world);
	moduleContext->eventManager->register_event(std::move(snakeMoveIntentDispatcher));
	auto appleEatenDispatcher = std::make_unique<pulse::EventRegister<AppleEatenEvent>>();
	appleEatenDispatcher->reg(eatAppleSystemWrapper);
	appleEatenDispatcher->reg(increaseScoreSystemWrapper, moduleContext->world.query<Score>());
	appleEatenDispatcher->reg(spawnAppleSystemWrapper, moduleContext->world.query<const SnakeBodies>());
	appleEatenDispatcher->observe(moduleContext->world);
	moduleContext->eventManager->register_event(std::move(appleEatenDispatcher));
	auto gameOverDispatcher = std::make_unique<pulse::EventRegister<GameOverEvent>>();
	gameOverDispatcher->reg(onGameOverSystemWrapper, moduleContext->world.query<SnakeBodies>(), moduleContext->world.query<IsApple>());
	gameOverDispatcher->observe(moduleContext->world);
	moduleContext->eventManager->register_event(std::move(gameOverDispatcher));
	auto restartDispatcher = std::make_unique<pulse::EventRegister<RestartEvent>>();
	restartDispatcher->reg(restartSystemWrapper);
	restartDispatcher->observe(moduleContext->world);
	moduleContext->eventManager->register_event(std::move(restartDispatcher));
}