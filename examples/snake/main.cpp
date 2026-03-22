#include "framework.h"
#include "imgui.h"
#include <flecs.h>
#include <bit>
#include <filesystem>
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_FS
#include "tiny_gltf.h"
#include <cassert>
#include <SDL3/SDL.h>
#include <random>

constexpr ecs_entity_t kNullEntity = 0;

struct Tree
{
	ecs_entity_t parent{ kNullEntity };
	ecs_entity_t firstChild{ kNullEntity };
	ecs_entity_t lastChild{ kNullEntity };
	ecs_entity_t previousSibling{ kNullEntity };
	ecs_entity_t nextSibling{ kNullEntity };
};

inline Tree& safeGetTree(flecs::world& world, flecs::entity& self)
{
	auto tree = self.try_get_mut<Tree>();
	if (!tree)
	{
		self.add<Tree>();
		tree = self.try_get_mut<Tree>();
	}
	return *tree;
}

void removeFromParent(flecs::world& world, flecs::entity& self, Tree& selfTree);

void insertBefore(flecs::world& world, flecs::entity& self, Tree& selfTree, flecs::entity& newNode, flecs::entity& referenceNode)
{
	assert(newNode != kNullEntity && "newNode is null");

	auto& newTree = newNode.get_mut<Tree>();
	removeFromParent(world, newNode, newTree);

	if (referenceNode != kNullEntity)
	{
		auto& referenceTree = referenceNode.get<Tree>();
		assert(referenceTree.parent == self);

		newTree.previousSibling = referenceTree.previousSibling;
		newTree.nextSibling = referenceNode;

		if (newTree.previousSibling == kNullEntity)
			selfTree.firstChild = newNode;
		else 
		{
			auto& previousTree = flecs::entity(world, newTree.previousSibling).get_mut<Tree>();
			previousTree.nextSibling = newNode;
		}
	}
	else
	{
		if (selfTree.lastChild != kNullEntity)
		{
			auto& lastTree = flecs::entity(world, selfTree.lastChild).get_mut<Tree>();
			assert(lastTree.nextSibling == kNullEntity);
			lastTree.nextSibling = newNode;
			newTree.previousSibling = selfTree.lastChild;
			selfTree.lastChild = newNode;
		}
		else
		{
			assert(selfTree.firstChild == kNullEntity);
			assert(selfTree.lastChild == kNullEntity);
			selfTree.firstChild = newNode;
			selfTree.lastChild = newNode;
		}
	}
	newTree.parent = self;
}

void appendChild(flecs::world& world, flecs::entity& self, Tree& selfTree, flecs::entity& child)
{
	assert(child != kNullEntity && "child is null");
	assert(child != self && "child is self");

	auto& childTree = child.get_mut<Tree>();
	removeFromParent(world, child, childTree);
	childTree.parent = self;
	if (selfTree.lastChild != kNullEntity)
	{
		auto& lastChildTree = flecs::entity(world, selfTree.lastChild).get_mut<Tree>();
		assert(lastChildTree.nextSibling == kNullEntity);
		childTree.previousSibling = selfTree.lastChild;
		lastChildTree.nextSibling = child;
	}
	else
	{
		assert(selfTree.firstChild == kNullEntity);
		selfTree.firstChild = child;
	}

	selfTree.lastChild = child;
}

void replaceChild(flecs::world& world, flecs::entity& self, Tree& selfTree, flecs::entity& newChild, flecs::entity& oldChild)
{
	assert(newChild != kNullEntity && "newChild is null");
	assert(oldChild != kNullEntity && "oldChild is null");
	assert(newChild != self);
	auto& oldChildTree = oldChild.get_mut<Tree>();
	assert(oldChildTree.parent == self);
	auto oldChildNext = flecs::entity(world, oldChildTree.nextSibling);
	removeFromParent(world, oldChild, oldChildTree);
	insertBefore(world, self, selfTree, newChild, oldChildNext);
}

void removeChild(flecs::world& world, flecs::entity& self, Tree& selfTree, flecs::entity& child)
{
	assert(child != kNullEntity && "child is null");
	auto& childTree = child.get_mut<Tree>();
	assert(childTree.parent == self);
	removeFromParent(world, child, childTree);
}

