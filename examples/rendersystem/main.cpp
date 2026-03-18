#include "framework.h"
#include "imgui.h"
#include <taskflow/taskflow.hpp>
#include "taskflow/algorithm/for_each.hpp"
#include "entt/entt.hpp"
#include <bit>
#include <filesystem>
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#define TINYGLTF_NO_FS
#include "tiny_gltf.h"
#include <cassert>
#include "imgui_entt_entity_editor.hpp"
#include <SDL3/SDL.h>

struct Tree
{
	entt::entity parent{ entt::null };
	entt::entity firstChild{ entt::null };
	entt::entity lastChild{ entt::null };
	entt::entity previousSibling{ entt::null };
	entt::entity nextSibling{ entt::null };
};

inline Tree& safeGetTree(entt::registry& registry, entt::entity self)
{
	return registry.get_or_emplace<Tree>(self);
}

void removeFromParent(entt::registry& registry, entt::entity self, Tree& selfTree);

void insertBefore(entt::registry& registry, entt::entity self, Tree& selfTree, entt::entity newNode, entt::entity referenceNode)
{
	assert(newNode != entt::null && "newNode is null");

	auto& newTree = registry.get<Tree>(newNode);
	removeFromParent(registry, newNode, newTree);

	if (referenceNode != entt::null)
	{
		auto& referenceTree = registry.get<Tree>(referenceNode);
		assert(referenceTree.parent == self);

		newTree.previousSibling = referenceTree.previousSibling;
		newTree.nextSibling = referenceNode;

		if (newTree.previousSibling == entt::null)
			selfTree.firstChild = newNode;
		else 
		{
			auto& previousTree = registry.get<Tree>(newTree.previousSibling);
			previousTree.nextSibling = newNode;
		}
	}
	else
	{
		if (selfTree.lastChild != entt::null)
		{
			auto& lastTree = registry.get<Tree>(selfTree.lastChild);
			assert(lastTree.nextSibling == entt::null);
			lastTree.nextSibling = newNode;
			newTree.previousSibling = selfTree.lastChild;
			selfTree.lastChild = newNode;
		}
		else
		{
			assert(selfTree.firstChild == entt::null);
			assert(selfTree.lastChild == entt::null);
			selfTree.firstChild = newNode;
			selfTree.lastChild = newNode;
		}
	}
	newTree.parent = self;
}

void appendChild(entt::registry& registry, entt::entity self, Tree& selfTree, entt::entity child)
{
	assert(child != entt::null && "child is null");
	assert(child != self && "child is self");

	auto& childTree = registry.get<Tree>(child);
	removeFromParent(registry, child, childTree);
	childTree.parent = self;
	if (selfTree.lastChild != entt::null)
	{
		auto& lastChildTree = registry.get<Tree>(selfTree.lastChild);
		assert(lastChildTree.nextSibling == entt::null);
		childTree.previousSibling = selfTree.lastChild;
		lastChildTree.nextSibling = child;
	}
	else
	{
		assert(selfTree.firstChild == entt::null);
		selfTree.firstChild = child;
	}

	selfTree.lastChild = child;
}

void replaceChild(entt::registry& registry, entt::entity self, Tree& selfTree, entt::entity newChild, entt::entity oldChild)
{
	assert(newChild != entt::null && "newChild is null");
	assert(oldChild != entt::null && "oldChild is null");
	assert(newChild != self);
	auto& oldChildTree = registry.get<Tree>(oldChild);
	assert(oldChildTree.parent == self);
	const auto oldChildNext = oldChildTree.nextSibling;
	removeFromParent(registry, oldChild, oldChildTree);
	insertBefore(registry, self, selfTree, newChild, oldChildNext);
}

void removeChild(entt::registry& registry, entt::entity self, Tree& selfTree, entt::entity child)
{
	assert(child != entt::null && "child is null");
	auto& childTree = registry.get<Tree>(child);
	assert(childTree.parent == self);
	removeFromParent(registry, child, childTree);
}

