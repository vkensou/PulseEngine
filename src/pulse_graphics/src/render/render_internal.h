#pragma once

#include "../graphics_internal.h"
#include "pulse_graphics.h"
#include "pulse_window.h"

#include "rendergraph_cpp.h"
#include "rendergraph_compiler.h"
#include "rendergraph_executor.h"
#include "renderer.h"

#include <memory>
#include <memory_resource>
#include <vector>

namespace pulse_graphics_internal {

extern ecs_entity_t pulse_graphics_render_begin_frame_phase;
extern ecs_entity_t pulse_graphics_render_reset_backbuffer_phase;
extern ecs_entity_t pulse_graphics_render_prepare_windows_phase;
extern ecs_entity_t pulse_graphics_render_record_graph_phase;
extern ecs_entity_t pulse_graphics_render_execute_graph_phase;
extern ecs_entity_t pulse_graphics_render_submit_phase;
extern ecs_entity_t pulse_graphics_render_present_phase;

pulse_graphics_state* state_from_world(ecs_world_t* world);

void reset_surface_handles(pulse_graphics_surface* surface);
void release_surface_resources(pulse_graphics_surface* surface);
void reset_swapchain_handles(pulse_graphics_swapchain* swapchain);
void release_swapchain_resources(pulse_graphics_swapchain* swapchain);

void register_components(ecs_world_t* world);
void ensure_component_relations(ecs_world_t* world);

void install_observers(pulse_graphics_state* state, ecs_world_t* world);

bool ensure_cgpu_surface(
    pulse_graphics_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const pulse_sdl_window& sdl_window,
    pulse_graphics_surface** out_surface
);
bool ensure_cgpu_swapchain(
    pulse_graphics_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const pulse_window& window,
    const pulse_graphics_surface* surface,
    pulse_graphics_swapchain** out_swapchain
);
bool acquire_window_image(pulse_graphics_swapchain* swapchain, uint32_t frame_index);

void install_render_systems(pulse_graphics_state* state, ecs_world_t* world);
void uninstall_render_systems(pulse_graphics_state* state, ecs_world_t* world);

} // namespace pulse_graphics_internal
