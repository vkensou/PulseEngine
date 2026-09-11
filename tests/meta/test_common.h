#pragma once

#undef NDEBUG
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "flecs.h"
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
		comp.member("hp", &TestPrim::hp);
		comp.member("shield", &TestPrim::shield);
		comp.member("alive", &TestPrim::alive);
		comp.member("sign", &TestPrim::sign);
		comp.member("ratio", &TestPrim::ratio);
		comp.member("precise", &TestPrim::precise);
	}
	{
		auto comp = flecs::component<TestMixed>(world, "TestMixed");
		comp.member("dir", &TestMixed::dir);
		comp.member(ecs_id(ecs_f32_t), "pos", 3, offsetof(TestMixed, pos));
		comp.member(ecs_id(ecs_f32_t), "rot", 4, offsetof(TestMixed, rot));
		comp.member(ecs_id(ecs_entity_t), "target", 0, offsetof(TestMixed, target));
		comp.member("samples", &TestMixed::samples);
	}
	flecs::component<TestOpaque>(world, "TestOpaque");
	flecs::component<TestTag>(world, "TestTag");
}
