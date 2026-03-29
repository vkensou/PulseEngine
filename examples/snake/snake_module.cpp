#include "snake_module.h"
//#include "predefine_components.h"
//#include "framework.h"
#include "snake.h"

struct SystemContext
{
	KeyboardState keyboardState;
	oval_update_context updateContext;
};

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

static void registerSnakeSystems(flecs::world& world)
{
	world.system<const SnakeInput, Facing4W, SnakeMove>("SnakeInput")
		.kind<pulse::LogicPipeline>()
		.each(handleSnakeInputSystemWrapper);

	world.system<const Facing4W, SnakeMove>("ScheduleSnakeMove")
		.kind<pulse::LogicPipeline>()
		.each(scheduleSnakeMoveSystemWrapper);

	world.system<SnakeBodies>("SyncSnakeBodyPosition")
		.kind<pulse::LogicPipeline>()
		.each(syncSnakeBodyPositionSystem);
}

static void registerSnakeEvents(flecs::world& world, pulse::EventCenter& eventManager)
{
	auto snakeMoveIntentDispatcher = std::make_unique<pulse::EntityEventRegister<SnakeMoveIntentEvent, SnakeBodies>>();
	snakeMoveIntentDispatcher->reg(executeSnakeMoveSystemWrapper, world.query<const IsApple, const Position>());
	snakeMoveIntentDispatcher->observe(world);
	eventManager.register_event(std::move(snakeMoveIntentDispatcher));

	auto appleEatenDispatcher = std::make_unique<pulse::EventRegister<AppleEatenEvent>>();
	appleEatenDispatcher->reg(eatAppleSystemWrapper);
	appleEatenDispatcher->reg(increaseScoreSystemWrapper, world.query<Score>());
	appleEatenDispatcher->reg(spawnAppleSystemWrapper, world.query<const SnakeBodies>());
	appleEatenDispatcher->observe(world);
	eventManager.register_event(std::move(appleEatenDispatcher));

	auto gameOverDispatcher = std::make_unique<pulse::EventRegister<GameOverEvent>>();
	gameOverDispatcher->reg(onGameOverSystemWrapper, world.query<SnakeBodies>(), world.query<IsApple>());
	gameOverDispatcher->observe(world);
	eventManager.register_event(std::move(gameOverDispatcher));
}

void importModule(flecs::world& world, pulse::EventCenter& eventManager)
{
	registerSnakeSystems(world);
	registerSnakeEvents(world, eventManager);
}