void removeFromParent(flecs::world& world, flecs::entity& self, Tree& selfTree)
{
	if (selfTree.parent == kNullEntity) return;

	auto& parentTree = flecs::entity(world, selfTree.parent).get_mut<Tree>();

	if (parentTree.firstChild == self)
		parentTree.firstChild = selfTree.nextSibling;
	if (parentTree.lastChild == self)
		parentTree.lastChild = selfTree.previousSibling;

	if (selfTree.previousSibling != kNullEntity)
	{
		auto& previousTree = flecs::entity(world, selfTree.previousSibling).get_mut<Tree>();
		previousTree.nextSibling = selfTree.nextSibling;
	}

	if (selfTree.nextSibling != kNullEntity)
	{
		auto& nextTree = flecs::entity(world, selfTree.nextSibling).get_mut<Tree>();
		nextTree.previousSibling = selfTree.previousSibling;
	}

	selfTree.parent = kNullEntity;
	selfTree.previousSibling = kNullEntity;
	selfTree.nextSibling = kNullEntity;
}

void setParent(flecs::world& world, flecs::entity self, flecs::entity newParent)
{
	auto& selfTree = safeGetTree(world, self);
	removeFromParent(world, self, selfTree);
	auto& parentTree = safeGetTree(world, newParent);
	appendChild(world, newParent, parentTree, self);
}

struct BindBuffer
{
	int set;
	int bind;
	HGEGraphics::Buffer* buffer;
};

struct BindTexture
{
	int set;
	int bind;
	HGEGraphics::Texture* texture;
};

struct BindSampler
{
	int set;
	int bind;
	CGPUSamplerId sampler;
};

struct Position
{
	HMM_Vec3 value;
};

struct Rotation
{
	HMM_Quat value;
};

struct SimpleHarmonic
{
	HMM_Vec3 amplitude;
	float speed = 0;
	HMM_Vec3 base;
};

struct Rotate
{
	HMM_Vec3 axis;
	float speed = 0;
	HMM_Quat base;
};

struct MoveInterpolation
{
	HMM_Vec3 value;
};

struct RotateInterpolation
{
	HMM_Quat value;
};

struct LocalTransform
{
	HMM_Mat4 model{ HMM_M4_Identity };
};

struct WorldTransform
{
	HMM_Mat4 value{ HMM_M4_Identity };
};

struct ShowMatrix
{
	HMM_Mat4 model;
};

struct Rendable
{
	int material;
	int mesh;
};

struct Light
{
	HMM_Vec4 color;
};

struct Camera
{
	ecs_entity_t window_entity;
	float fov;
	float nearPlane;
	float farPlane;
	int width;
	int height;
};

struct CameraMatrix
{
	HMM_Mat4 view;
	HMM_Mat4 proj;
};

struct PassData
{
	HMM_Mat4	vpMatrix;
};

struct MaterialData
{
	HMM_Vec4	albedo;
};

struct RenderObject
{
	int material;
	int mesh;
	HMM_Mat4 wMatrix;
};

struct ObjectData
{
	HMM_Mat4	wMatrix;
};

struct ViewRenderPacket
{
	ecs_entity_t window_entity;
	PassData passData;
	std::pmr::vector<RenderObject> renderObjects;
	std::pmr::vector<ObjectData> renderData;
};

struct SystemContext
{
	float delta_time;
	float time_since_startup;
	double delta_time_double;
	double time_since_startup_double;
	float interpolation_time;
	double interpolation_time_double;
};

void doSimpleHarmonicMove(flecs::iter& it, size_t i, const SimpleHarmonic& simpleHarmonic, Position& position)
{
	const SystemContext& context = *it.param<SystemContext>();
	position.value = simpleHarmonic.amplitude * sin(simpleHarmonic.speed * context.time_since_startup) + simpleHarmonic.base;
}

void doRotation(flecs::iter& it, size_t i, const Rotate& rotate, Rotation& rotation)
{
	const SystemContext& context = *it.param<SystemContext>();
	rotation.value = rotate.base * HMM_QFromAxisAngle_LH(rotate.axis, context.time_since_startup * rotate.speed);
}

void updateMatrixPositionOnly(const Position& position, LocalTransform& matrix)
{
	matrix.model = HMM_Translate(position.value);
}

void updateMatrixRotationOnly(const Rotation& rotation, LocalTransform& matrix)
{
	matrix.model = HMM_QToM4(rotation.value);
}

void updateMatrixPositionAndRotation(const Position& position, const Rotation& rotation, LocalTransform& matrix)
{
	matrix.model = HMM_TRS(position.value, rotation.value, HMM_V3_One);
}

