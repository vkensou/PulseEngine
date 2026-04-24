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
	struct Query
	{
		ecs_query_t* query_;
		Query() : query_(nullptr) {}
		Query(ecs_query_t* q) : query_(q) {}

		operator ecs_query_t* () const { return query_; }
	};

	struct SystemDesc
	{
		const char* name = nullptr;
		Entity dependsOn;
		bool immediate = false;
		ecs_term_t terms[FLECS_TERM_COUNT_MAX] = {};
	};

	struct EventDesc
	{
		uint64_t event;
		uint64_t terms[FLECS_TERM_COUNT_MAX] = {};
		Entity entity;
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
MAKE_TYPE_FACTORY(SystemDesc, dasPulseECS::SystemDesc);
MAKE_TYPE_FACTORY(EventDesc, dasPulseECS::EventDesc);
MAKE_TYPE_FACTORY(ModuleContext, dasPulseECS::ModuleContext);
