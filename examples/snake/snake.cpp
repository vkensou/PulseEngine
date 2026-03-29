#include "snake.h"

#include "handmademath.h"
#include <optional>
#include <random>
#include <span>

HMM_Vec3 toDelta(Direction4W direction)
{
	switch (direction)
	{
	case Direction4W::Right:
		return HMM_V3(1, 0, 0);
	case Direction4W::Up:
		return HMM_V3(0, 1, 0);
	case Direction4W::Left:
		return HMM_V3(-1, 0, 0);
	case Direction4W::Down:
		return HMM_V3(0, -1, 0);
	default:
		return HMM_V3(0, 0, 0);
	}
}

bool isParallel(Direction4W left, Direction4W right)
{
	return (((int)left % 2) == ((int)right % 2));
}

bool isOpposite(Direction4W left, Direction4W right)
{
	return (left != right) && isParallel(left, right);
}

flecs::entity createRenderable(pulse::command_buffer& command_buffer, HMM_Vec3 position, int mat, int mesh)
{
	auto ent = command_buffer.entity();
	ent.add<LocalTransform>()
		.add<WorldTransform>()
		.add<Position>()
		.add<Rendable>()
		.add<ShowMatrix>();

	ent.set<Position>({ .value = position })
		.set<LocalTransform>({ .model = HMM_M4_Identity })
		.set<WorldTransform>({ .value = HMM_M4_Identity })
		.set<Rendable>({ .material = mat, .mesh = mesh })
		.set<ShowMatrix>({ .model = HMM_M4_Identity });

	return ent;
}

