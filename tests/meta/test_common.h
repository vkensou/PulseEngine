#pragma once

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "flecs.h"
#include "ecs_meta.hpp"
#include "pulse_math.h"

#include <string>
#include <vector>

enum class TestDirection
{
	Right,
	Up,
	Left,
	Down,
};

enum TestUnscoped
{
	UnA,
	UnB,
};

struct TestPrim
{
	int32_t hp;
	uint32_t shield;
	bool alive;
	char sign;
	float ratio;
	double precise;
};

struct TestMixed
{
	TestDirection dir;
	HMM_Vec3 pos;
	HMM_Quat rot;
	flecs::entity target;
	int32_t samples[4];
};

struct TestOpaque
{
	std::string name;
	TestUnscoped mode;
	std::vector<int32_t> bag;
};

struct TestTag
{
};

inline void register_test_meta(flecs::world& world)
{
	flecs::component<TestDirection>(world, "TestDirection");
	{
		auto comp = flecs::component<TestPrim>(world, "TestPrim");
		pulse::meta_member(comp, "hp", &TestPrim::hp);
		pulse::meta_member(comp, "shield", &TestPrim::shield);
		pulse::meta_member(comp, "alive", &TestPrim::alive);
		pulse::meta_member(comp, "sign", &TestPrim::sign);
		pulse::meta_member(comp, "ratio", &TestPrim::ratio);
		pulse::meta_member(comp, "precise", &TestPrim::precise);
	}
	{
		auto comp = flecs::component<TestMixed>(world, "TestMixed");
		pulse::meta_member(comp, "dir", &TestMixed::dir);
		pulse::meta_member(comp, "pos", &TestMixed::pos);
		pulse::meta_member(comp, "rot", &TestMixed::rot);
		pulse::meta_member(comp, "target", &TestMixed::target);
		pulse::meta_member(comp, "samples", &TestMixed::samples);
	}
	{
		auto comp = flecs::component<TestOpaque>(world, "TestOpaque");
		pulse::meta_member(comp, "name", &TestOpaque::name);
		pulse::meta_member(comp, "mode", &TestOpaque::mode);
		pulse::meta_member(comp, "bag", &TestOpaque::bag);
	}
	flecs::component<TestTag>(world, "TestTag");
}