void removeFromParent(entt::registry& registry, entt::entity self, Tree& selfTree)
{
	if (selfTree.parent == entt::null) return;

	auto& parentTree = registry.get<Tree>(selfTree.parent);

	if (parentTree.firstChild == self)
		parentTree.firstChild = selfTree.nextSibling;
	if (parentTree.lastChild == self)
		parentTree.lastChild = selfTree.previousSibling;

	if (selfTree.previousSibling != entt::null)
	{
		auto& previousTree = registry.get<Tree>(selfTree.previousSibling);
		previousTree.nextSibling = selfTree.nextSibling;
	}

	if (selfTree.nextSibling != entt::null)
	{
		auto& nextTree = registry.get<Tree>(selfTree.nextSibling);
		nextTree.previousSibling = selfTree.previousSibling;
	}

	selfTree.parent = entt::null;
	selfTree.previousSibling = entt::null;
	selfTree.nextSibling = entt::null;
}

void setParent(entt::registry& registry, entt::entity self, entt::entity newParent)
{
	auto& selfTree = safeGetTree(registry, self);
	removeFromParent(registry, self, selfTree);
	auto& parentTree = safeGetTree(registry, newParent);
	appendChild(registry, newParent, parentTree, self);
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
	entt::entity window_entity;
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
	entt::entity window_entity;
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

void doSimpleHarmonicMove(const SystemContext& context, const SimpleHarmonic& simpleHarmonic, Position& position)
{
	position.value = simpleHarmonic.amplitude * sin(simpleHarmonic.speed * context.time_since_startup) + simpleHarmonic.base;
}

void doRotation(const SystemContext& context, const Rotate& rotate, Rotation& rotation)
{
	rotation.value = rotate.base * HMM_QFromAxisAngle_LH(rotate.axis, context.time_since_startup * rotate.speed);
}

void updateMatrixPositionOnly(const SystemContext& context, const Position& position, LocalTransform& matrix)
{
	matrix.model = HMM_Translate(position.value);
}

void updateMatrixRotationOnly(const SystemContext& context, const Rotation& rotation, LocalTransform& matrix)
{
	matrix.model = HMM_QToM4(rotation.value);
}

void updateMatrixPositionAndRotation(const SystemContext& context, const Position& position, const Rotation& rotation, LocalTransform& matrix)
{
	matrix.model = HMM_TRS(position.value, rotation.value, HMM_V3_One);
}

void updateTreeTransform(entt::registry& registry, entt::entity entity, const Tree& entityTree, HMM_Mat4 parentTransform)
{
	auto local = registry.try_get<LocalTransform>(entity);
	auto world = registry.try_get<WorldTransform>(entity);
	HMM_Mat4 selfWorldTransform;
	if (world && local)
	{
		selfWorldTransform = world->value = parentTransform * local->model;
	}
	else if (world)
	{
		selfWorldTransform = world->value = parentTransform;
	}
	else
	{
		selfWorldTransform = parentTransform;
	}

	entt::entity child = entityTree.firstChild;
	while (child != entt::null)
	{
		auto& childTree = registry.get<Tree>(child);
		updateTreeTransform(registry, child, childTree, selfWorldTransform);
		child = childTree.nextSibling;
	}
}

void updateHierarchyTransform(const SystemContext& context, entt::registry& registry, entt::entity entity, const Tree& entityTree)
{
	if (entityTree.parent != entt::null) return;
	updateTreeTransform(registry, entity, entityTree, HMM_M4_Identity);
}

void updateNonHierarchyTrasform(const SystemContext& context, const LocalTransform& local, WorldTransform& world)
{
	world.value = local.model;
}

void updateMoveInterpolation(const SystemContext& context, const SimpleHarmonic& simpleHarmonic, MoveInterpolation& moveInterp)
{
	auto pos1 = simpleHarmonic.amplitude* sin(simpleHarmonic.speed * context.time_since_startup) + simpleHarmonic.base;
	auto pos2 = simpleHarmonic.amplitude * sin(simpleHarmonic.speed * (context.time_since_startup + context.interpolation_time)) + simpleHarmonic.base;
	moveInterp.value = pos2 - pos1;
}

void updateRotateInterpolation(const SystemContext& context, const Rotate& rotate, RotateInterpolation& rotateInterp)
{
	auto rot1 = rotate.base * HMM_QFromAxisAngle_LH(rotate.axis, context.time_since_startup * rotate.speed);
	auto rot2 = rotate.base * HMM_QFromAxisAngle_LH(rotate.axis, (context.time_since_startup + context.interpolation_time) * rotate.speed);
	rotateInterp.value = HMM_MulQ(HMM_InvQ(rot2), rot1);
}

void updateShowMatrixStatic(const SystemContext& context, const WorldTransform& matrix, ShowMatrix& showMatrix)
{
	showMatrix.model = matrix.value;
}

void updateShowMatrixMoveOnly(const SystemContext& context, const WorldTransform& matrix, const MoveInterpolation& moveInterp, ShowMatrix& showMatrix)
{
	showMatrix.model = HMM_MulM4(matrix.value, HMM_Translate(moveInterp.value));
}

void updateShowMatrixRotateOnly(const SystemContext& context, const WorldTransform& matrix, const RotateInterpolation& rotateInterp, ShowMatrix& showMatrix)
{
	auto oriPosition = HMM_M4GetTranslate(matrix.value);
	auto oriRotation = HMM_M4ToQ_LH(matrix.value);
	auto newRotation = HMM_MulQ(oriRotation, rotateInterp.value);
	showMatrix.model = HMM_TRS(oriPosition, newRotation, HMM_V3_One);
}

void updateShowMatrixMoveAndRotate(const SystemContext& context, const WorldTransform& matrix, const MoveInterpolation& moveInterp, const RotateInterpolation& rotateInterp, ShowMatrix& showMatrix)
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
	entt::entity window1{};
	entt::entity window2{};
	std::vector<HGEGraphics::Mesh*> meshes;
	std::vector<HGEGraphics::Material*> materials;
	std::pmr::synchronized_pool_resource root_memory_resource;
	std::array<FrameRenderPacket, 2> frameRenderPackets;
	MM::EntityEditor<entt::entity> enttEditor;
	entt::entity editorCurEntity;

	Application()
		: frameRenderPackets{ FrameRenderPacket{&root_memory_resource}, FrameRenderPacket{&root_memory_resource} }
	{
		void* buffer = nullptr;
		buffer = frameRenderPackets[0].memory_resource->allocate(1024 * 1024, 8);
		frameRenderPackets[0].memory_resource->deallocate(buffer, 1024 * 1024, 8);
		buffer = frameRenderPackets[1].memory_resource->allocate(1024 * 1024, 8);
		frameRenderPackets[1].memory_resource->deallocate(buffer, 1024 * 1024, 8);

		enttEditor.registerComponent<Tree>("Tree");
		enttEditor.registerComponent<WorldTransform>("World Transform");
	}
};

