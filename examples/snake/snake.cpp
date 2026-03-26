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

flecs::entity createRenderable(flecs::world& world, HMM_Vec3 position, int mat, int mesh)
{
	auto ent = world.entity();
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

std::optional<HMM_Vec3> getNewApplePosition(flecs::world& world, const Snake& snake, const Border& border)
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

flecs::entity createApple(flecs::world& world, const Snake& snake, const Border& border, int quad, int appleMat)
{
	auto newPos = getNewApplePosition(world, snake, border);
	if (!newPos.has_value())
		return {};

	auto apple = createRenderable(world, newPos.value(), appleMat, quad);
	apple.add<IsApple>();
	return apple;
}

flecs::entity createSnake(flecs::world& world, int quad, int headMat, int bodyMat, HMM_Vec3 initPos)
{
	int snakeInitLength = 3;

	auto head = createRenderable(world, initPos, headMat, quad);
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
		auto newBody = createRenderable(world, HMM_V3(initPos.X - 1 - i, initPos.Y, initPos.Z), bodyMat, quad);
		bodies.insert(bodies.begin(), newBody);
	}

	auto snake = world.entity();
	snake.add<Snake>();
	snake.set<Snake>({ .head = head, .bodies = std::move(bodies) });
	return snake;
}

void createBoard(flecs::world& world, HMM_Mat4 matrix, int mat, int mesh)
{
	auto ent = world.entity();
	ent.add<WorldTransform>()
		.add<Rendable>()
		.add<ShowMatrix>();

	ent.set<WorldTransform>({ .value = matrix })
		.set<Rendable>({ .material = mat, .mesh = mesh })
		.set<ShowMatrix>({ .model = HMM_M4_Identity });
}

flecs::entity createBorder(flecs::world& world, int quad, int borderMat, int up, int bottom, int left, int right)
{
	auto centerX = (left + right) / 2.0f;
	auto centerY = (up + bottom) / 2.0f;
	auto width = right - left + 1;
	auto height = up - bottom + 1;
	createBoard(world, HMM_TRS(HMM_V3(right, centerY, 0), HMM_Q_Identity, HMM_V3(1, height, 1)), borderMat, quad);
	createBoard(world, HMM_TRS(HMM_V3(left, centerY, 0), HMM_Q_Identity, HMM_V3(1, height, 1)), borderMat, quad);
	createBoard(world, HMM_TRS(HMM_V3(centerX, up, 0), HMM_Q_Identity, HMM_V3(width, 1, 1)), borderMat, quad);
	createBoard(world, HMM_TRS(HMM_V3(centerX, bottom, 0), HMM_Q_Identity, HMM_V3(width, 1, 1)), borderMat, quad);

	auto border = world.singleton<Border>();
	border.add<Border>()
		.set<Border>({ .up = up, .bottom = bottom, .left = left,  .right = right });
	return border;
}

void createEntities(flecs::world& world, const SnakeResources& resources)
{
	auto border = world.singleton<Border>();
	auto snake = createSnake(world, resources.quad, resources.snakeHeadMat, resources.snakeBodyMat, HMM_V3(0, 0, 0));
	auto apple = createApple(world, snake.get<Snake>(), border.get<Border>(), resources.quad, resources.appleMat);
}

void destructEntities(flecs::world& world)
{
	auto appleQuery = world.query<IsApple>();
	appleQuery.run([](flecs::iter& it)
		{
			while (it.next())
			{
				for (size_t i = 0; i < it.count(); ++i)
				{
					auto e = it.entity(i);
					e.destruct();
				}
			}
		});

	auto snakeQuery = world.query<Snake>();
	snakeQuery.run([](flecs::iter& it)
		{
			while (it.next())
			{
				const auto& snakes = it.field<Snake>(0);
				for (size_t i = 0; i < it.count(); ++i)
				{
					auto e = it.entity(i);
					auto& snake = snakes[i];
					for (auto& body : snake.bodies)
					{
						body.destruct();
					}
					e.destruct();
				}
			}
		});
}

