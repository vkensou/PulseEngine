#pragma once

#include <flecs.h>
#include "daScript/daScript.h"
#include "module_importer.h"
#include "predefine_components.h"

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

Entity create_entity(World& world);

void dump_world(const World& world);

void dump_entity(Entity entity);

class ModuleFlecs : public das::Module
{
public:
	ModuleFlecs();
};
