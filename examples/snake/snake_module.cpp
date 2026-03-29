#include "snake_module.h"
#include "snake.h"

void loadSnakeResourcesSystemWrapper(flecs::iter& it)
{
	auto world = it.world();
	pulse::command_buffer command_buffer(world);
    SystemContext& app = *(SystemContext*)world.get_ctx();
	loadSnakeResourcesSystem(pulse::res<ResourceManager>(app.resourceManager), command_buffer);
}

void initSnakeGameSystemWrapper(flecs::iter& it)
{
	auto world = it.world();
	pulse::command_buffer command_buffer(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	initSnakeGameSystem(command_buffer, resourcesQuery);
}

void handleSnakeInputSystemWrapper(flecs::iter& it, size_t i, const SnakeInput& input, Facing4W& direction, SnakeMove& move)
{
	auto world = it.world();
	SystemContext& app = *(SystemContext*)world.get_ctx();
	handleSnakeInputSystem(pulse::res<const KeyboardState>(app.keyboardState), input, direction, move);
}

void scheduleSnakeMoveSystemWrapper(flecs::iter& it, size_t i, const Facing4W& direction, SnakeMove& move)
{
	auto world = it.world();
	auto entity = it.entity(i);
	SystemContext& app = *(SystemContext*)world.get_ctx();
	pulse::res<const oval_update_context> updateContext(app.updateContext);
	auto snakeMoveWriter = pulse::event_writer<SnakeMoveIntentEvent>(world);
	scheduleSnakeMoveSystem(updateContext, snakeMoveWriter, entity, direction, move);
}

void executeSnakeMoveSystemWrapper(pulse::event_reader<SnakeMoveIntentEvent> eventReader, flecs::world& world, flecs::query<const IsApple, const Position> apple, SnakeBodies& snake)
{
	auto borderQuery = pulse::singleton_query<const Border>(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	auto appleEatWriter = pulse::event_writer<AppleEatenEvent>(world);
	auto gameoverWriter = pulse::event_writer<GameOverEvent>(world);
	pulse::command_buffer command_buffer(world);
	executeSnakeMoveSystem(eventReader, command_buffer, apple, borderQuery, resourcesQuery, appleEatWriter, gameoverWriter, snake);
}

void eatAppleSystemWrapper(pulse::event_reader<AppleEatenEvent> eventReader, flecs::world& world)
{
	pulse::command_buffer command_buffer(world);
	eatAppleSystem(eventReader, command_buffer);
}

void increaseScoreSystemWrapper(pulse::event_reader<AppleEatenEvent> eventReader, flecs::world& world, flecs::query<Score>& scores)
{
	scores.each([&](Score& score)
		{
			increaseScoreSystem(eventReader, score);
		});
}

void spawnAppleSystemWrapper(pulse::event_reader<AppleEatenEvent> eventReader, flecs::world& world, flecs::query<const SnakeBodies>& snakeQuery)
{
	pulse::command_buffer command_buffer(world);
	auto borderQuery = pulse::singleton_query<const Border>(world);
	auto resourcesQuery = pulse::singleton_query<const SnakeResources>(world);
	spawnAppleSystem(eventReader, command_buffer, snakeQuery, borderQuery, resourcesQuery);
}

void onGameOverSystemWrapper(pulse::event_reader<GameOverEvent> eventReader, flecs::world& world, flecs::query<SnakeBodies>& snakeQuery, flecs::query<IsApple>& appleQuery)
{
	pulse::command_buffer command_buffer(world);
	onGameOverSystem(eventReader, command_buffer, snakeQuery, appleQuery);
}

void onImguiWrapper(flecs::iter& it, size_t i, const SnakeGame& snakeGame, const Score& score)
{
	auto world = it.world();
	auto restartWriter = pulse::event_writer<RestartEvent>(world);
	onImguiSystem(snakeGame, score, restartWriter);
}

void restartSystemWrapper(pulse::event_reader<RestartEvent> restartEvent, flecs::world& world)
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

	moduleContext->world.system("SnakeInit")
		.kind(moduleContext->initPipeline)
		.immediate()
		.run(initSnakeGameSystemWrapper);

	moduleContext->world.system<const SnakeInput, Facing4W, SnakeMove>("SnakeInput")
		.kind(moduleContext->updatePipeline)
		.each(handleSnakeInputSystemWrapper);

	moduleContext->world.system<const Facing4W, SnakeMove>("ScheduleSnakeMove")
		.kind(moduleContext->updatePipeline)
		.each(scheduleSnakeMoveSystemWrapper);

	moduleContext->world.system<SnakeBodies>("SyncSnakeBodyPosition")
		.kind(moduleContext->updatePipeline)
		.each(syncSnakeBodyPositionSystem);

	moduleContext->world.system<const SnakeGame, const Score>("SnakeUI")
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
