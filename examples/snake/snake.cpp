#include "snake.h"

#include <optional>
#include <random>
#include "imgui.h"

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

std::optional<Direction4W> getInputDirection(const SnakeInput& input, const PulseKeyboardInput& keyboard)
{
	// 边沿检测：just_pressed 由 pulse-input 事件链路维护（PostFrame 清除）
	if (keyboard.just_pressed[input.rightKey])
		return Direction4W::Right;
	if (keyboard.just_pressed[input.upKey])
		return Direction4W::Up;
	if (keyboard.just_pressed[input.leftKey])
		return Direction4W::Left;
	if (keyboard.just_pressed[input.downKey])
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

// ============================================================
// 资源加载（模块内异步加载状态机，配合游戏状态 UnInitialized/Loading）
// ============================================================

// UnInitialized：发起异步加载请求 → Loading
// Loading：      每帧轮询；失败 → LoadFailed；就绪 → 建材质/棋盘/蛇/苹果 → Gaming
void loadSnakeResourcesSystem(PulseAppId app, pulse::res<SnakeAssets> assets, pulse::system_state_machine<SnakeGameState> state, pulse::command_buffer& command_buffer, flecs::query<PulseWindow, PulsePrimaryWindow>& primaryWindowQuery)
{
	auto& as = assets.get();

	if (state.is(SnakeGameState::UnInitialized))
	{
		// pulse_create_shader_from_file 内部会深拷贝 settings（含属性数组、
		// 名字符串与 blend attachments），栈上临时 desc 可安全丢弃
		CGPUBlendAttachmentState blend_attachments = {
			.enable = false,
			.src_factor = CGPU_BLEND_FACTOR_ONE,
			.dst_factor = CGPU_BLEND_FACTOR_ZERO,
			.src_alpha_factor = CGPU_BLEND_FACTOR_ONE,
			.dst_alpha_factor = CGPU_BLEND_FACTOR_ZERO,
			.blend_op = CGPU_BLEND_OP_ADD,
			.blend_alpha_op = CGPU_BLEND_OP_ADD,
			.color_mask = CGPU_COLOR_MASK_RGBA,
		};
		PulseShaderProperty shader_props[] = {
			{ .name = "vpMatrix", .type = PULSE_SHADER_PROPERTY_TYPE_MAT4,   .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL, .set = 0, .binding = 0, .offset = 0, .size = 64 },
			{ .name = "albedo",   .type = PULSE_SHADER_PROPERTY_TYPE_FLOAT4, .role = PULSE_SHADER_PROPERTY_ROLE_MATERIAL,     .set = 1, .binding = 0, .offset = 0, .size = 16 },
			{ .name = "wMatrix",  .type = PULSE_SHADER_PROPERTY_TYPE_MAT4,   .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL, .set = 2, .binding = 0, .offset = 0, .size = 64 },
		};
		PulseShaderCreateFromFileDesc shader_desc = {
			.vert_path = "color.vert.spv",
			.frag_path = "color.frag.spv",
			.blend_desc = {
				.attachment_count = 1,
				.p_attachments = &blend_attachments,
				.alpha_to_coverage = false,
				.independent_blend = false,
			},
			.depth_desc = {
				.depth_test = true,
				.depth_write = true,
				.depth_op = CGPU_COMPARE_OP_GREATER_EQUAL,
				.stencil_test = false,
			},
			.rasterizer_state = {
				.cull_mode = CGPU_CULL_MODE_BACK,
				.front_face = CGPU_FRONT_FACE_CLOCK_WISE,
			},
			.property_count = 3,
			.p_properties = shader_props,
		};

		// ---- shader / mesh（异步，不等待）----
		as.shader = pulse_create_shader_from_file(app, &shader_desc);
		as.mesh = pulse_load_mesh(app, "Quad.obj");
		state.to(SnakeGameState::Loading);
		return;
	}

	// ---- 失败检测（避免静默无限轮询）----
	PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
	PulseAssetRequest shaderRequest = pulse_shader_request_to_asset_request(as.shader);
	PulseAssetRequest meshRequest = pulse_mesh_request_to_asset_request(as.mesh);
	EPulseAssetState shaderState = pulse_asset_system_get_state(assetSystem, shaderRequest);
	EPulseAssetState meshState = pulse_asset_system_get_state(assetSystem, meshRequest);
	if (shaderState == PULSE_ASSET_STATE_FAILED || meshState == PULSE_ASSET_STATE_FAILED)
	{
		if (shaderState == PULSE_ASSET_STATE_FAILED)
			printf("Snake shader load failed: %s\n", pulse_asset_system_get_error(assetSystem, shaderRequest));
		if (meshState == PULSE_ASSET_STATE_FAILED)
			printf("Snake mesh load failed: %s\n", pulse_asset_system_get_error(assetSystem, meshRequest));
		state.to(SnakeGameState::LoadFailed);
		return;
	}

	if (!pulse_shader_is_ready(app, as.shader) || !pulse_mesh_is_ready(app, as.mesh))
		return;

	// ---- 就绪：解析资源 handle ----
	PulseShaderHandle shader = pulse_shader_get_handle(app, as.shader);
	PulseMeshHandle quad = pulse_mesh_get_handle(app, as.mesh);

	// 创建 4 个材质（白边框 / 红苹果 / 黄蛇头 / 绿蛇身）
	PulseMaterialHandle boardMat, appleMat, snakeHeadMat, snakeBodyMat;
	{
		PulseMaterialCreateDesc mat_desc = { .shader = shader };
		boardMat = pulse_create_material(app, &mat_desc);
		pulse_material_set_property_float4(app, boardMat, "albedo", HMM_V4(1, 1, 1, 1));
	}
	{
		PulseMaterialCreateDesc mat_desc = { .shader = shader };
		appleMat = pulse_create_material(app, &mat_desc);
		pulse_material_set_property_float4(app, appleMat, "albedo", HMM_V4(1, 0, 0, 0));
	}
	{
		PulseMaterialCreateDesc mat_desc = { .shader = shader };
		snakeHeadMat = pulse_create_material(app, &mat_desc);
		pulse_material_set_property_float4(app, snakeHeadMat, "albedo", HMM_V4(1, 1, 0, 1));
	}
	{
		PulseMaterialCreateDesc mat_desc = { .shader = shader };
		snakeBodyMat = pulse_create_material(app, &mat_desc);
		pulse_material_set_property_float4(app, snakeBodyMat, "albedo", HMM_V4(0, 1, 0, 1));
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

	auto windowEntity = primaryWindowQuery.first();
	auto camera = command_buffer.entity();
	camera.set<PulseLocalTransform>({ .translation = HMM_V3(0.5f, 0.5f, -38.f), .rotation = HMM_Q_Identity, .scale = HMM_V3_One });
	camera.set<PulseCamera>({ .window_entity = windowEntity, .fov = 45.f, .near_plane = 0.1f, .far_plane = 1000.f });

	state.to(SnakeGameState::Gaming);
	printf("Snake resources ready.\n");
}

void handleSnakeInputSystem(pulse::res<const PulseKeyboardInput> keyboard, const SnakeInput& input, Facing4W& direction, SnakeMove& move)
{
	auto keyInput = getInputDirection(input, keyboard.get());
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

void onGameOverSystem(pulse::event_reader<GameOverEvent> eventAppleEat, pulse::command_buffer& command_buffer, pulse::system_state_machine<SnakeGameState> state, flecs::query<SnakeBodies>& snakeQuery, flecs::query<IsApple>& appleQuery)
{
	state.to(SnakeGameState::GameOver);
	destructEntities(snakeQuery, appleQuery);
}

void snakeUISystem(const Score& score, flecs::query<PulseWindow, PulsePrimaryWindow>& primaryWindowQuery, pulse::system_state_machine<SnakeGameState> state, pulse::res<const PulseKeyboardInput> keyboard, pulse::event_writer<RestartEvent> restartEvent)
{
	bool playing = state.is(SnakeGameState::Gaming);
	if (playing)
	{
		ImGui::Text("%d", score.value);
	}
	else
	{
		if (ImGui::Button("Restart"))
		{
			restartEvent.broadcast();
		}
	}
}

void restartSystem(pulse::event_reader<RestartEvent> restartEvent, pulse::command_buffer& command_buffer, pulse::system_state_machine<SnakeGameState> state, pulse::singleton_query<const Border> borderQuery, pulse::singleton_query<const SnakeResources> resources)
{
	command_buffer.defer_suspend();
	createEntities(command_buffer, borderQuery.get(), resources.get());
	command_buffer.defer_resume();
	state.to(SnakeGameState::Gaming);
}