std::optional<Direction> getInputDirection(const SnakeInput& input, std::vector<uint8_t> lastKeyboardStates, std::span<const bool> currentKeyboardStates)
{
	bool lastRight = lastKeyboardStates[input.rightKey];
	bool lastUp = lastKeyboardStates[input.upKey];
	bool lastLeft = lastKeyboardStates[input.leftKey];
	bool lastDown = lastKeyboardStates[input.downKey];

	bool right = currentKeyboardStates[input.rightKey];
	bool up = currentKeyboardStates[input.upKey];
	bool left = currentKeyboardStates[input.leftKey];
	bool down = currentKeyboardStates[input.downKey];

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

void createSnakeGame(flecs::world& world, int up, int bottom, int left, int right)
{
	auto snakeApp = world.singleton<SnakeApp>();
	auto& resources = snakeApp.get<SnakeResources>();

	auto border = createBorder(world, resources.quad, resources.boardMat, up, bottom, left, right);
	createEntities(world, resources);
}

void snakeInput(SystemSnakeInputState& systemSnakeInputState, const SnakeInput& input, Direction& direction, SnakeMove& move)
{
	int numKeys;
	auto currentKeyboardStates = SDL_GetKeyboardState(&numKeys);

	assert(numKeys == systemSnakeInputState.lastKeyboardStates.size());

	auto keyInput = getInputDirection(input, systemSnakeInputState.lastKeyboardStates, std::span<const bool>(currentKeyboardStates, numKeys));
	if (keyInput.has_value())
	{
		if (!isOpposite(direction, keyInput.value()))
		{
			direction = keyInput.value();
			move.lastTime = move.interval;
		}
	}

	memcpy(systemSnakeInputState.lastKeyboardStates.data(), currentKeyboardStates, numKeys);
}

void snakeMove(flecs::world& world, const SystemContext& context, SystemSnakeMoveState& systemSnakeMoveState, const SnakeResources& resources, Snake& snake)
{
	auto head = snake.head;
	auto& headMove = head.get_mut<SnakeMove>();

	headMove.lastTime += context.delta_time;
	if (headMove.lastTime < headMove.interval)
		return;

	auto& headPosition = head.get_mut<Position>();
	auto& headDirection = head.get<Direction>();
	auto nextPos = headPosition.value + toDelta(headDirection);

	auto appleEnt = systemSnakeMoveState.apple.first();
	std::optional<HMM_Vec3> applePos;
	if (appleEnt.is_alive())
		applePos = appleEnt.get<Position>().value;

	auto borderEnt = systemSnakeMoveState.border.first();
	auto& border = borderEnt.get<Border>();

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
		headMove.lastTime -= headMove.interval;
	}
	else if (obstacle.Type() == ObstacleType::Apple)
	{
		auto newBody = createRenderable(world, headPosition.value, resources.snakeBodyMat, resources.quad);
		snake.bodies.insert(snake.bodies.end() - 1, newBody);
		headPosition.value = nextPos;
		headMove.lastTime -= headMove.interval;

		world.event<AppleEat>()
			.id<IsApple>()
			.entity(appleEnt)
			.enqueue();

		world.event<AppleEat>()
			.id<Score>()
			.entity(world.singleton<SnakeApp>())
			.enqueue();

		world.event<AppleEat>()
			.id<Game>()
			.entity(world.singleton<SnakeApp>())
			.enqueue();
	}
	else
	{
		world.event<GameOver>()
			.id<Game>()
			.entity(world.singleton<SnakeApp>())
			.enqueue();
	}
}

void eatApple(flecs::entity apple, const IsApple&)
{
	apple.destruct();
}

void addScore(Score& score)
{
	score.value += 1;
}

void createAppleSystem(flecs::world& world, const SnakeResources& resources)
{
	auto snakeQuery = world.query<Snake>();
	auto borderQuery = world.query<Border>();
	auto snakeEnt = snakeQuery.first();
	auto borderEnt = borderQuery.first();
	if (snakeEnt.is_alive() && borderEnt.is_alive())
	{
		createApple(world, snakeEnt.get<Snake>(), borderEnt.get<Border>(), resources.quad, resources.appleMat);
	}
}

void gameover(flecs::world& world, flecs::entity entity)
{
	entity.disable();
	destructEntities(world);
}

void restart(flecs::world& world, const SnakeResources& resources)
{
	createEntities(world, resources);
}
