#include "snake_module.h"
#include "snake.h"

void prepareGameSystemWrapper(
	flecs::iter& it)
{
	auto world = it.world();
	auto& assetsQuery = world.get_mut<SnakeAssets>();
	pulse::command_buffer command_buffer(world);
	prepareGameSystem(pulse::res<SnakeAssets>(assetsQuery), command_buffer);
}
void handleSnakeInputSystemWrapper(
	flecs::iter& it, size_t i
	, const SnakeInput& input
	, Facing4W& direction
	, SnakeMove& move
)
{
	auto world = it.world();
	auto& assetsQuery = world.get<const SnakeAssets>();
	handleSnakeInputSystem(pulse::res<const SnakeAssets>(assetsQuery), input, direction, move);
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
	onGameOverSystem(gameOverEvent, command_buffer, snakeQuery, appleQuery);
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
	moduleContext->world.system("PrepareGame")
		.kind(moduleContext->updatePipeline)
		.run(prepareGameSystemWrapper);
	moduleContext->world.system<const SnakeInput, Facing4W, SnakeMove>("HandleSnakeInput")
		.kind(moduleContext->updatePipeline)
		.each(handleSnakeInputSystemWrapper);
	moduleContext->world.system<const Facing4W, SnakeMove>("ScheduleSnakeMove")
		.kind(moduleContext->updatePipeline)
		.each(scheduleSnakeMoveSystemWrapper);
	moduleContext->world.system<SnakeBodies>("SyncSnakeBodyPosition")
		.kind(moduleContext->updatePipeline)
		.each(syncSnakeBodyPositionSystemWrapper);
	auto snakeMoveIntentDispatcher = std::make_unique<pulse::EntityEventRegister<SnakeMoveIntentEvent, SnakeBodies>>();
	snakeMoveIntentDispatcher->reg(executeSnakeMoveSystemWrapper, moduleContext->world.query<const IsApple, const PulseLocalTransform>());
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