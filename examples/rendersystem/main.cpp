#include "framework.h"
#include "imgui.h"
#include <taskflow/taskflow.hpp>
#include "taskflow/algorithm/for_each.hpp"
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
	HMM_Vec4	lightDir;
	HMM_Vec4	viewPos;
};

struct MaterialData
{
	HMM_Vec4	shininess;
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
	ecs_entity_t window2{};
	std::vector<HGEGraphics::Mesh*> meshes;
	std::vector<HGEGraphics::Material*> materials;
	std::pmr::synchronized_pool_resource root_memory_resource;
	std::array<FrameRenderPacket, 2> frameRenderPackets;

	flecs::system systemDoSimpleHarmonicMove;
	flecs::system systemDoRotation;
	flecs::system systemUpdateMatrixPositionOnly;
	flecs::system systemUpdateMatrixRotationOnly;
	flecs::system systemUpdateMatrixPositionAndRotation;
	flecs::system systemUpdateHierarchyTransform;
	flecs::system systemUpdateNonHierarchyTrasform;

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

inline ECGPUFilterType find_min_filter(int min_filter)
{
	switch (min_filter)
	{
	case TINYGLTF_TEXTURE_FILTER_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
		return CGPU_FILTER_TYPE_NEAREST;
	case TINYGLTF_TEXTURE_FILTER_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		return CGPU_FILTER_TYPE_LINEAR;
	default:
		return CGPU_FILTER_TYPE_LINEAR;
	}
};

inline ECGPUMipMapMode find_mipmap_mode(int min_filter)
{
	switch (min_filter)
	{
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST:
		return CGPU_MIP_MAP_MODE_NEAREST;
	case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR:
	case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR:
		return CGPU_MIP_MAP_MODE_LINEAR;
	default:
		return CGPU_MIP_MAP_MODE_LINEAR;
	}
};

inline ECGPUFilterType find_mag_filter(int mag_filter)
{
	switch (mag_filter)
	{
	case TINYGLTF_TEXTURE_FILTER_NEAREST:
		return CGPU_FILTER_TYPE_NEAREST;
	case TINYGLTF_TEXTURE_FILTER_LINEAR:
		return CGPU_FILTER_TYPE_LINEAR;
	default:
		return CGPU_FILTER_TYPE_LINEAR;
	}
};

inline ECGPUAddressMode find_wrap_mode(int wrap)
{
	switch (wrap)
	{
	case TINYGLTF_TEXTURE_WRAP_REPEAT:
		return CGPU_ADDRESS_MODE_REPEAT;
	case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE:
		return CGPU_ADDRESS_MODE_CLAMP_TO_EDGE;
	case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT:
		return CGPU_ADDRESS_MODE_MIRROR;
	default:
		return CGPU_ADDRESS_MODE_REPEAT;
	}
};

struct TexturedVertex
{
	HMM_Vec3 position;
	HMM_Vec3 normal;
	HMM_Vec2 texCoord;
};

HGEGraphics::Texture* load_texture(Application& app, tinygltf::Image& gltf_image, std::string path, bool mipmap)
{
	return oval_load_texture(app.device, (path + gltf_image.uri).c_str(), true);

	mipmap = false;
	auto mipLevels = mipmap ? static_cast<uint32_t>(std::floor(std::log2(std::max(gltf_image.width, gltf_image.height)))) + 1 : 1;
	CGPUTextureDescriptor texture_desc =
	{
		.name = nullptr,
		.width = (uint64_t)gltf_image.width,
		.height = (uint64_t)gltf_image.height,
		.depth = 1,
		.array_size = 1,
		.format = CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB,
		.mip_levels = mipLevels,
		.descriptors = ECGPUResourceTypeFlags(mipmap ? CGPU_RESOURCE_TYPE_TEXTURE | CGPU_RESOURCE_TYPE_RENDER_TARGET : CGPU_RESOURCE_TYPE_TEXTURE),
	};

	auto texture = oval_create_texture_from_buffer(app.device, texture_desc, gltf_image.image.data(), gltf_image.image.size());
	return texture;
}

HGEGraphics::Mesh* load_primitive(Application& app, const tinygltf::Primitive& gltf_primitive, const tinygltf::Model& model, bool right_hand)
{
	int rh = right_hand ? -1 : 1;

	std::vector<HMM_Vec3> positions;
	std::vector<HMM_Vec3> normals;
	std::vector<HMM_Vec2> texcoords;
	for (auto& attribute : gltf_primitive.attributes)
	{
		auto& accessor = model.accessors[attribute.second];
		auto& bufferView = model.bufferViews[accessor.bufferView];
		auto& buffer = model.buffers[bufferView.buffer];
		auto stride = accessor.ByteStride(bufferView);
		auto startByte = accessor.byteOffset + bufferView.byteOffset;
		auto endByte = startByte + accessor.count * stride;
	
		if (attribute.first == "NORMAL")
		{
			normals.resize(accessor.count);
			memcpy(normals.data(), (buffer.data.data() + startByte), accessor.count * stride);
		}
		else if (attribute.first == "POSITION")
		{
			positions.resize(accessor.count);
			memcpy(positions.data(), (buffer.data.data() + startByte), accessor.count * stride);
		}
		else if (attribute.first == "TEXCOORD_0")
		{
			texcoords.resize(accessor.count);
			memcpy(texcoords.data(), (buffer.data.data() + startByte), accessor.count * stride);
		}
	}
	
	std::vector<TexturedVertex> vertices{ positions.size() };
	for (size_t i = 0; i < vertices.size(); ++i)
	{
		HMM_Vec3 position = i < positions.size() ? positions[i] : HMM_V3_Zero;
		position.X *= rh;
		HMM_Vec3 normal = i < normals.size() ? normals[i] : HMM_V3_Up;
		normal.X *= rh;
		HMM_Vec2 texcoord = i < texcoords.size() ? texcoords[i] : HMM_V2(0, 0);
		vertices[i] = { position, normal, texcoord };
	}
	
	CGPUVertexAttribute mesh_vertex_attributes[3] =
	{
		{ "POSITION", 1, CGPU_VERTEX_FORMAT_FLOAT32X3, 0, 0, sizeof(float) * 3, CGPU_VERTEX_INPUT_RATE_VERTEX },
		{ "NORMAL", 1, CGPU_VERTEX_FORMAT_FLOAT32X3, 0, sizeof(float) * 3, sizeof(float) * 3, CGPU_VERTEX_INPUT_RATE_VERTEX },
		{ "TEXCOORD", 1, CGPU_VERTEX_FORMAT_FLOAT32X2, 0, sizeof(float) * 6, sizeof(float) * 2, CGPU_VERTEX_INPUT_RATE_VERTEX },
	};
	
	CGPUVertexLayout mesh_vertex_layout =
	{
		.attribute_count = 3,
		.p_attributes = mesh_vertex_attributes,
	};
	
	int indexCount = 0;
	int indexStride = 0;
	std::vector<uint32_t> indices32;
	std::vector<uint16_t> indices16;
	if (gltf_primitive.indices >= 0)
	{
		auto& accessor = model.accessors[gltf_primitive.indices];
		auto& bufferView = model.bufferViews[accessor.bufferView];
		auto& buffer = model.buffers[bufferView.buffer];
		auto stride = accessor.ByteStride(bufferView);
		auto startByte = accessor.byteOffset + bufferView.byteOffset;
		auto endByte = startByte + accessor.count * stride;
	
		indexCount = accessor.count;
		const uint8_t* indexData = buffer.data.data() + startByte;
		if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			indexStride = 2;
			indices16.resize(accessor.count);
			memcpy(indices16.data(), indexData, accessor.count * stride);
			if (right_hand)
			{
				for (size_t j = 0; j < indices16.size() / 3; ++j)
				{
					auto temp = indices16.at(j * 3 + 0);
					indices16.at(j * 3 + 0) = indices16.at(j * 3 + 2);
					indices16.at(j * 3 + 2) = temp;
				}
			}
		}
		else if (accessor.componentType == TINYGLTF_COMPONENT_TYPE_INT)
		{
			indexStride = 4;
			indices32.resize(accessor.count);
			memcpy(indices32.data(), indexData, accessor.count * stride);
			if (right_hand)
			{
				for (size_t j = 0; j < indices32.size() / 3; ++j)
				{
					auto temp = indices32.at(j * 3 + 0);
					indices32.at(j * 3 + 0) = indices32.at(j * 3 + 2);
					indices32.at(j * 3 + 2) = temp;
				}
			}
		}
	}
	
