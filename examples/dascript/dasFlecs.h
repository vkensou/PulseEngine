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
		ecs_entity_t dependsOn;
		bool immediate = false;
		ecs_term_t terms[FLECS_TERM_COUNT_MAX] = {};
	};

	struct EventDesc
	{
		uint64_t event;
		uint64_t terms[FLECS_TERM_COUNT_MAX] = {};
		ecs_entity_t entity;
	};

	struct ModuleContext
	{
		World world;
		ecs_entity_t initPipeline;
		ecs_entity_t updatePipeline;
		ecs_entity_t postUpdatePipeline;
		ecs_entity_t renderPipeline;
		ecs_entity_t imguiPipeline;
		//pulse::EventCenter* eventManager;
	};
}
MAKE_TYPE_FACTORY(SystemDesc, dasPulseECS::SystemDesc);
MAKE_TYPE_FACTORY(EventDesc, dasPulseECS::EventDesc);
MAKE_TYPE_FACTORY(ModuleContext, dasPulseECS::ModuleContext);