void updateTreeTransform(flecs::world world, flecs::entity entity, const Tree& entityTree, HMM_Mat4 parentTransform)
{
	auto local = entity.try_get<LocalTransform>();
	auto worldTr = entity.try_get_mut<WorldTransform>();
	HMM_Mat4 selfWorldTransform;
	if (worldTr && local)
	{
		selfWorldTransform = worldTr->value = parentTransform * local->model;
	}
	else if (worldTr)
	{
		selfWorldTransform = worldTr->value = parentTransform;
	}
	else
	{
		selfWorldTransform = parentTransform;
	}

	ecs_entity_t child = entityTree.firstChild;
	while (child != kNullEntity)
	{
		auto childEnt = flecs::entity(world, child);
		auto& childTree = childEnt.get<Tree>();
		updateTreeTransform(world, childEnt, childTree, selfWorldTransform);
		child = childTree.nextSibling;
	}
}

void updateHierarchyTransform(flecs::iter& it, size_t i, const Tree& entityTree)
{
	if (entityTree.parent != kNullEntity) return;
	updateTreeTransform(it.world(), it.entity(i), entityTree, HMM_M4_Identity);
}

void updateNonHierarchyTrasform(const LocalTransform& local, WorldTransform& world)
{
	world.value = local.model;
}

void updateMoveInterpolation(flecs::iter& it, size_t i, const SimpleHarmonic& simpleHarmonic, MoveInterpolation& moveInterp)
{
	const SystemContext& context = *it.param<SystemContext>();
	auto pos1 = simpleHarmonic.amplitude * sin(simpleHarmonic.speed * context.time_since_startup) + simpleHarmonic.base;
	auto pos2 = simpleHarmonic.amplitude * sin(simpleHarmonic.speed * (context.time_since_startup + context.interpolation_time)) + simpleHarmonic.base;
	moveInterp.value = pos2 - pos1;
}

void updateRotateInterpolation(flecs::iter& it, size_t i, const Rotate& rotate, RotateInterpolation& rotateInterp)
{
	const SystemContext& context = *it.param<SystemContext>();
	auto rot1 = rotate.base * HMM_QFromAxisAngle_LH(rotate.axis, context.time_since_startup * rotate.speed);
	auto rot2 = rotate.base * HMM_QFromAxisAngle_LH(rotate.axis, (context.time_since_startup + context.interpolation_time) * rotate.speed);
	rotateInterp.value = HMM_MulQ(HMM_InvQ(rot2), rot1);
}

void updateShowMatrixStatic(const WorldTransform& matrix, ShowMatrix& showMatrix)
{
	showMatrix.model = matrix.value;
}

void updateShowMatrixMoveOnly(const WorldTransform& matrix, const MoveInterpolation& moveInterp, ShowMatrix& showMatrix)
{
	showMatrix.model = HMM_MulM4(matrix.value, HMM_Translate(moveInterp.value));
}

void updateShowMatrixRotateOnly(const WorldTransform& matrix, const RotateInterpolation& rotateInterp, ShowMatrix& showMatrix)
{
	auto oriPosition = HMM_M4GetTranslate(matrix.value);
	auto oriRotation = HMM_M4ToQ_LH(matrix.value);
	auto newRotation = HMM_MulQ(oriRotation, rotateInterp.value);
	showMatrix.model = HMM_TRS(oriPosition, newRotation, HMM_V3_One);
}

void updateShowMatrixMoveAndRotate(const WorldTransform& matrix, const MoveInterpolation& moveInterp, const RotateInterpolation& rotateInterp, ShowMatrix& showMatrix)
{
	auto oriPosition = HMM_M4GetTranslate(matrix.value);
	auto oriRotation = HMM_M4ToQ_LH(matrix.value);
	auto newPosition = oriPosition + moveInterp.value;
	auto newRotation = HMM_MulQ(oriRotation, rotateInterp.value);
	showMatrix.model = HMM_TRS(newPosition, newRotation, HMM_V3_One);
}

struct FrameRenderPacket
{
	std::pmr::synchronized_pool_resource* memory_resource;
	std::pmr::vector<ViewRenderPacket> viewDatas;

	FrameRenderPacket(std::pmr::synchronized_pool_resource* memory_resource)
		: memory_resource(memory_resource), viewDatas(memory_resource)
	{
	}

	void clear()
	{
		viewDatas.clear();
	}
};

struct Application
{
	oval_device_t* device{ nullptr };
	ecs_entity_t window1{};
	std::vector<HGEGraphics::Mesh*> meshes;
	std::vector<HGEGraphics::Material*> materials;
	std::pmr::synchronized_pool_resource root_memory_resource;
	std::array<FrameRenderPacket, 2> frameRenderPackets;
	int quad;
	int appleMat, snakeHeadMat, snakeBodyMat;
	int boardMat;