	return oval_create_mesh_from_buffer(app.device, vertices.size(), indexCount, CGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, mesh_vertex_layout, indexStride, (const uint8_t *)vertices.data(), indexStride == 2 ? (const uint8_t*)indices16.data() : (const uint8_t*)indices32.data(), false, false);
}

bool FileExists(const std::string& abs_filename, void* user_data)
{
	SDL_IOStream* rw = SDL_IOFromFile(abs_filename.c_str(), "rb");
	if (!rw)
	{
		return false;
	}
	SDL_CloseIO(rw);
	return true;
}

std::string ExpandFilePath(const std::string& filepath, void*)
{
	return filepath;
}

bool ReadWholeFile(std::vector<unsigned char>* out, std::string* err,
	const std::string& filepath, void*)
{
	SDL_IOStream* rw = SDL_IOFromFile(filepath.c_str(), "rb");
	if (!rw)
	{
		return false;
	}

	auto size = SDL_GetIOSize(rw);

	out->resize(size);
	SDL_ReadIO(rw, out->data(), sizeof(char) * size);
	SDL_CloseIO(rw);
	return true;
}

bool WriteWholeFile(std::string* err, const std::string& filepath,
	const std::vector<unsigned char>& contents, void*)
{
	SDL_IOStream* rw = SDL_IOFromFile(filepath.c_str(), "rb");
	if (!rw)
	{
		return false;
	}

	SDL_WriteIO(rw, contents.data(), contents.size() * 1);
	SDL_CloseIO(rw);
	return true;
}

