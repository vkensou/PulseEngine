#include "snake.h"

#include "handmademath.h"
#include <optional>
#include <random>
#include <span>

HMM_Vec3 toDelta(Direction direction)
{
	switch (direction)
	{
	case Direction::Right:
		return HMM_V3(1, 0, 0);
	case Direction::Up:
		return HMM_V3(0, 1, 0);
	case Direction::Left:
		return HMM_V3(-1, 0, 0);
	case Direction::Down:
		return HMM_V3(0, -1, 0);
	default:
		return HMM_V3(0, 0, 0);
	}
}

bool isParallel(Direction left, Direction right)
{
	return (((int)left % 2) == ((int)right % 2));
}

bool isOpposite(Direction left, Direction right)
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

std::optional<HMM_Vec3> getNewApplePosition(const Snake& snake, const Border& border)
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
		auto body = snake.bodies[i];
		auto& position = body.get<Position>();
		used.push_back(position.value);
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

flecs::entity createApple(pulse::command_buffer& command_buffer, const Snake& snake, const Border& border, int quad, int appleMat)
{
	auto newPos = getNewApplePosition(snake, border);
	if (!newPos.has_value())
		return {};

	auto apple = createRenderable(command_buffer, newPos.value(), appleMat, quad);
	apple.add<IsApple>();
	apple.add<EventTag>();
	return apple;
}

flecs::entity createSnake(pulse::command_buffer& command_buffer, int quad, int headMat, int bodyMat, HMM_Vec3 initPos)
{
	int snakeInitLength = 3;

	auto head = createRenderable(command_buffer, initPos, headMat, quad);
	head.add<Direction>()
		.add<SnakeInput>()
		.add<SnakeMove>();

	head.set<Direction>(Direction::Right)
		.set<SnakeInput>({ .rightKey = SDL_SCANCODE_RIGHT, .upKey = SDL_SCANCODE_UP, .leftKey = SDL_SCANCODE_LEFT, .downKey = SDL_SCANCODE_DOWN })
		.set<SnakeMove>({ .interval = 1, .lastTime = 0 });

	std::vector<flecs::entity> bodies;
	bodies.push_back(head);
	for (int i = 0; i < 3; ++i)
	{
		auto newBody = createRenderable(command_buffer, HMM_V3(initPos.X - 1 - i, initPos.Y, initPos.Z), bodyMat, quad);
		bodies.insert(bodies.begin(), newBody);
	}

	auto snake = command_buffer.entity();
	snake.add<Snake>();
	snake.set<Snake>({ .head = head, .bodies = std::move(bodies) });
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
	auto apple = createApple(command_buffer, snake.get<Snake>(), border, resources.quad, resources.appleMat);
}

void destructEntities(flecs::query<Snake>& snakeQuery, flecs::query<IsApple>& appleQuery)
{
	snakeQuery.each([](flecs::entity entity, Snake& snake)
		{
			for (auto& body : snake.bodies)
			{
				body.destruct();
			}
			entity.destruct();
		});

	appleQuery.each([](flecs::entity entity, IsApple apple) 
		{
			entity.destruct();
		});
}

std::optional<Direction> getInputDirection(const SnakeInput& input, pulse::res<const KeyboardState>& keyboardState)
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
		return Direction::Right;
	else if (!lastUp && up)
		return Direction::Up;
	else if (!lastLeft && left)
		return Direction::Left;
	else if (!lastDown && down)
		return Direction::Down;
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

Obstacle queryCollideObstacle(HMM_Vec3 nextPos, const Snake& snake, const Border& border, std::optional<HMM_Vec3> applePos)
{
	int left = border.left;
	int right = border.right;
	int bottom = border.bottom;
	int up = border.up;

	if (nextPos.X <= left || nextPos.X >= right || nextPos.Y <= bottom || nextPos.Y >= up)
		return Obstacle::Border();

	for (int i = 1; i < snake.bodies.size() - 1; ++i)
	{
		auto bodyEnt = snake.bodies[i];
		auto& bodyPos = bodyEnt.get<Position>();
		if (bodyPos.value == nextPos)
			return Obstacle::SnakeBody();
	}

	if (applePos.has_value())
	{
		if (nextPos == applePos.value())
			return Obstacle::Apple(0);
	}

	return Obstacle::None();
}

