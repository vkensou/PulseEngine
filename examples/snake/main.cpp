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
#include "predefine_components.h"
#include "snake_module.h"

constexpr ecs_entity_t kNullEntity = 0;

struct SystemContext
{
	KeyboardState keyboardState;
	oval_update_context updateContext;
	oval_render_context renderContext;
};

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
	const SystemContext& app = *it.ctx<SystemContext>();
	const oval_render_context& context = app.renderContext;
	auto pos1 = simpleHarmonic.amplitude * sin(simpleHarmonic.speed * context.time_since_startup) + simpleHarmonic.base;
	auto pos2 = simpleHarmonic.amplitude * sin(simpleHarmonic.speed * (context.time_since_startup + context.render_interpolation_time)) + simpleHarmonic.base;
	moveInterp.value = pos2 - pos1;
}

void updateRotateInterpolation(flecs::iter& it, size_t i, const Rotate& rotate, RotateInterpolation& rotateInterp)
{
	const SystemContext& app = *it.ctx<SystemContext>();
	const oval_render_context& context = app.renderContext;
	auto rot1 = rotate.base * HMM_QFromAxisAngle_LH(rotate.axis, context.time_since_startup * rotate.speed);
	auto rot2 = rotate.base * HMM_QFromAxisAngle_LH(rotate.axis, (context.time_since_startup + context.render_interpolation_time) * rotate.speed);
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
	std::unique_ptr<oval_device_t, decltype(&oval_free_device)> device;
	ecs_entity_t window1{};
	std::vector<HGEGraphics::Mesh*> meshes;
	std::vector<HGEGraphics::Material*> materials;
	std::pmr::synchronized_pool_resource root_memory_resource;
	std::array<FrameRenderPacket, 2> frameRenderPackets;

	SystemContext systemContext;

	flecs::entity logicPipeline;
	flecs::entity postLogicPipeline;
	flecs::entity renderPipeline;

	pulse::EventCenter eventCenter;

	Application()
		: device(nullptr, oval_free_device), frameRenderPackets{ FrameRenderPacket{&root_memory_resource}, FrameRenderPacket{&root_memory_resource} }
	{
		void* buffer = nullptr;
		buffer = frameRenderPackets[0].memory_resource->allocate(1024 * 1024, 8);
		frameRenderPackets[0].memory_resource->deallocate(buffer, 1024 * 1024, 8);
		buffer = frameRenderPackets[1].memory_resource->allocate(1024 * 1024, 8);
		frameRenderPackets[1].memory_resource->deallocate(buffer, 1024 * 1024, 8);
	}
};



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
	auto shader = oval_create_shader(app.device.get(), "shaderbin/color.vert.spv", "shaderbin/color.frag.spv", blend_desc, depth_desc, rasterizer_state);

	auto quad = oval_load_mesh(app.device.get(), "media/models/Quad.obj");
	int quadIndex = app.meshes.size();
	app.meshes.push_back(quad);

	int boardMatIndex, appleMatIndex, snakeHeadMatIndex, snakeBodyMatIndex;

	{
		auto material = oval_create_material(app.device.get(), shader);
		auto materialData = MaterialData{
			.albedo = HMM_V4(1, 1, 1, 1),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		boardMatIndex = app.materials.size();
		app.materials.push_back(material);
	}

	{
		auto material = oval_create_material(app.device.get(), shader);
		auto materialData = MaterialData{
			.albedo = HMM_V4(1, 0, 0, 0),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		appleMatIndex = app.materials.size();
		app.materials.push_back(material);
	}

	{
		auto material = oval_create_material(app.device.get(), shader);
		auto materialData = MaterialData{
			.albedo = HMM_V4(1, 1, 0, 1),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		snakeHeadMatIndex = app.materials.size();
		app.materials.push_back(material);
	}

	{
		auto material = oval_create_material(app.device.get(), shader);
		auto materialData = MaterialData{
			.albedo = HMM_V4(0, 1, 0, 1),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		snakeBodyMatIndex = app.materials.size();
		app.materials.push_back(material);
	}

	auto snakeApp = world.singleton<pulse::SingleHolder>();
	snakeApp.add<pulse::EventTag>();
	snakeApp.set<SnakeResources>({ .quad = quadIndex, .appleMat = appleMatIndex, .snakeHeadMat = snakeHeadMatIndex, .snakeBodyMat = snakeBodyMatIndex, .boardMat = boardMatIndex });

	pulse::command_buffer command_buffer(world);
	initSnakeGameSystem(command_buffer, pulse::singleton_query<const SnakeResources>(world));
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

	app.logicPipeline = world.pipeline()
		.with(flecs::System)
		.with<pulse::LogicPipeline>()
		.build();

	app.postLogicPipeline = world.pipeline()
		.with(flecs::System)
		.with<pulse::PostLogicPipeline>()
		.build();

	int numKeys;
	auto currentKeyboardStates = SDL_GetKeyboardState(&numKeys);
	app.systemContext.keyboardState.lastKeys.resize(numKeys);
	app.systemContext.keyboardState.currentKeys.resize(numKeys);
	std::fill(app.systemContext.keyboardState.lastKeys.begin(), app.systemContext.keyboardState.lastKeys.end(), 0);
	memcpy(app.systemContext.keyboardState.currentKeys.data(), currentKeyboardStates, numKeys);

	world.system<const Position, LocalTransform>("UpdateMatrixPositionOnly")
		.without<Rotation>()
		.kind<pulse::PostLogicPipeline>()
		.each(updateMatrixPositionOnly);

	world.system<const Rotation, LocalTransform>("UpdateMatrixRotationOnly")
		.without<Position>()
		.kind<pulse::PostLogicPipeline>()
		.each(updateMatrixRotationOnly);

	world.system<const Position, const Rotation, LocalTransform>("UpdateMatrixPositionAndRotation")
		.kind<pulse::PostLogicPipeline>()
		.each(updateMatrixPositionAndRotation);

	world.system<const Tree>("UpdateHierarchyTransform")
		.kind<pulse::PostLogicPipeline>()
		.each(updateHierarchyTransform);

	world.system<const LocalTransform, WorldTransform>("UpdateNonHierarchyTrasform")
		.without<Tree>()
		.kind<pulse::PostLogicPipeline>()
		.each(updateNonHierarchyTrasform);

	app.renderPipeline = world.pipeline()
		.with(flecs::System)
		.with<pulse::RenderPipeline>()
		.build();

	world.system<const SimpleHarmonic, MoveInterpolation>("UpdateMoveInterpolation")
		.kind<pulse::RenderPipeline>()
		.each(updateMoveInterpolation);

	world.system<const Rotate, RotateInterpolation>("UpdateRotateInterpolation")
		.kind<pulse::RenderPipeline>()
		.each(updateRotateInterpolation);

	world.system<const WorldTransform, ShowMatrix>("ShowMatrixStatic")
		.without<MoveInterpolation, RotateInterpolation>()
		.kind<pulse::RenderPipeline>()
		.each(updateShowMatrixStatic);

	world.system<const WorldTransform, const MoveInterpolation, ShowMatrix>("ShowMatrixMoveOnly")
		.without<RotateInterpolation>()
		.kind<pulse::RenderPipeline>()
		.each(updateShowMatrixMoveOnly);

	world.system<const WorldTransform, const RotateInterpolation, ShowMatrix>("ShowMatrixRotateOnly")
		.without<MoveInterpolation>()
		.kind<pulse::RenderPipeline>()
		.each(updateShowMatrixRotateOnly);

	world.system<const WorldTransform, const MoveInterpolation, const RotateInterpolation, ShowMatrix>("ShowMatrixMoveAndRotate")
		.kind<pulse::RenderPipeline>()
		.each(updateShowMatrixMoveAndRotate);

	importModule(world, app.eventCenter);
}

static void simulate(Application& app, flecs::world world, const oval_update_context& update_context)
{
	int numKeys;
	auto currentKeyboardStates = SDL_GetKeyboardState(&numKeys);
	assert(numKeys == app.systemContext.keyboardState.lastKeys.size());
	assert(numKeys == app.systemContext.keyboardState.currentKeys.size());
	memcpy(app.systemContext.keyboardState.lastKeys.data(), app.systemContext.keyboardState.currentKeys.data(), numKeys);
	memcpy(app.systemContext.keyboardState.currentKeys.data(), currentKeyboardStates, numKeys);

	app.systemContext.updateContext = update_context;
	world.run_pipeline(app.logicPipeline, update_context.delta_time);
	world.run_pipeline(app.postLogicPipeline, update_context.delta_time);
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
	app.systemContext.renderContext = render_context;
	world.run_pipeline(app.renderPipeline, render_context.delta_time);
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
	flecs::world world = flecs::world(oval_get_world(app.device.get()));
	simulate(app, world, update_context);
}

void on_render(oval_device_t* device, oval_render_context render_context)
{
	Application& app = *(Application*)device->descriptor.userdata;
	flecs::world world = flecs::world(oval_get_world(app.device.get()));
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
	
	flecs::world world = flecs::world(oval_get_world(device));
	auto snakeApp = world.singleton<pulse::SingleHolder>();
	if (snakeApp.enabled())
	{
		auto& score = snakeApp.get<Score>();
		ImGui::Text("%d", score.value);
	}
	else
	{
		if (ImGui::Button("Restart"))
		{
			Application& app = *(Application*)device->descriptor.userdata;
			pulse::command_buffer command_buffer(world);
			restartSystem(command_buffer, pulse::singleton_query<const Border>(world), pulse::singleton_query<const SnakeResources>(world));
			snakeApp.enable();
		}
	}

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
	app.device.reset(oval_create_device(&device_descriptor));

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
	app.window1 = oval_create_window_entity(app.device.get(), &window_descriptor);

	flecs::world world = flecs::world(oval_get_world(app.device.get()));
	world.set_ctx(&app.systemContext);

	_init_resource(app, world);
	_init_world(app, world, app.window1);
		
	oval_runloop(app.device.get());
	_free_resource(app);
	if (app.window1 != 0)
		oval_free_window_entity(app.device.get(), app.window1);

	return 0;
}