bool GetFileSizeInBytes(size_t* filesize_out, std::string* err,
	const std::string& filepath, void*)
{
	SDL_IOStream* rw = SDL_IOFromFile(filepath.c_str(), "rb");
	if (!rw)
	{
		return false;
	}

	(*filesize_out) = SDL_GetIOSize(rw);
	//SDL_RWwrite(rw, contents.data(), contents.size(), 1);
	SDL_CloseIO(rw);
	return true;
}

void load_scene(Application& app, flecs::world& world, const char* filepath, HGEGraphics::Shader* shader)
{
	using namespace tinygltf;
	Model model;
	TinyGLTF loader;
	std::string err;
	std::string warn;

	FsCallbacks tiny_gltfFsCallback = {
		.FileExists = FileExists,
		.ExpandFilePath = ExpandFilePath,
		.ReadWholeFile = ReadWholeFile,
		.WriteWholeFile = WriteWholeFile,
		.GetFileSizeInBytes = GetFileSizeInBytes,
	};
	if (!loader.SetFsCallbacks(tiny_gltfFsCallback, &err))
	{
		printf("Err: %s\n", err.c_str());
	}

	std::filesystem::path path = filepath;
	path.remove_filename();
	bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, filepath);
	if (!warn.empty())
		printf("Warn: %s\n", warn.c_str());
	if (!err.empty())
		printf("Err: %s\n", err.c_str());
	if (!ret)
	{
		printf("Failed to parse: %s\n", filepath);
		return;
	}

	std::vector<CGPUSamplerId> samplers;
	for (size_t i = 0; i < model.samplers.size(); ++i)
	{
		auto& gltf_sampler = model.samplers[i];

		ECGPUFilterType minFilter = find_min_filter(gltf_sampler.minFilter);
		ECGPUFilterType magFilter = find_mag_filter(gltf_sampler.magFilter);
		ECGPUMipMapMode mipmapMode = find_mipmap_mode(gltf_sampler.minFilter);
		ECGPUAddressMode address_u = find_wrap_mode(gltf_sampler.wrapS);
		ECGPUAddressMode address_v = find_wrap_mode(gltf_sampler.wrapT);
		ECGPUAddressMode address_w = CGPU_ADDRESS_MODE_REPEAT;

		CGPUSamplerDescriptor texture_sampler_desc = {
			.min_filter = minFilter,
			.mag_filter = magFilter,
			.mipmap_mode = mipmapMode,
			.address_u = address_u,
			.address_v = address_v,
			.address_w = address_w,
			.mip_lod_bias = 0,
			.max_anisotropy = 1,
		};
		auto sampler = oval_create_sampler(app.device, &texture_sampler_desc);
		samplers.push_back(sampler);
	}

	std::vector<HGEGraphics::Texture*> textures;
	for (size_t i = 0; i < model.images.size(); ++i)
	{
		auto& gltf_image = model.images[i];
		auto texture = load_texture(app, gltf_image, path.string(), true);
		textures.push_back(texture);
	}

	for (size_t i = 0; i < model.materials.size(); ++i)
	{
		auto& gltf_material = model.materials[i];
		auto material = oval_create_material(app.device, shader);
		auto& baseColorTexture = gltf_material.pbrMetallicRoughness.baseColorTexture;
		if (baseColorTexture.index != -1)
		{
			auto& gltf_texture = model.textures[baseColorTexture.index];
			auto tex = textures[gltf_texture.source];
			material->bindTexture(1, 1, tex);
			material->bindSampler(1, 2, samplers[0]);
		}

		float roughness = gltf_material.pbrMetallicRoughness.roughnessFactor;
		float a2 = roughness * roughness;
		a2 = std::clamp(a2, 0.0001f, 0.999f);
		float shininess = 2 / a2 - 2;
		float specularLevel = 1 / (a2 * 3.14159265359f);
		auto materialData = MaterialData{
			.shininess = HMM_V4(shininess, specularLevel, 0, 0),
			.albedo = HMM_V4(gltf_material.pbrMetallicRoughness.baseColorFactor[0], 
				gltf_material.pbrMetallicRoughness.baseColorFactor[0], 
				gltf_material.pbrMetallicRoughness.baseColorFactor[0], 
				gltf_material.pbrMetallicRoughness.baseColorFactor[0]),
		};
		material->bindBuffer<MaterialData>(1, 0, materialData);
		app.materials.push_back(material);
	}

	struct TupleHasher {
		std::size_t operator()(const std::tuple<int, int>& key) const {
			// 使用标准库的哈希函数组合两个整数的哈希值
			auto h1 = std::hash<int>{}(std::get<0>(key));
			auto h2 = std::hash<int>{}(std::get<1>(key));

			// 常见的组合方式：异或或移位组合
			return h1 ^ (h2 << 1);
		}
	};

	std::unordered_map<std::tuple<int, int>, int, TupleHasher> meshes;
	for (size_t i = 0; i < model.meshes.size(); ++i)
	{
		auto& gltf_mesh = model.meshes[i];
		for (size_t j = 0; j < gltf_mesh.primitives.size(); ++j)
		{
			meshes.insert({ std::make_tuple<int, int>((int)i, (int)j), (int)app.meshes.size() });
			app.meshes.push_back(load_primitive(app, gltf_mesh.primitives[j], model, true));
		}
	}

	std::vector<flecs::entity> entities;
	for (size_t i = 0; i < model.nodes.size(); ++i)
	{
		auto& node = model.nodes[i];

		HMM_Mat4 matrix = HMM_M4_Identity;
		if (node.matrix.size() == 16)
		{
			matrix.Elements[0][0] = node.matrix[0];
			matrix.Elements[0][1] = node.matrix[1];
			matrix.Elements[0][2] = node.matrix[2];
			matrix.Elements[0][3] = node.matrix[3];

			matrix.Elements[1][0] = node.matrix[4];
			matrix.Elements[1][1] = node.matrix[5];
			matrix.Elements[1][2] = node.matrix[6];
			matrix.Elements[1][3] = node.matrix[7];

			matrix.Elements[2][0] = node.matrix[8];
			matrix.Elements[2][1] = node.matrix[9];
			matrix.Elements[2][2] = node.matrix[10];
			matrix.Elements[2][3] = node.matrix[11];

			matrix.Elements[3][0] = node.matrix[12];
			matrix.Elements[3][1] = node.matrix[13];
			matrix.Elements[3][2] = node.matrix[14];
			matrix.Elements[3][3] = node.matrix[15];
		}
		else
		{
			HMM_Vec3 translate = HMM_V3_Zero;
			HMM_Quat rot = HMM_Q_Identity;
			HMM_Vec3 scale = HMM_V3_One;
			if (node.translation.size() == 3)
				translate = HMM_V3(node.translation[0], node.translation[1], node.translation[2]);
			if (node.rotation.size() == 4)
				rot = HMM_Q(node.rotation[0], node.rotation[1], node.rotation[2], node.rotation[3]);
			if (node.scale.size() == 3)
				scale = HMM_V3(node.scale[0], node.scale[1], node.scale[2]);
			matrix = HMM_TRS(translate, rot, scale);
		}

		auto ent = world.entity();
		ent.add<LocalTransform>()
			.add<WorldTransform>();
		ent.set<LocalTransform>({.model = matrix});
		if (node.mesh != -1)
		{
			const auto& mesh = model.meshes[node.mesh];
			for (size_t j = 0; j < mesh.primitives.size(); ++j)
			{
				auto sub = world.entity();
				sub.add<WorldTransform>()
					.add<Rendable>()
					.add<ShowMatrix>();
				sub.set<WorldTransform>({ .value = HMM_M4_Identity })
					.set<Rendable>({.material = mesh.primitives[j].material, .mesh = meshes[std::tuple<int, int>(node.mesh, (int)j)]})
					.set<ShowMatrix>({.model = HMM_M4_Identity});
				setParent(world, sub, ent);
			}
		}
		entities.push_back(ent);
	}

	std::vector<bool> transed(entities.size());
	std::fill(transed.begin(), transed.end(), false);
	for (size_t i = 0; i < model.nodes.size(); ++i)
	{
		auto& node = model.nodes[i];

		for (size_t j = 0; j < node.children.size(); ++j)
		{
			auto& child = model.nodes[node.children[j]];
			setParent(world, entities[node.children[j]], entities[i]);
		}
	}

	entities[0].add<Rotation>()
		.add<Rotate>();
	entities[0].set<Rotation>({ .value = HMM_Q_Identity })
		.set<Rotate>({ .axis = HMM_V3_Up, .speed = 1.0f, .base = HMM_Q_Identity });
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
	auto shader = oval_create_shader(app.device, "shaderbin/obj2.vert.spv", "shaderbin/obj2.frag.spv", blend_desc, depth_desc, rasterizer_state);

	load_scene(app, world, "media/gltf/gltf-truck/CesiumMilkTruck.gltf", shader);
}

