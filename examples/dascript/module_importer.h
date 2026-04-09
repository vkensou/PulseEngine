#pragma once

#include "flecs.h"

namespace pulse
{
	struct EventCenter;

	struct ModuleContext
	{
		flecs::world world;
		flecs::entity_t initPipeline;
		flecs::entity_t updatePipeline;
		flecs::entity_t postUpdatePipeline;
		flecs::entity_t renderPipeline;
		flecs::entity_t imguiPipeline;
		pulse::EventCenter* eventManager;
	};
}