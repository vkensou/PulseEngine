#pragma once

#include <flecs.h>

struct World
{
	int a;
};

struct Entity
{
	int b;
	Entity() : b(0) {}
	Entity(int b) : b(b) {}
	operator int () const { return b; }
};

Entity create_entity(World& world)
{
	auto id_ = world.a++;
	Entity entity = { id_ };
	return entity;
}

void dump_world(const World& world)
{
	printf("%d\n", world.a);
}

void dump_entity(Entity entity)
{
	printf("%d\n", entity.b);
}