	flecs::system systemDoSimpleHarmonicMove;
	flecs::system systemDoRotation;
	flecs::system systemUpdateMatrixPositionOnly;
	flecs::system systemUpdateMatrixRotationOnly;
	flecs::system systemUpdateMatrixPositionAndRotation;
	flecs::system systemUpdateHierarchyTransform;
	flecs::system systemUpdateNonHierarchyTrasform;

	flecs::system systemSnakeMove;

	flecs::system systemUpdateMoveInterpolation;
	flecs::system systemUpdateRotateInterpolation;
	flecs::system systemShowMatrixStatic;
	flecs::system systemShowMatrixMoveOnly;
	flecs::system systemShowMatrixRotateOnly;
	flecs::system systemShowMatrixMoveAndRotate;

	Application()
		: frameRenderPackets{ FrameRenderPacket{&root_memory_resource}, FrameRenderPacket{&root_memory_resource} }
	{
		void* buffer = nullptr;
		buffer = frameRenderPackets[0].memory_resource->allocate(1024 * 1024, 8);
		frameRenderPackets[0].memory_resource->deallocate(buffer, 1024 * 1024, 8);
		buffer = frameRenderPackets[1].memory_resource->allocate(1024 * 1024, 8);
		frameRenderPackets[1].memory_resource->deallocate(buffer, 1024 * 1024, 8);
	}
};

enum class Direction
{
	Right,
	Up,
	Left,
	Down,
};

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

struct SnakeMove
{
	float interval;
	float lastTime;
};

struct Border
{
	int up, bottom, left, right;
};

struct Score
{
	int value;
};

struct SnakeInput
{
	SDL_Keycode rightKey, upKey, leftKey, downKey;
};

struct Snake
{
	flecs::entity head;
	std::vector<flecs::entity> bodies;
};

struct IsApple {};

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

	int maxCount = (right - left + 1) * (up - bottom + 1);

	if (snake.bodies.size() + 1 >= maxCount)
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
		int x = dis(gen, std::uniform_int_distribution<>::param_type(left + 1, right));
		int y = dis(gen, std::uniform_int_distribution<>::param_type(bottom + 1, up));
		auto newPos = HMM_V3(x, y, 0);

		for (int i = 0; i < used.size(); ++i)
		{
			if (used[i] == newPos)
				continue;
		}
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
		.set<SnakeInput>({ .rightKey = SDLK_RIGHT, .upKey = SDLK_UP, .leftKey = SDLK_LEFT, .downKey = SDLK_DOWN })
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
	snake.set<Snake>({.head = head, .bodies = std::move(bodies)});
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
	auto width = right - left + 1;
	auto height = up - bottom + 1;
	createBoard(world, HMM_TRS(HMM_V3(right, 0 + 0.5, 0), HMM_Q_Identity, HMM_V3(1, height, 1)), borderMat, quad);
	createBoard(world, HMM_TRS(HMM_V3(left, 0 + 0.5, 0), HMM_Q_Identity, HMM_V3(1, height, 1)), borderMat, quad);
	createBoard(world, HMM_TRS(HMM_V3(0 + 0.5, up, 0), HMM_Q_Identity, HMM_V3(width, 1, 1)), borderMat, quad);
	createBoard(world, HMM_TRS(HMM_V3(0 + 0.5, bottom, 0), HMM_Q_Identity, HMM_V3(width, 1, 1)), borderMat, quad);

	auto border = world.entity();
	border.add<Border>()
		.set<Border>({ .up = up, .bottom = bottom, .left = left,  .right = right });
	return border;
}

struct SystemSnakeMove
{
	flecs::query<IsApple, Position> apple;
	flecs::query<Border> border;
};