namespace MM {
	template <>
	void ComponentEditorWidget<Tree>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& t = reg.get<Tree>(e);
		ImGui::Text("parent: %d", t.parent);
		ImGui::Text("first child: %d", t.firstChild);
		ImGui::Text("last child: %d", t.lastChild);
		ImGui::Text("previous sibling: %d", t.previousSibling);
		ImGui::Text("next sibling: %d", t.nextSibling);
	}

	template <>
	void ComponentEditorWidget<WorldTransform>(entt::registry& reg, entt::registry::entity_type e)
	{
		auto& v = reg.get<WorldTransform>(e);
		
		ImGui::DragFloat("00", &v.value.Elements[0][0], 0.1f);
		ImGui::DragFloat("01", &v.value.Elements[0][1], 0.1f);
		ImGui::DragFloat("02", &v.value.Elements[0][2], 0.1f);
		ImGui::DragFloat("03", &v.value.Elements[0][3], 0.1f);

		ImGui::DragFloat("10", &v.value.Elements[1][0], 0.1f);
		ImGui::DragFloat("11", &v.value.Elements[1][1], 0.1f);
		ImGui::DragFloat("12", &v.value.Elements[1][2], 0.1f);
		ImGui::DragFloat("13", &v.value.Elements[1][3], 0.1f);

		ImGui::DragFloat("20", &v.value.Elements[2][0], 0.1f);
		ImGui::DragFloat("21", &v.value.Elements[2][1], 0.1f);
		ImGui::DragFloat("22", &v.value.Elements[2][2], 0.1f);
		ImGui::DragFloat("23", &v.value.Elements[2][3], 0.1f);

		ImGui::DragFloat("30", &v.value.Elements[3][0], 0.1f);
		ImGui::DragFloat("31", &v.value.Elements[3][1], 0.1f);
		ImGui::DragFloat("32", &v.value.Elements[3][2], 0.1f);
		ImGui::DragFloat("33", &v.value.Elements[3][3], 0.1f);
	}
}

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