void initSnakeGame(pulse::command_buffer& command_buffer, pulse::singleton_query<const SnakeResources> resources)
{
	int up = 16;
	int bottom = -15;
	int left = -20;
	int right = 21;

	Border border = createBorder(command_buffer, resources.get().quad, resources.get().boardMat, up, bottom, left, right);
	createEntities(command_buffer, border, resources.get());
}

void snakeInput(pulse::res<const KeyboardState> keyboardState, const SnakeInput& input, Direction& direction, SnakeMove& move)
{
	auto keyInput = getInputDirection(input, keyboardState);
	if (keyInput.has_value())
	{
		if (!isOpposite(direction, keyInput.value()))
		{
			direction = keyInput.value();
			move.lastTime = move.interval;
		}
	}
}

void snakeMove(pulse::command_buffer& command_buffer, pulse::res<const SystemContext> context, flecs::query<const IsApple, const Position>& appleQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources, pulse::event_writer<AppleEat> appleEatWriter, pulse::event_writer<GameOver> gameOverWriter, Snake& snake)
{
	auto head = snake.head;
	auto& headMove = head.get_mut<SnakeMove>();

	headMove.lastTime += context.get().delta_time;
	if (headMove.lastTime < headMove.interval)
		return;

	auto& headPosition = head.get_mut<Position>();
	auto& headDirection = head.get<Direction>();
	auto nextPos = headPosition.value + toDelta(headDirection);

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
			auto& bodyPos = bodies[i].get_mut<Position>();
			const auto& nextBodyPos = bodies[i + 1].get<Position>();
			bodyPos.value = nextBodyPos.value;
		}
		headPosition.value = nextPos;
	}
	else if (obstacle.Type() == ObstacleType::Apple)
	{
		auto newBody = createRenderable(command_buffer, headPosition.value, resources.get().snakeBodyMat, resources.get().quad);
		snake.bodies.insert(snake.bodies.end() - 1, newBody);
		headPosition.value = nextPos;

		AppleEat appleEat = { .apple = appleEnt };
		appleEatWriter.broadcast(appleEat);
	}
	else
	{
		gameOverWriter.broadcast();
	}

	while (headMove.lastTime > headMove.interval)
		headMove.lastTime -= headMove.interval;
}

void eatApple(pulse::event_reader<AppleEat> eventAppleEat, pulse::command_buffer& command_buffer)
{
	command_buffer.destruct(eventAppleEat.read().apple);
}

void addScore(pulse::event_reader<AppleEat> eventAppleEat, Score& score)
{
	score.value += 1;
}

void createAppleSystem(pulse::event_reader<AppleEat> eventAppleEat, pulse::command_buffer& command_buffer, flecs::query<const Snake>& snakeQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources)
{
	auto snakeEnt = snakeQuery.first();
	if (snakeEnt.is_alive())
	{
		createApple(command_buffer, snakeEnt.get<Snake>(), borderQuery.get(), resources.get().quad, resources.get().appleMat);
	}
}

void gameover(pulse::event_reader<GameOver> eventAppleEat, pulse::command_buffer& command_buffer, flecs::query<Snake>& snakeQuery, flecs::query<IsApple>& appleQuery)
{
	command_buffer.add_singleton(flecs::Disabled);
	destructEntities(snakeQuery, appleQuery);
}

void restart(pulse::command_buffer& command_buffer, pulse::singleton_query<const Border> borderQuery, pulse::singleton_query<const SnakeResources> resources)
{
	createEntities(command_buffer, borderQuery.get(), resources.get());
}
