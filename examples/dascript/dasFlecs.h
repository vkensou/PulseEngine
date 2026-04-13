#pragma once

#include <flecs.h>
#include "daScript/daScript.h"

namespace dasPulseECS
{
	struct World
	{
		ecs_world_t* world_;
		World() : world_(nullptr) {}
		World(ecs_world_t* w) : world_(w) {}

		operator ecs_world_t*() const { return world_; }
	};
	struct Entity
	{
		ecs_entity_t entity_;
		Entity() : entity_(0) {}
		Entity(ecs_entity_t b) : entity_(b) {}

		operator uint64_t() const { return entity_; }
	};

	struct ModuleContext
	{
		World world;
		Entity initPipeline;
		Entity updatePipeline;
		Entity postUpdatePipeline;
		Entity renderPipeline;
		Entity imguiPipeline;
		//pulse::EventCenter* eventManager;
	};
}
MAKE_TYPE_FACTORY(ModuleContext, dasPulseECS::ModuleContext);