void load_scene(Application& app, entt::registry& registry, const char* filepath, HGEGraphics::Shader* shader)
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

	std::vector<entt::entity> entities;
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

		auto ent = registry.create();
		registry.emplace<LocalTransform>(ent, matrix);
		registry.emplace<WorldTransform>(ent);
		if (node.mesh != -1)
		{
			const auto& mesh = model.meshes[node.mesh];
			for (size_t j = 0; j < mesh.primitives.size(); ++j)
			{
				auto sub = registry.create();
				registry.emplace<WorldTransform>(sub, HMM_M4_Identity);
				registry.emplace<Rendable>(sub, mesh.primitives[j].material, meshes[std::tuple<int, int>(node.mesh, (int)j)]);
				registry.emplace<ShowMatrix>(sub, HMM_M4_Identity);
				setParent(registry, sub, ent);
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
			setParent(registry, entities[node.children[j]], entities[i]);
		}
	}

	registry.emplace<Rotation>(entities[0], HMM_Q_Identity);
	registry.emplace<Rotate>(entities[0], HMM_V3_Up, 1.0f, HMM_Q_Identity);
}

void _init_resource(Application& app, entt::registry& registry)
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

	load_scene(app, registry, "media/gltf/gltf-truck/CesiumMilkTruck.gltf", shader);
}

void _free_resource(Application& app)
{
	app.materials.clear();
}

void _init_world(Application& app, entt::registry& registry, entt::entity window_entity)
{
	auto cam = registry.create();
	auto cameraParentMat = HMM_QToM4(HMM_QFromEuler_YXZ(HMM_AngleDeg(33.4), HMM_AngleDeg(45), 0));
	auto cameraLocalMat = HMM_Translate(HMM_V3(0, 0, -10));
	auto cameraWMat = HMM_Mul(cameraParentMat, cameraLocalMat);
	registry.emplace<WorldTransform>(cam, cameraWMat);
	registry.emplace<Camera>(cam, window_entity, 45.0f, 0.1f, 20.f, 800, 600);

	auto light = registry.create();
	auto lightDir = HMM_Norm(HMM_V3(0.25f, -0.7f, 1.25f));
	auto lightMat = HMM_PoseAt_LH(HMM_V3_Zero, lightDir, HMM_V3_Up);
	registry.emplace<WorldTransform>(light, lightMat);
	registry.emplace<Light>(light, HMM_V4(1.0f, 1.0f, 1.0f, 1.0f));
}

