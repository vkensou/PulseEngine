#pragma once

#include "pulse_cgpu_render.h"
#include "pulse_window.h"

#include <vector>

namespace pulse_cgpu_render_internal {

struct frame_data {
    CGPUFenceId fence = CGPU_NULLPTR;
    CGPUCommandPoolId pool = CGPU_NULLPTR;
    std::vector<CGPUCommandBufferId> available_cmds;
    std::vector<CGPUCommandBufferId> submitted_cmds;

    bool init(CGPUDeviceId device, CGPUQueueId queue);
    void begin_frame();
    CGPUCommandBufferId request_command_buffer();
    void destroy();
};

struct render_frame_context {
    uint32_t frame_index = 0;
    frame_data* frame = nullptr;
    bool active = false;
    bool submitted = false;
    bool failed = false;
    std::vector<ecs_entity_t> prepared_entities;
    std::vector<CGPUSemaphoreId> wait_semaphores;
    std::vector<CGPUSemaphoreId> signal_semaphores;

    void reset();
};

struct pulse_cgpu_render_state {
    pulse_app_t app = nullptr;
    pulse_cgpu_render_plugin_desc desc{};
    pulse_cgpu_renderer renderer{};
    std::vector<frame_data> frames;
    render_frame_context frame_context;
    ecs_entity_t sdl_window_on_set_observer = 0;
    ecs_entity_t sdl_window_on_remove_observer = 0;
    ecs_entity_t window_on_set_observer = 0;
    bool existing_sdl_windows_bootstrapped = false;
};

typedef struct pulse_cgpu_render_state_resource {
    pulse_cgpu_render_state* state;
} pulse_cgpu_render_state_resource;

extern ECS_COMPONENT_DECLARE(pulse_cgpu_render_state_resource);

pulse_cgpu_render_state* state_from_world(ecs_world_t* world);

void reset_surface_handles(pulse_cgpu_surface* surface);
void release_surface_resources(pulse_cgpu_surface* surface);
void reset_swapchain_handles(pulse_cgpu_swapchain* swapchain);
void release_swapchain_resources(pulse_cgpu_swapchain* swapchain);

void register_components(ecs_world_t* world);
void ensure_component_relations(ecs_world_t* world);

void install_observers(pulse_cgpu_render_state* state, ecs_world_t* world);

bool ensure_cgpu_surface(
    pulse_cgpu_render_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const pulse_sdl_window& sdl_window,
    pulse_cgpu_surface** out_surface
);
bool ensure_cgpu_swapchain(
    pulse_cgpu_render_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const pulse_window& window,
    const pulse_cgpu_surface* surface,
    pulse_cgpu_swapchain** out_swapchain
);
bool acquire_window_image(pulse_cgpu_swapchain* swapchain, uint32_t frame_index);
void encode_clear_pass(
    const pulse_cgpu_render_state* state,
    CGPUCommandBufferId cmd,
    pulse_cgpu_swapchain* swapchain
);

void install_render_systems(pulse_cgpu_render_state* state, ecs_world_t* world);

} // namespace pulse_cgpu_render_internal
