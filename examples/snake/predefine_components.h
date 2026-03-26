#pragma once

#include "handmademath.h"
#include "flecs.h"

struct SystemContext
{
	float delta_time;
	float time_since_startup;
	double delta_time_double;
	double time_since_startup_double;
	float interpolation_time;
	double interpolation_time_double;
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