template<typename...T, typename Func>
tf::Task createForeachTask(const SystemContext& context, entt::registry& registry, tf::Taskflow& taskFlow, Func&& func)
{
	auto view = registry.view<T...>();
	return taskFlow.emplace([view, context, func = std::forward<Func>(func)](tf::Subflow& subflow) {
		subflow.for_each(view.begin(), view.end(), [view, context, func](auto entity)
			{
				auto args = std::tuple_cat(std::forward_as_tuple(context), view.get(entity));
				std::apply(func, std::move(args));
			});
		});
}

template<typename...T, typename...Exclude, typename Func>
tf::Task createForeachTask(const SystemContext& context, entt::registry& registry, tf::Taskflow& taskFlow, Func&& func, entt::exclude_t<Exclude...> exclude)
{
	auto view = registry.view<T...>(exclude);
	return taskFlow.emplace([view, context, func = std::forward<Func>(func)](tf::Subflow& subflow) {
		subflow.for_each(view.begin(), view.end(), [view, context, func](auto entity)
			{
				auto args = std::tuple_cat(std::forward_as_tuple(context), view.get(entity));
				std::apply(func, std::move(args));
			});
		});
}

template<typename...T, typename Func>
tf::Task createForeachTask2(const SystemContext& context, entt::registry& registry, tf::Taskflow& taskFlow, Func&& func)
{
	return taskFlow.emplace([&registry, context, func = std::forward<Func>(func)](tf::Subflow& subflow) {
		auto view = registry.view<T...>();
		for (auto entity : view)
		{
			auto args = std::tuple_cat(std::forward_as_tuple(context), std::forward_as_tuple(registry), std::forward_as_tuple(entity), view.get(entity));
			std::apply(func, std::move(args));
		}});
}

template<typename...T, typename Func>
void updateSystem(const SystemContext& context, entt::registry& registry, Func&& func)
{
	auto view = registry.view<T...>();
	for (auto entity : view)
	{
		auto args = std::tuple_cat(std::forward_as_tuple(context), view.get(entity));
		std::apply(std::forward<Func>(func), std::move(args));
	}
}

template<typename...T, typename...Exclude, typename Func>
void updateSystem(const SystemContext& context, entt::registry& registry, Func&& func, entt::exclude_t<Exclude...> exclude)
{
	auto view = registry.view<T...>(exclude);
	for (auto entity : view)
	{
		auto args = std::tuple_cat(std::forward_as_tuple(context), view.get(entity));
		std::apply(std::forward<Func>(func), std::move(args));
	}
}

tf::Task simulate(Application& app, entt::registry& registry, const oval_update_context& update_context, tf::Taskflow& flow)
{
	SystemContext context = SystemContext{ update_context.delta_time, update_context.time_since_startup, update_context.delta_time_double, update_context.time_since_startup_double, 0, 0 };
	auto simpleHarmonicMoveSystem = createForeachTask<const SimpleHarmonic, Position>(context, registry, flow, doSimpleHarmonicMove).name("简谐运动");
	auto rotationSystem = createForeachTask<const Rotate, Rotation>(context, registry, flow, doRotation).name("旋转运动");

	auto beforeUpdateMatrix = flow.placeholder();
	beforeUpdateMatrix
		.succeed(simpleHarmonicMoveSystem, rotationSystem);;

	auto updateMatrixPositionOnlySystem = createForeachTask<const Position, LocalTransform>(context, registry, flow, updateMatrixPositionOnly, entt::exclude<Rotation>).name("更新矩阵PositionOnly");
	auto updateMatrixRotationOnlySystem = createForeachTask<const Rotation, LocalTransform>(context, registry, flow, updateMatrixRotationOnly, entt::exclude<Position>).name("更新矩阵RotationOnly");
	auto updateMatrixPositionAndRotationSystem = createForeachTask<const Position, const Rotation, LocalTransform>(context, registry, flow, updateMatrixPositionAndRotation).name("更新矩阵PositionAndRotation");
	auto beforeUpdateLocalTransformSystemGroup = flow.placeholder().precede(updateMatrixPositionOnlySystem, updateMatrixRotationOnlySystem, updateMatrixPositionAndRotationSystem);
	auto afterUpdateLocalTransformSystemGroup = flow.placeholder().succeed(updateMatrixPositionOnlySystem, updateMatrixRotationOnlySystem, updateMatrixPositionAndRotationSystem);
	beforeUpdateLocalTransformSystemGroup.succeed(beforeUpdateMatrix);

	auto updateHierarchyTransformSystem = createForeachTask2<const Tree>(context, registry, flow, updateHierarchyTransform);
	auto updateNonHierarchyTransformSystem = createForeachTask<const LocalTransform, WorldTransform>(context, registry, flow, updateNonHierarchyTrasform, entt::exclude<Tree>).name("更新非层级WorldTransform");
	auto beforeUpdateHierarchyTransformSystemGroup = flow.placeholder().precede(updateHierarchyTransformSystem, updateNonHierarchyTransformSystem).succeed(afterUpdateLocalTransformSystemGroup);
	auto afterUpdateHierarchyTransformSystemGroup = flow.placeholder().succeed(updateHierarchyTransformSystem, updateNonHierarchyTransformSystem);

	auto endOfSimulate = flow.placeholder();
	endOfSimulate.succeed(afterUpdateHierarchyTransformSystemGroup);

	return endOfSimulate;
}

