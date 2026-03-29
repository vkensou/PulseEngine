#pragma once

#include "framework.h"
#include "handmademath.h"
#include "ecsext.hpp"
#include <vector>
#include <memory>

struct ResourceManager
{
	oval_device_t* device;
	std::vector<HGEGraphics::Mesh*> meshes;
	std::vector<HGEGraphics::Material*> materials;
};

struct KeyboardState
{
	std::vector<uint8_t> lastKeys;
	std::vector<uint8_t> currentKeys;
};

struct Position
{
	HMM_Vec3 value;
};

struct Rotation
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

struct SystemContext
{
	KeyboardState keyboardState;
	oval_update_context updateContext;
	oval_render_context renderContext;
	ResourceManager resourceManager;
};