std::optional<HMM_Vec3> getNewApplePosition(const SnakeBodies& snake, const Border& border)
{
	int left = border.left;
	int right = border.right;
	int bottom = border.bottom;
	int up = border.up;

	int maxCount = (right - left - 1) * (up - bottom - 1);

	if (snake.bodies.size() >= maxCount)
	{
		return {};
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis;

	int random_number = dis(gen);

	std::vector<HMM_Vec3> used;
	used.reserve(snake.bodies.size());
	for (size_t i = 0; i < snake.bodies.size(); ++i)
	{
		auto& body = snake.bodies[i];
		used.push_back(body.position);
	}

	while (true)
	{
		int x = dis(gen, std::uniform_int_distribution<>::param_type(left + 1, right - 1));
		int y = dis(gen, std::uniform_int_distribution<>::param_type(bottom + 1, up - 1));
		auto newPos = HMM_V3(x, y, 0);

		if (!(left < x && x < right && bottom < y && y < up))
			continue;

		bool bodyOverlaped = false;
		for (int i = 0; i < used.size(); ++i)
		{
			if (used[i] == newPos)
			{
				bodyOverlaped = true;
				continue;
			}
		}

		if (!bodyOverlaped)
			return newPos;
	}

	return {};
}

flecs::entity createApple(pulse::command_buffer& command_buffer, const SnakeBodies& snake, const Border& border, int quad, int appleMat)
{
	auto newPos = getNewApplePosition(snake, border);
	if (!newPos.has_value())
		return {};

	auto apple = createRenderable(command_buffer, newPos.value(), appleMat, quad);
	apple.add<IsApple>();
	apple.add<pulse::EventTag>();
	return apple;
}

flecs::entity createSnake(pulse::command_buffer& command_buffer, int quad, int headMat, int bodyMat, HMM_Vec3 initPos)
{
	int snakeInitLength = 3;

	auto head = createRenderable(command_buffer, initPos, headMat, quad);

	std::vector<SnakeBody> bodies;
	bodies.push_back({ .position = initPos, .entity = head });
	for (int i = 0; i < 3; ++i)
	{
		auto bodyPos = HMM_V3(initPos.X - 1 - i, initPos.Y, initPos.Z);
		auto newBody = createRenderable(command_buffer, bodyPos, bodyMat, quad);
		bodies.insert(bodies.begin(), { .position = bodyPos, .entity = newBody });
	}

	auto snake = command_buffer.entity();
	snake
		.set<SnakeBodies>({ .bodies = std::move(bodies) })
		.set<Facing4W>({ .value = Direction4W::Right } )
		.set<SnakeInput>({ .rightKey = SDL_SCANCODE_RIGHT, .upKey = SDL_SCANCODE_UP, .leftKey = SDL_SCANCODE_LEFT, .downKey = SDL_SCANCODE_DOWN })
		.set<SnakeMove>({ .interval = 1, .lastTime = 0 });
	return snake;
}

void createBoard(pulse::command_buffer& command_buffer, HMM_Mat4 matrix, int mat, int mesh)
{
	auto ent = command_buffer.entity();
	ent.add<WorldTransform>()
		.add<Rendable>()
		.add<ShowMatrix>();

	ent.set<WorldTransform>({ .value = matrix })
		.set<Rendable>({ .material = mat, .mesh = mesh })
		.set<ShowMatrix>({ .model = HMM_M4_Identity });
}

Border createBorder(pulse::command_buffer& command_buffer, int quad, int borderMat, int up, int bottom, int left, int right)
{
	auto centerX = (left + right) / 2.0f;
	auto centerY = (up + bottom) / 2.0f;
	auto width = right - left + 1;
	auto height = up - bottom + 1;
	createBoard(command_buffer, HMM_TRS(HMM_V3(right, centerY, 0), HMM_Q_Identity, HMM_V3(1, height, 1)), borderMat, quad);
	createBoard(command_buffer, HMM_TRS(HMM_V3(left, centerY, 0), HMM_Q_Identity, HMM_V3(1, height, 1)), borderMat, quad);
	createBoard(command_buffer, HMM_TRS(HMM_V3(centerX, up, 0), HMM_Q_Identity, HMM_V3(width, 1, 1)), borderMat, quad);
	createBoard(command_buffer, HMM_TRS(HMM_V3(centerX, bottom, 0), HMM_Q_Identity, HMM_V3(width, 1, 1)), borderMat, quad);

	Border border = { .up = up, .bottom = bottom, .left = left,  .right = right };
	command_buffer.set_singleton<Border>(border);
	return border;
}

void createEntities(pulse::command_buffer& command_buffer, const Border& border, const SnakeResources& resources)
{
	command_buffer.set_singleton<Score>({ .value = 0 });
	auto snake = createSnake(command_buffer, resources.quad, resources.snakeHeadMat, resources.snakeBodyMat, HMM_V3(0, 0, 0));
	auto apple = createApple(command_buffer, snake.get<SnakeBodies>(), border, resources.quad, resources.appleMat);
}

void destructEntities(flecs::query<SnakeBodies>& snakeQuery, flecs::query<IsApple>& appleQuery)
{
	snakeQuery.each([](flecs::entity entity, SnakeBodies& snake)
		{
			for (auto& body : snake.bodies)
			{
				body.entity.destruct();
			}
			entity.destruct();
		});

	appleQuery.each([](flecs::entity entity, IsApple apple) 
		{
			entity.destruct();
		});
}

std::optional<Direction4W> getInputDirection(const SnakeInput& input, pulse::res<const KeyboardState>& keyboardState)
{
	bool lastRight = keyboardState.get().lastKeys[input.rightKey];
	bool lastUp = keyboardState.get().lastKeys[input.upKey];
	bool lastLeft = keyboardState.get().lastKeys[input.leftKey];
	bool lastDown = keyboardState.get().lastKeys[input.downKey];

	bool right = keyboardState.get().currentKeys[input.rightKey];
	bool up = keyboardState.get().currentKeys[input.upKey];
	bool left = keyboardState.get().currentKeys[input.leftKey];
	bool down = keyboardState.get().currentKeys[input.downKey];

	if (!lastRight && right)
		return Direction4W::Right;
	else if (!lastUp && up)
		return Direction4W::Up;
	else if (!lastLeft && left)
		return Direction4W::Left;
	else if (!lastDown && down)
		return Direction4W::Down;
	else
		return {};
}

enum class ObstacleType
{
	None,
	Apple,
	SnakeBody,
	Border
};

struct Obstacle
{
private:
	const ObstacleType type;
	const int appleId;

	Obstacle(ObstacleType type, int appleId = -1)
		: type(type), appleId(appleId)
	{
	}

public:
	ObstacleType Type() const { return type; }
	int AppleId() const { return appleId; }

	static Obstacle None()
	{
		return Obstacle(ObstacleType::None);
	}

	static Obstacle Apple(int appleId)
	{
		return Obstacle(ObstacleType::Apple, appleId);
	}

	static Obstacle SnakeBody()
	{
		return Obstacle(ObstacleType::SnakeBody);
	}

	static Obstacle Border()
	{
		return Obstacle(ObstacleType::Border);
	}
};

Obstacle queryCollideObstacle(HMM_Vec3 nextPos, const SnakeBodies& snake, const Border& border, std::optional<HMM_Vec3> applePos)
{
	int left = border.left;
	int right = border.right;
	int bottom = border.bottom;
	int up = border.up;

	if (nextPos.X <= left || nextPos.X >= right || nextPos.Y <= bottom || nextPos.Y >= up)
		return Obstacle::Border();

	for (int i = 1; i < snake.bodies.size() - 1; ++i)
	{
		auto body = snake.bodies[i];
		if (body.position == nextPos)
			return Obstacle::SnakeBody();
	}

	if (applePos.has_value())
	{
		if (nextPos == applePos.value())
			return Obstacle::Apple(0);
	}

	return Obstacle::None();
}

void initSnakeGameSystem(pulse::command_buffer& command_buffer, pulse::singleton_query<const SnakeResources> resources)
{
	int up = 16;
	int bottom = -15;
	int left = -20;
	int right = 21;

	Border border = createBorder(command_buffer, resources.get().quad, resources.get().boardMat, up, bottom, left, right);
	createEntities(command_buffer, border, resources.get());
}

void handleSnakeInputSystem(pulse::res<const KeyboardState> keyboardState, const SnakeInput& input, Facing4W& direction, SnakeMove& move)
{
	auto keyInput = getInputDirection(input, keyboardState);
	if (keyInput.has_value())
	{
		if (!isOpposite(direction.value, keyInput.value()))
		{
			direction.value = keyInput.value();
			move.lastTime = move.interval;
		}
	}
}

void scheduleSnakeMoveSystem(pulse::res<const oval_update_context> context, pulse::event_writer<SnakeMoveIntentEvent> snakeMoveWriter, flecs::entity entity, const Facing4W& direction, SnakeMove& move)
{	
	move.lastTime += context.get().delta_time;
	if (move.lastTime < move.interval)
		return;

	while (move.lastTime > move.interval)
		move.lastTime -= move.interval;

	auto delta = toDelta(direction.value);
	snakeMoveWriter.send<SnakeBodies>(entity, { .delta = delta });
}

void executeSnakeMoveSystem(pulse::event_reader<SnakeMoveIntentEvent> snakeMoveReader, pulse::command_buffer& command_buffer, flecs::query<const IsApple, const Position>& appleQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources, pulse::event_writer<AppleEatenEvent> appleEatWriter, pulse::event_writer<GameOverEvent> gameOverWriter, SnakeBodies& snake)
{
	auto& head = snake.bodies.back();

	auto delta = snakeMoveReader.read().delta;
	auto headPosition = head.position;
	auto nextPos = headPosition + delta;

	auto appleEnt = appleQuery.first();
	std::optional<HMM_Vec3> applePos;
	if (appleEnt.is_alive())
		applePos = appleEnt.get<Position>().value;

	auto& border = borderQuery.get();

	auto obstacle = queryCollideObstacle(nextPos, snake, border, applePos);

	if (obstacle.Type() == ObstacleType::None)
	{
		auto& bodies = snake.bodies;
		for (int i = 0; i < bodies.size() - 1; ++i)
		{
			auto& body = bodies[i];
			const auto& nextBodyPos = bodies[i + 1].position;
			body.position = nextBodyPos;
		}
		head.position = nextPos;
	}
	else if (obstacle.Type() == ObstacleType::Apple)
	{
		head.position = nextPos;
		auto newBody = createRenderable(command_buffer, headPosition, resources.get().snakeBodyMat, resources.get().quad);
		snake.bodies.insert(snake.bodies.end() - 1, { .position = headPosition , .entity = newBody });

		AppleEatenEvent appleEat = { .apple = appleEnt };
		appleEatWriter.broadcast(appleEat);
	}
	else
	{
		gameOverWriter.broadcast();
	}
}

void syncSnakeBodyPositionSystem(SnakeBodies& snake)
{
	for (int i = 0; i < snake.bodies.size(); ++i)
	{
		auto& body = snake.bodies[i];
		body.entity.get_mut<Position>().value = body.position;
	}

}

void eatAppleSystem(pulse::event_reader<AppleEatenEvent> eventAppleEat, pulse::command_buffer& command_buffer)
{
	command_buffer.destruct(eventAppleEat.read().apple);
}

void increaseScoreSystem(pulse::event_reader<AppleEatenEvent> eventAppleEat, Score& score)
{
	score.value += 1;
}

void spawnAppleSystem(pulse::event_reader<AppleEatenEvent> eventAppleEat, pulse::command_buffer& command_buffer, flecs::query<const SnakeBodies>& snakeQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources)
{
	auto snakeEnt = snakeQuery.first();
	if (snakeEnt.is_alive())
	{
		createApple(command_buffer, snakeEnt.get<SnakeBodies>(), borderQuery.get(), resources.get().quad, resources.get().appleMat);
	}
}

void onGameOverSystem(pulse::event_reader<GameOverEvent> eventAppleEat, pulse::command_buffer& command_buffer, flecs::query<SnakeBodies>& snakeQuery, flecs::query<IsApple>& appleQuery)
{
	command_buffer.add_singleton(flecs::Disabled);
	destructEntities(snakeQuery, appleQuery);
}

void restartSystem(pulse::command_buffer& command_buffer, pulse::singleton_query<const Border> borderQuery, pulse::singleton_query<const SnakeResources> resources)
{
	createEntities(command_buffer, borderQuery.get(), resources.get());
}