std::pmr::vector<entt::entity> vis(Application& app, entt::registry& registry, const Camera& camera, std::pmr::synchronized_pool_resource* memory_resource)
{
	auto view = registry.view<const ShowMatrix, const Rendable>();
	std::pmr::vector<entt::entity> visibles(memory_resource);
	visibles.reserve(view.size_hint());
	for (auto [entity, matrix, rendable] : view.each())
	{
		bool visible = true;
		if (visible)
		{
			visibles.push_back(entity);
		}
	}
	return visibles;
}

std::pmr::vector<RenderObject> extract(Application& app, entt::registry& registry, std::pmr::vector<entt::entity> visibles, std::pmr::synchronized_pool_resource* memory_resource)
{
	std::pmr::vector<RenderObject> renderObjects(memory_resource);
	renderObjects.reserve(visibles.size());
	for (auto entity : visibles)
	{
		auto matrix = registry.get<ShowMatrix>(entity);
		auto rendable = registry.get<Rendable>(entity);
		RenderObject robj = {
			.material = rendable.material,
			.mesh = rendable.mesh,
			.wMatrix = matrix.model,
		};
		renderObjects.push_back(robj);
	}
	return renderObjects;
}

void interpolate(Application& app, entt::registry& registry, const oval_render_context& render_context)
{
	SystemContext context = SystemContext{ render_context.delta_time, render_context.time_since_startup, render_context.delta_time_double, render_context.time_since_startup_double, render_context.render_interpolation_time, render_context.render_interpolation_time_double };
	updateSystem<const SimpleHarmonic, MoveInterpolation>(context, registry, updateMoveInterpolation);
	updateSystem<const Rotate, RotateInterpolation>(context, registry, updateRotateInterpolation);
	updateSystem<const WorldTransform, ShowMatrix>(context, registry, updateShowMatrixStatic, entt::exclude<MoveInterpolation, RotateInterpolation>);
	updateSystem<const WorldTransform, const MoveInterpolation, ShowMatrix>(context, registry, updateShowMatrixMoveOnly, entt::exclude<RotateInterpolation>);
	updateSystem<const WorldTransform, const RotateInterpolation, ShowMatrix>(context, registry, updateShowMatrixRotateOnly, entt::exclude<MoveInterpolation>);
	updateSystem<const WorldTransform, const MoveInterpolation, const RotateInterpolation, ShowMatrix>(context, registry, updateShowMatrixMoveAndRotate);
}

