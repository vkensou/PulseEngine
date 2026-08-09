#include "snake.h"

#include "pulse_math.h"
#include "pulse_input.h"
#include <algorithm>
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

flecs::entity createRenderable(pulse::command_buffer& command_buffer, HMM_Vec3 position, PulseMaterialHandle mat, PulseMeshHandle mesh)
{
	auto ent = command_buffer.entity();
	ent.add<PulseLocalTransform>()
		.add<PulseRenderable>();

	ent.set<PulseLocalTransform>({ .translation = position, .rotation = HMM_Q_Identity, .scale = HMM_V3_One })
		.set<PulseRenderable>({ .mesh = mesh, .material = mat });

	return ent;
}

std::optional<HMM_Vec3> getNewApplePosition(const std::vector<SnakeBody>& snake, const Border& border)
{
	int left = border.left;
	int right = border.right;
	int bottom = border.bottom;
	int up = border.up;

	int maxCount = (right - left - 1) * (up - bottom - 1);

	if (snake.size() >= (size_t)maxCount)
	{
		return {};
	}

	std::random_device rd;
	std::mt19937 gen(rd());
	std::uniform_int_distribution<> dis;

	int random_number = dis(gen);

	std::vector<HMM_Vec3> used;
	used.reserve(snake.size());
	for (size_t i = 0; i < snake.size(); ++i)
	{
		auto& body = snake[i];
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

flecs::entity createApple(pulse::command_buffer& command_buffer, const std::vector<SnakeBody>& snake, const Border& border, PulseMeshHandle quad, PulseMaterialHandle appleMat)
{
	auto newPos = getNewApplePosition(snake, border);
	if (!newPos.has_value())
		return {};

	auto apple = createRenderable(command_buffer, newPos.value(), appleMat, quad);
	apple.add<IsApple>();
	return apple;
}

// 返回蛇实体与身体列表：实体创建是 deferred 的（系统内），不能事后
// 再 get 身体数据，所以直接随返回值带出。
std::pair<flecs::entity, std::vector<SnakeBody>> createSnake(pulse::command_buffer& command_buffer, PulseMeshHandle quad, PulseMaterialHandle headMat, PulseMaterialHandle bodyMat, HMM_Vec3 initPos)
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
		.set<SnakeBodies>({ .bodies = bodies })
		.set<Facing4W>({ .value = Direction4W::Right } )
		.set<SnakeInput>({ .rightKey = SDL_SCANCODE_RIGHT, .upKey = SDL_SCANCODE_UP, .leftKey = SDL_SCANCODE_LEFT, .downKey = SDL_SCANCODE_DOWN })
		.set<SnakeMove>({ .interval = 1, .lastTime = 0 });
	return { snake, std::move(bodies) };
}

// 棋盘：静态渲染实体，直接挂 PulseLocalTransform（带缩放）+ PulseRenderable。
void createBoard(pulse::command_buffer& command_buffer, HMM_Vec3 position, HMM_Vec3 scale, PulseMaterialHandle mat, PulseMeshHandle mesh)
{
	auto ent = command_buffer.entity();
	ent.add<PulseLocalTransform>()
		.add<PulseRenderable>();
	ent.set<PulseLocalTransform>({ .translation = position, .rotation = HMM_Q_Identity, .scale = scale })
		.set<PulseRenderable>({ .mesh = mesh, .material = mat });
}

Border createBorder(pulse::command_buffer& command_buffer, PulseMeshHandle quad, PulseMaterialHandle borderMat, int up, int bottom, int left, int right)
{
	auto centerX = (left + right) / 2.0f;
	auto centerY = (up + bottom) / 2.0f;
	auto width = right - left + 1;
	auto height = up - bottom + 1;
	createBoard(command_buffer, HMM_V3(right, centerY, 0), HMM_V3(1, height, 1), borderMat, quad);
	createBoard(command_buffer, HMM_V3(left, centerY, 0), HMM_V3(1, height, 1), borderMat, quad);
	createBoard(command_buffer, HMM_V3(centerX, up, 0), HMM_V3(width, 1, 1), borderMat, quad);
	createBoard(command_buffer, HMM_V3(centerX, bottom, 0), HMM_V3(width, 1, 1), borderMat, quad);

	Border border = { .up = up, .bottom = bottom, .left = left,  .right = right };
	command_buffer.set_singleton<Border>(border);
	return border;
}

void createEntities(pulse::command_buffer& command_buffer, const Border& border, const SnakeResources& resources)
{
	command_buffer.set_singleton<SnakeGame>({ .playing = true });
	command_buffer.set_singleton<Score>({ .value = 0 });
	auto [snake, bodies] = createSnake(command_buffer, resources.quad, resources.snakeHeadMat, resources.snakeBodyMat, HMM_V3(0, 0, 0));
	(void)snake;
	auto apple = createApple(command_buffer, bodies, border, resources.quad, resources.appleMat);
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

std::optional<Direction4W> getInputDirection(const SnakeInput& input, PulseAppId app)
{
	// 边沿检测：just_pressed 由 pulse_input 事件链路维护（PostFrame 清除）
	if (pulse_input_key_just_pressed(app, input.rightKey))
		return Direction4W::Right;
	if (pulse_input_key_just_pressed(app, input.upKey))
		return Direction4W::Up;
	if (pulse_input_key_just_pressed(app, input.leftKey))
		return Direction4W::Left;
	if (pulse_input_key_just_pressed(app, input.downKey))
		return Direction4W::Down;
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

// 资源就绪后的一次性初始化：解析 handle → 创建材质 → 棋盘/蛇/苹果。
// 未就绪时直接返回，由系统每帧重试（异步加载状态机）。
void prepareGameSystem(pulse::res<SnakeAssets> assets, pulse::command_buffer& command_buffer)
{
	auto& as = assets.get();
	if (as.ready)
		return;

	if (!pulse_shader_is_ready(as.app, as.shader) || !pulse_mesh_is_ready(as.app, as.mesh))
		return;

	// 解析资源 handle
	PulseShaderHandle shader = pulse_shader_get_handle(as.app, as.shader);
	PulseMeshHandle quad = pulse_mesh_get_handle(as.app, as.mesh);

	// 创建 4 个材质（白边框 / 红苹果 / 黄蛇头 / 绿蛇身）
	PulseMaterialHandle boardMat, appleMat, snakeHeadMat, snakeBodyMat;
	{
		PulseMaterialCreateDesc mat_desc = { .shader = shader };
		boardMat = pulse_create_material(as.app, &mat_desc);
		pulse_material_set_property_float4(as.app, boardMat, "albedo", HMM_V4(1, 1, 1, 1));
	}
	{
		PulseMaterialCreateDesc mat_desc = { .shader = shader };
		appleMat = pulse_create_material(as.app, &mat_desc);
		pulse_material_set_property_float4(as.app, appleMat, "albedo", HMM_V4(1, 0, 0, 0));
	}
	{
		PulseMaterialCreateDesc mat_desc = { .shader = shader };
		snakeHeadMat = pulse_create_material(as.app, &mat_desc);
		pulse_material_set_property_float4(as.app, snakeHeadMat, "albedo", HMM_V4(1, 1, 0, 1));
	}
	{
		PulseMaterialCreateDesc mat_desc = { .shader = shader };
		snakeBodyMat = pulse_create_material(as.app, &mat_desc);
		pulse_material_set_property_float4(as.app, snakeBodyMat, "albedo", HMM_V4(0, 1, 0, 1));
	}

	SnakeResources snakeResources = {
		.quad = quad,
		.appleMat = appleMat,
		.snakeHeadMat = snakeHeadMat,
		.snakeBodyMat = snakeBodyMat,
		.boardMat = boardMat,
	};
	command_buffer.set_singleton<SnakeResources>(snakeResources);

	// 棋盘 + 蛇 + 苹果
	int up = 16;
	int bottom = -15;
	int left = -20;
	int right = 21;

	// 注意：系统运行期间（stage）不能 defer_suspend 后直接创建实体，
	// 会让结构变更绕过 staging 直接改 readonly 主 world 导致死锁。
	// 这里让实体创建走正常的 deferred 路径（系统结束 merge 时生效）。
	Border border = createBorder(command_buffer, quad, boardMat, up, bottom, left, right);
	createEntities(command_buffer, border, snakeResources);

	as.ready = true;
	printf("Snake resources ready.\n");
}

void handleSnakeInputSystem(pulse::res<const SnakeAssets> assets, const SnakeInput& input, Facing4W& direction, SnakeMove& move)
{
	auto keyInput = getInputDirection(input, assets.get().app);
	if (keyInput.has_value())
	{
		if (!isOpposite(direction.value, keyInput.value()))
		{
			direction.value = keyInput.value();
			move.lastTime = move.interval;
		}
	}
}

void scheduleSnakeMoveSystem(pulse::res<const PulseTimer> timer, pulse::event_writer<SnakeMoveIntentEvent> snakeMoveWriter, flecs::entity entity, const Facing4W& direction, SnakeMove& move)
{	
	move.lastTime += timer.get().delta_time;
	if (move.lastTime < move.interval)
		return;

	while (move.lastTime > move.interval)
		move.lastTime -= move.interval;

	auto delta = toDelta(direction.value);
	snakeMoveWriter.send<SnakeBodies>(entity, { .delta = delta });
}

void executeSnakeMoveSystem(pulse::event_reader<SnakeMoveIntentEvent> snakeMoveReader, pulse::command_buffer& command_buffer, flecs::query<const IsApple, const PulseLocalTransform>& appleQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources, pulse::event_writer<AppleEatenEvent> appleEatWriter, pulse::event_writer<GameOverEvent> gameOverWriter, SnakeBodies& snake)
{
	auto& head = snake.bodies.back();

	auto delta = snakeMoveReader.read().delta;
	auto headPosition = head.position;
	auto nextPos = headPosition + delta;

	auto appleEnt = appleQuery.first();
	std::optional<HMM_Vec3> applePos;
	if (appleEnt.is_alive())
		applePos = appleEnt.get<PulseLocalTransform>().translation;

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
		body.entity.set<PulseLocalTransform>(
			{ .translation = body.position, .rotation = HMM_Q_Identity, .scale = HMM_V3_One });
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
		createApple(command_buffer, snakeEnt.get<SnakeBodies>().bodies, borderQuery.get(), resources.get().quad, resources.get().appleMat);
	}
}

void onGameOverSystem(pulse::event_reader<GameOverEvent> eventAppleEat, pulse::command_buffer& command_buffer, flecs::query<SnakeBodies>& snakeQuery, flecs::query<IsApple>& appleQuery)
{
	command_buffer.set_singleton<SnakeGame>({ .playing = false });
	destructEntities(snakeQuery, appleQuery);
}

void restartSystem(pulse::event_reader<RestartEvent> restartEvent, pulse::command_buffer& command_buffer, pulse::singleton_query<const Border> borderQuery, pulse::singleton_query<const SnakeResources> resources)
{
	command_buffer.defer_suspend();
	createEntities(command_buffer, borderQuery.get(), resources.get());
	command_buffer.defer_resume();
	command_buffer.set_singleton<SnakeGame>({ .playing = true });
}