void snakeMove(flecs::iter& it, size_t i, const Snake& snake)
{
	const SystemContext& context = *it.param<SystemContext>();
	auto head = snake.head;
	auto& headMove = head.get_mut<SnakeMove>();

	headMove.lastTime += context.delta_time;
	if (headMove.lastTime < headMove.interval)
		return;

	auto& headPosition = head.get_mut<Position>();
	auto& headDirection = head.get<Direction>();
	auto nextPos = headPosition.value + toDelta(headDirection);

	auto& systemSnameMove = it.system().get<SystemSnakeMove>();
	auto appleEnt = systemSnameMove.apple.first();
	auto& applePosition = appleEnt.get<Position>();

	auto borderEnt = systemSnameMove.border.first();
	auto& border = borderEnt.get<Border>();

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

void _init_resource(Application& app, flecs::world& world)
{
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
	CGPUBlendStateDescriptor blend_desc = {
		.attachment_count = 1,
		.p_attachments = &blend_attachments,
		.alpha_to_coverage = false,
		.independent_blend = false,
	};
	CGPUDepthStateDescriptor depth_desc = {
		.depth_test = true,
		.depth_write = true,
		.depth_op = CGPU_COMPARE_OP_GREATER_EQUAL,
		.stencil_test = false,
	};
	CGPURasterizerStateDescriptor rasterizer_state = {
		.cull_mode = CGPU_CULL_MODE_BACK,
		.front_face	= CGPU_FRONT_FACE_CLOCK_WISE,
	};
	auto shader = oval_create_shader(app.device, "shaderbin/color.vert.spv", "shaderbin/color.frag.spv", blend_desc, depth_desc, rasterizer_state);

	auto quad = oval_load_mesh(app.device, "media/models/Quad.obj");
	app.quad = app.meshes.size();
	app.meshes.push_back(quad);

	{
		auto material = oval_create_material(app.device, shader);
		auto materialData = MaterialData{
			.albedo = HMM_V4(1, 1, 1, 1),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		app.boardMat = app.materials.size();
		app.materials.push_back(material);
	}

	{
		auto material = oval_create_material(app.device, shader);
		auto materialData = MaterialData{
			.albedo = HMM_V4(1, 0, 0, 0),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		app.appleMat = app.materials.size();
		app.materials.push_back(material);
	}

	{
		auto material = oval_create_material(app.device, shader);
		auto materialData = MaterialData{
			.albedo = HMM_V4(1, 1, 0, 1),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		app.snakeHeadMat = app.materials.size();
		app.materials.push_back(material);
	}

	{
		auto material = oval_create_material(app.device, shader);
		auto materialData = MaterialData{
			.albedo = HMM_V4(0, 1, 0, 1),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		app.snakeBodyMat = app.materials.size();
		app.materials.push_back(material);
	}

	int up = 16;
	int bottom = -15;
	int left = -20;
	int right = 21;
	auto border = createBorder(world, app.quad, app.boardMat, up, bottom, left, right);
	auto snake = createSnake(world, app.quad, app.snakeHeadMat, app.snakeBodyMat, HMM_V3(0, 0, 0));
	auto apple = createApple(world, snake.get<Snake>(), border.get<Border>(), app.quad, app.appleMat);
}

void _free_resource(Application& app)
{
	app.materials.clear();
}

void _init_world(Application& app, flecs::world& world, ecs_entity_t window_entity)
{
	auto cam = world.entity();
	auto cameraParentMat = HMM_M4_Identity;
	auto cameraLocalMat = HMM_Translate(HMM_V3(0 + 0.5, 0 + 0.5, -38));
	auto cameraWMat = HMM_Mul(cameraParentMat, cameraLocalMat);
	cam.add<WorldTransform>()
		.add<Camera>();
	cam.set<WorldTransform>({ .value = cameraWMat })
		.set<Camera>({ .window_entity = window_entity, .fov = 45.0f, .nearPlane = 0.1f, .farPlane = 1000.f, .width = 800, .height = 600 });

	app.systemDoSimpleHarmonicMove = world.system<const SimpleHarmonic, Position>("DoSimpleHarmonicMove")
		.each(doSimpleHarmonicMove);

	app.systemDoRotation = world.system<const Rotate, Rotation>("DoRotation")
		.each(doRotation);

	app.systemUpdateMatrixPositionOnly = world.system<const Position, LocalTransform>("UpdateMatrixPositionOnly")
		.without<Rotation>()
		.each(updateMatrixPositionOnly);

	app.systemUpdateMatrixRotationOnly = world.system<const Rotation, LocalTransform>("UpdateMatrixRotationOnly")
		.without<Position>()
		.each(updateMatrixRotationOnly);

	app.systemUpdateMatrixPositionAndRotation = world.system<const Position, const Rotation, LocalTransform>("UpdateMatrixPositionAndRotation")
		.each(updateMatrixPositionAndRotation);

	app.systemUpdateHierarchyTransform = world.system<const Tree>("UpdateHierarchyTransform")
		.each(updateHierarchyTransform);

	app.systemUpdateNonHierarchyTrasform = world.system<const LocalTransform, WorldTransform>("UpdateNonHierarchyTrasform")
		.without<Tree>()
		.each(updateNonHierarchyTrasform);

	app.systemSnakeMove = world.system<const Snake>("SnakeMove")
		.each(snakeMove);

	app.systemSnakeMove.add<SystemSnakeMove>();
	app.systemSnakeMove.set<SystemSnakeMove>({ .apple = world.query<IsApple, Position>(), .border = world.query<Border>() });

	app.systemUpdateMoveInterpolation = world.system<const SimpleHarmonic, MoveInterpolation>("UpdateMoveInterpolation")
		.each(updateMoveInterpolation);

	app.systemUpdateRotateInterpolation = world.system<const Rotate, RotateInterpolation>("UpdateRotateInterpolation")
		.each(updateRotateInterpolation);

	app.systemShowMatrixStatic = world.system<const WorldTransform, ShowMatrix>("ShowMatrixStatic")
		.without<MoveInterpolation, RotateInterpolation>()
		.each(updateShowMatrixStatic);

	app.systemShowMatrixMoveOnly = world.system<const WorldTransform, const MoveInterpolation, ShowMatrix>("ShowMatrixMoveOnly")
		.without<RotateInterpolation>()
		.each(updateShowMatrixMoveOnly);

	app.systemShowMatrixRotateOnly = world.system<const WorldTransform, const RotateInterpolation, ShowMatrix>("ShowMatrixRotateOnly")
		.without<MoveInterpolation>()
		.each(updateShowMatrixRotateOnly);

	app.systemShowMatrixMoveAndRotate = world.system<const WorldTransform, const MoveInterpolation, const RotateInterpolation, ShowMatrix>("ShowMatrixMoveAndRotate")
		.each(updateShowMatrixMoveAndRotate);
}

static void simulate(Application& app, const oval_update_context& update_context)
{
	SystemContext context = SystemContext{ update_context.delta_time, update_context.time_since_startup, update_context.delta_time_double, update_context.time_since_startup_double, 0, 0 };
	app.systemDoSimpleHarmonicMove.run(0, &context);
	app.systemDoRotation.run(0, &context);
	app.systemSnakeMove.run(0, &context);
	app.systemUpdateMatrixPositionOnly.run(0, &context);
	app.systemUpdateMatrixRotationOnly.run(0, &context);
	app.systemUpdateMatrixPositionAndRotation.run(0, &context);
	app.systemUpdateHierarchyTransform.run(0, &context);
	app.systemUpdateNonHierarchyTrasform.run(0, &context);
}

std::pmr::vector<ecs_entity_t> vis(Application& app, flecs::world& world, const Camera& camera, std::pmr::synchronized_pool_resource* memory_resource)
{
	std::pmr::vector<ecs_entity_t> visibles(memory_resource);
	world.each([&](flecs::entity e, const ShowMatrix& matrix, const Rendable& rendable) {
		(void)matrix; (void)rendable;
		bool visible = true;
		if (visible)
			visibles.push_back(e.id());
	});
	return visibles;
}

std::pmr::vector<RenderObject> extract(Application& app, flecs::world& world, std::pmr::vector<ecs_entity_t> visibles, std::pmr::synchronized_pool_resource* memory_resource)
{
	std::pmr::vector<RenderObject> renderObjects(memory_resource);
	renderObjects.reserve(visibles.size());
	for (auto entity_id : visibles)
	{
		auto entity = flecs::entity(world, entity_id);
		auto matrix = entity.try_get<ShowMatrix>();
		auto rendable = entity.try_get<Rendable>();
		if (!matrix || !rendable) continue;
		RenderObject robj = {
			.material = rendable->material,
			.mesh = rendable->mesh,
			.wMatrix = matrix->model,
		};
		renderObjects.push_back(robj);
	}
	return renderObjects;
}

void interpolate(Application& app, flecs::world& world, const oval_render_context& render_context)
{
	SystemContext context = SystemContext{ render_context.delta_time, render_context.time_since_startup, render_context.delta_time_double, render_context.time_since_startup_double, render_context.render_interpolation_time, render_context.render_interpolation_time_double };
	app.systemUpdateMoveInterpolation.run(0, &context);
	app.systemUpdateRotateInterpolation.run(0, &context);
	app.systemShowMatrixStatic.run(0, &context);
	app.systemShowMatrixMoveOnly.run(0, &context);
	app.systemShowMatrixRotateOnly.run(0, &context);
	app.systemShowMatrixMoveAndRotate.run(0, &context);
}

void enumViews(Application& app, flecs::world& world, FrameRenderPacket& currentFramePack)
{
	auto lightDir = HMM_Norm(HMM_V3(0, -1, 0));
	bool firstLight = true;
	world.each([&](flecs::entity e, Light& light, const WorldTransform& transform) {
		(void)e; (void)light;
		if (firstLight) {
			lightDir = HMM_M4GetForward(transform.value);
			firstLight = false;
		}
	});

	currentFramePack.clear();
	world.each([&](flecs::entity e, Camera& camera, const WorldTransform& transform) {
		(void)e;
		auto cameraMat = transform.value;

		auto eye = HMM_M4GetTranslate(cameraMat);
		auto forward = HMM_M4GetForward(cameraMat);
		(void)forward;
		auto viewMat = HMM_LookAt2_LH(eye, forward, HMM_V3_Up);

		if (!world.is_alive(camera.window_entity))
			return;

		auto window = flecs::entity(world, camera.window_entity).try_get<WindowComponent>();
		if (!window)
			return;

		camera.width = window->width;
		camera.height = window->height;

		float aspect = (float)camera.width / camera.height;
		float near = camera.nearPlane;
		float far = camera.farPlane;
		auto proj = HMM_Perspective_LH_RO(camera.fov * HMM_DegToRad, aspect, near, far);
		auto vpMat = proj * viewMat;

		auto visibles = vis(app, world, camera, currentFramePack.memory_resource);
		currentFramePack.viewDatas.emplace_back(ViewRenderPacket{
			.window_entity = camera.window_entity,
			.passData = {
				.vpMatrix = vpMat,
			},
			.renderObjects = extract(app, world, std::move(visibles), currentFramePack.memory_resource),
			});
	});
}

void prepare(Application& app, FrameRenderPacket& lastFrameRenderPacket)
{
	for (auto& view : lastFrameRenderPacket.viewDatas)
	{
		std::sort(view.renderObjects.begin(), view.renderObjects.end(), [](const RenderObject& a, const RenderObject& b)
			{
				if (a.material != b.material)
					return a.material > b.material;
				else
					return a.mesh > b.mesh;
			});

		view.renderData.resize(view.renderObjects.size());
		for (size_t i = 0; i < view.renderObjects.size(); ++i)
		{
			view.renderData[i].wMatrix = view.renderObjects[i].wMatrix;
		}
	}
}

void submit(Application& app, FrameRenderPacket& lastFrameRenderPacket, oval_device_t* device, HGEGraphics::rendergraph_t& rg)
{
	using namespace HGEGraphics;

	for (auto& view : lastFrameRenderPacket.viewDatas)
	{
		auto current_back_buffer = oval_get_backbuffer_for_window(device, view.window_entity, rg);
		if (!rendergraph_texture_handle_valid(current_back_buffer))
			continue;

		auto depth_handle = rendergraph_declare_texture(&rg);
		rg_texture_set_extent(&rg, depth_handle, rg_texture_get_width(&rg, current_back_buffer), rg_texture_get_height(&rg, current_back_buffer));
		rg_texture_set_depth_format(&rg, depth_handle, DepthBits::D24, true);

		auto pass_ubo_handle = rendergraph_declare_uniform_buffer_quick(&rg, sizeof(PassData), &view.passData);
		auto object_ubo_handle = rendergraph_declare_uniform_buffer_quick(&rg, view.renderData.size() * sizeof(ObjectData), view.renderData.data());

		auto passBuilder = rendergraph_add_renderpass(&rg, "Main Pass");
		uint32_t color = 0xff000000;
		renderpass_add_color_attachment(&passBuilder, current_back_buffer, ECGPULoadAction::CGPU_LOAD_ACTION_CLEAR, color, ECGPUStoreAction::CGPU_STORE_ACTION_STORE);
		renderpass_add_depth_attachment(&passBuilder, depth_handle, CGPU_LOAD_ACTION_CLEAR, 0, CGPU_STORE_ACTION_DISCARD, CGPU_LOAD_ACTION_CLEAR, 0, CGPU_STORE_ACTION_DISCARD);
		renderpass_use_buffer(&passBuilder, pass_ubo_handle);
		renderpass_use_buffer(&passBuilder, object_ubo_handle);

		struct MainPassPassData
		{
			Application* app;
			ViewRenderPacket* view;
			HGEGraphics::buffer_handle_t pass_ubo_handle;
			HGEGraphics::buffer_handle_t object_ubo_handle;
		};
		MainPassPassData* passdata;
		renderpass_set_executable(&passBuilder, [](RenderPassEncoder* encoder, void* passdata)
			{
				MainPassPassData* resolved_passdata = (MainPassPassData*)passdata;
				Application& app = *resolved_passdata->app;
				set_global_dynamic_buffer(encoder, resolved_passdata->pass_ubo_handle, 0, 0);
				for (size_t i = 0; i < resolved_passdata->view->renderObjects.size(); ++i)
				{
					auto& obj = resolved_passdata->view->renderObjects[i];
					set_global_buffer_with_offset_size(encoder, resolved_passdata->object_ubo_handle, 2, 0, i * sizeof(ObjectData), sizeof(ObjectData));
					draw(encoder, app.materials[obj.material], app.meshes[obj.mesh]);
				}
			}, sizeof(MainPassPassData), (void**)&passdata);
		passdata->app = &app;
		passdata->view = &view;
		passdata->pass_ubo_handle = pass_ubo_handle;
		passdata->object_ubo_handle = object_ubo_handle;
	}
}

void on_update(oval_device_t* device, oval_update_context update_context)
{
	Application& app = *(Application*)device->descriptor.userdata;
	simulate(app, update_context);
}

void on_render(oval_device_t* device, oval_render_context render_context)
{
	Application& app = *(Application*)device->descriptor.userdata;
	flecs::world world = flecs::world(oval_get_world(app.device));
	auto& cuurentFrameRenderPacket = app.frameRenderPackets[render_context.currentRenderPacketFrame];
	interpolate(app, world, render_context);
	enumViews(app, world, cuurentFrameRenderPacket);
	prepare(app, cuurentFrameRenderPacket);
}

void on_imgui1(ecs_entity_t entity, oval_device_t* device, oval_render_context render_context)
{
	ImGui::Text("Hello, ImGui!");
	ImGui::Text("%d", render_context.fps);
	ImGui::Text("%lf", render_context.delta_time_double);
	if (ImGui::Button("Capture"))
		oval_render_debug_capture(device);
	
	uint32_t length;
	const char** names;
	const float* durations;
	oval_query_render_profile(device, &length, &names, &durations);
	if (length > 0)
	{
		float total_duration = 0.f;
		for (uint32_t i = 0; i < length; ++i)
		{
			float duration = durations[i] * 1000;
			ImGui::Text("%s %7.2f us", names[i], duration);
			total_duration += duration;
		}
		ImGui::Text("Total Time: %7.2f us", total_duration);
	}
}

void on_window_close1(ecs_entity_t entity, oval_device_t* device)
{
	Application* app = (Application*)device->descriptor.userdata;
	app->window1 = kNullEntity;
}

void on_submit(oval_device_t* device, oval_submit_context submit_context, HGEGraphics::rendergraph_t& rg)
{
	Application* app = (Application*)device->descriptor.userdata;
	auto& lastFrameRenderPacket = app->frameRenderPackets[submit_context.submitRenderPacketFrame];
	submit(*app, lastFrameRenderPacket, device, rg);
}

void on_post_update(oval_device_t* device, oval_update_context update_context)
{
	oval_sync_window_component_and_raw_handle(device);
}

extern "C"
int SDL_main(int argc, char *argv[])
{
	const int width = 800;
	const int height = 600;
	Application app;
	oval_device_descriptor device_descriptor =
	{
		.userdata = &app,
		.on_update = on_update,
		.on_render = on_render,
		.on_submit = on_submit,
		.on_post_update = on_post_update,
		.update_frequecy_mode = UPDATE_FREQUENCY_MODE_VARIABLE,
		.fixed_update_time_step = 1.0 / 1,
		.render_frequecy_mode = RENDER_FREQUENCY_MODE_LIMITED,
		.render_need_interpolate = false,
		.target_fps = 100,
		.enable_capture = false,
		.enable_profile = false,
		.enable_gpu_validation = true,
	};
	app.device = oval_create_device(&device_descriptor);

	if (!app.device)
		return -1;

	oval_window_descriptor window_descriptor = {
		.width = width,
		.height = height,
		.primary = true,
		.resizable = false,
		.use_imgui = true,
		.own_imgui = true,
		.on_imgui = on_imgui1,
		.on_close = on_window_close1,
	};
	app.window1 = oval_create_window_entity(app.device, &window_descriptor);

	flecs::world world = flecs::world(oval_get_world(app.device));

	_init_resource(app, world);
	_init_world(app, world, app.window1);
		
	oval_runloop(app.device);
	_free_resource(app);
	oval_free_window_entity(app.device, app.window1);
	oval_free_device(app.device);

	return 0;
}