void enumViews(Application& app, entt::registry& registry, FrameRenderPacket& currentFramePack)
{
	auto lightDir = HMM_Norm(HMM_V3(0, -1, 0));

	auto light_view = registry.view<Light, const WorldTransform>();
	for (auto [entity, light, transform] : light_view.each())
	{
		auto forward = HMM_M4GetForward(transform.value);
		lightDir = forward;
		break;
	}

	currentFramePack.clear();
	auto camera_view = registry.view<Camera, const WorldTransform>();
	for (auto [entity, camera, transform] : camera_view.each())
	{
		auto cameraMat = transform.value;

		auto eye = HMM_M4GetTranslate(cameraMat);
		auto forward = HMM_M4GetForward(cameraMat);
		auto right = HMM_M4GetRight(cameraMat);
		auto viewMat = HMM_LookAt2_LH(eye, forward, HMM_V3_Up);

		if (!registry.valid(camera.window_entity))
			continue;

		auto window = registry.try_get<WindowComponent>(camera.window_entity);
		if (!window)
			continue;

		camera.width = window->width;
		camera.height = window->height;

		float aspect = (float)camera.width / camera.height;
		float near = camera.nearPlane;
		float far = camera.farPlane;
		auto proj = HMM_Perspective_LH_RO(camera.fov * HMM_DegToRad, aspect, near, far);
		auto vpMat = proj * viewMat;

		currentFramePack.viewDatas.emplace_back(ViewRenderPacket{
			.window_entity = camera.window_entity,
			.passData = {
				.vpMatrix = vpMat,
				.lightDir = lightDir,
				.viewPos = HMM_V4V(eye, 0),
			},
			.renderObjects = std::move(extract(app, registry, std::move(vis(app, registry, camera, currentFramePack.memory_resource)), currentFramePack.memory_resource)),
			});
	}
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
	auto& registry = *oval_get_registry(device);

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
	auto& registry = *oval_get_registry(app.device);

	tf::Taskflow flow;
	auto simulateTask = simulate(app, registry, update_context, flow);

	return flow;
}

void on_render(oval_device_t* device, oval_render_context render_context)
{
	Application* app = (Application*)device->descriptor.userdata;
	auto& registry = *oval_get_registry(device);
	auto& cuurentFrameRenderPacket = app->frameRenderPackets[render_context.currentRenderPacketFrame];
	interpolate(*app, registry, render_context);
	enumViews(*app, registry, cuurentFrameRenderPacket);
	prepare(*app, cuurentFrameRenderPacket);
}

void on_imgui1(entt::entity entity, oval_device_t* device, oval_render_context render_context)
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

	Application* app = (Application*)device->descriptor.userdata;
	app->enttEditor.renderSimpleCombo(*oval_get_registry(app->device), app->editorCurEntity);
}

void on_imgui2(entt::entity entity, oval_device_t* device, oval_render_context render_context)
{
	ImGui::Text("Hello, Window2!");
}

void on_window_close1(entt::entity entity, oval_device_t* device)
{
	Application* app = (Application*)device->descriptor.userdata;
	app->window1 = entt::null;
}

void on_window_close2(entt::entity entity, oval_device_t* device)
{
	Application* app = (Application*)device->descriptor.userdata;
	app->window2 = entt::null;
}

void on_submit(oval_device_t* device, oval_submit_context submit_context, HGEGraphics::rendergraph_t& rg)
{
	Application* app = (Application*)device->descriptor.userdata;
	auto& lastFrameRenderPacket = app->frameRenderPackets[submit_context.submitRenderPacketFrame];
	submit(*app, lastFrameRenderPacket, device, rg);
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
	auto& registry = *oval_get_registry(app.device);

	_init_resource(app, registry);
	_init_world(app, registry, app.window1);
		
	oval_runloop(app.device);
	_free_resource(app);
	oval_free_window_entity(app.device, app.window2);
	oval_free_window_entity(app.device, app.window1);
	oval_free_device(app.device);

	return 0;
}