void _free_resource(Application& app)
{
	app.materials.clear();
}

void _init_world(Application& app, flecs::world& world, ecs_entity_t window_entity)
{
	auto cam = world.entity();
	auto cameraParentMat = HMM_QToM4(HMM_QFromEuler_YXZ(HMM_AngleDeg(33.4), HMM_AngleDeg(45), 0));
	auto cameraLocalMat = HMM_Translate(HMM_V3(0, 0, -10));
	auto cameraWMat = HMM_Mul(cameraParentMat, cameraLocalMat);
	cam.add<WorldTransform>()
		.add<Camera>();
	cam.set<WorldTransform>({ .value = cameraWMat })
		.set<Camera>({ .window_entity = window_entity, .fov = 45.0f, .nearPlane = 0.1f, .farPlane = 20.f, .width = 800, .height = 600 });

	auto light = world.entity();
	auto lightDir = HMM_Norm(HMM_V3(0.25f, -0.7f, 1.25f));
	auto lightMat = HMM_PoseAt_LH(HMM_V3_Zero, lightDir, HMM_V3_Up);
	light.add<WorldTransform>()
		.add<Light>();
	light.set<WorldTransform>({ .value = lightMat })
		.set<Light>({ .color = HMM_V4(1.0f, 1.0f, 1.0f, 1.0f) });

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

tf::Task simulate(Application& app, flecs::world& world, const oval_update_context& update_context, tf::Taskflow& flow)
{
	return flow.emplace([&app, update_context]() {
		SystemContext context = SystemContext{ update_context.delta_time, update_context.time_since_startup, update_context.delta_time_double, update_context.time_since_startup_double, 0, 0 };
		app.systemDoSimpleHarmonicMove.run(0, &context);
		app.systemDoRotation.run(0, &context);
		app.systemUpdateMatrixPositionOnly.run(0, &context);
		app.systemUpdateMatrixRotationOnly.run(0, &context);
		app.systemUpdateMatrixPositionAndRotation.run(0, &context);
		app.systemUpdateHierarchyTransform.run(0, &context);
		app.systemUpdateNonHierarchyTrasform.run(0, &context);
		});
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
				.lightDir = lightDir,
				.viewPos = HMM_V4V(eye, 0),
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

tf::Taskflow on_update(oval_device_t* device, oval_update_context update_context)
{
	Application& app = *(Application*)device->descriptor.userdata;
	flecs::world world = flecs::world(oval_get_world(app.device));

	tf::Taskflow flow;
	auto simulateTask = simulate(app, world, update_context, flow);

	return flow;
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

void on_imgui2(ecs_entity_t entity, oval_device_t* device, oval_render_context render_context)
{
	ImGui::Text("Hello, Window2!");
}

void on_window_close1(ecs_entity_t entity, oval_device_t* device)
{
	Application* app = (Application*)device->descriptor.userdata;
	app->window1 = kNullEntity;
}

void on_window_close2(ecs_entity_t entity, oval_device_t* device)
{
	Application* app = (Application*)device->descriptor.userdata;
	app->window2 = kNullEntity;
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
		.resizable = true,
		.use_imgui = true,
		.own_imgui = true,
		.on_imgui = on_imgui1,
		.on_close = on_window_close1,
	};
	app.window1 = oval_create_window_entity(app.device, &window_descriptor);

	oval_window_descriptor window_descriptor2 = {
		.width = width,
		.height = height,
		.resizable = true,
		.use_imgui = true,
		.own_imgui = true,
		.on_imgui = on_imgui2,
		.on_close = on_window_close2,
	};
	app.window2 = oval_create_window_entity(app.device, &window_descriptor2);
	flecs::world world = flecs::world(oval_get_world(app.device));

	_init_resource(app, world);
	_init_world(app, world, app.window1);
		
	oval_runloop(app.device);
	_free_resource(app);
	oval_free_window_entity(app.device, app.window2);
	oval_free_window_entity(app.device, app.window1);
	oval_free_device(app.device);

	return 0;
}