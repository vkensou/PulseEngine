#pragma once

#include "pulse_cgpu_render.h"
#include "pulse_window.h"

#include "rendergraph_cpp.h"
#include "rendergraph_compiler.h"
#include "rendergraph_executor.h"
#include "renderer.h"

#include <memory>
#include <memory_resource>
#include <vector>

namespace pulse_cgpu_render_internal {

struct frame_data {
    CGPUFenceId fence = CGPU_NULLPTR;
    CGPUCommandPoolId pool = CGPU_NULLPTR;
    std::vector<CGPUCommandBufferId> available_cmds;
    std::vector<CGPUCommandBufferId> submitted_cmds;
    std::unique_ptr<std::pmr::unsynchronized_pool_resource> exec_memory;
    std::unique_ptr<HGEGraphics::ExecutorContext> exec_context;

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
    std::unique_ptr<std::pmr::unsynchronized_pool_resource> graph_memory;
    std::unique_ptr<HGEGraphics::rendergraph_t> graph;
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
    ecs_entity_t begin_frame_system = 0;
    ecs_entity_t reset_backbuffer_system = 0;
    ecs_entity_t prepare_windows_system = 0;
    ecs_entity_t build_graph_system = 0;
    ecs_entity_t execute_graph_system = 0;
    ecs_entity_t submit_system = 0;
    ecs_entity_t present_system = 0;
    bool existing_sdl_windows_bootstrapped = false;
};

typedef struct pulse_cgpu_render_state_resource {
    pulse_cgpu_render_state* state;
} pulse_cgpu_render_state_resource;

extern ECS_COMPONENT_DECLARE(pulse_cgpu_render_state_resource);

extern ecs_entity_t pulse_cgpu_render_begin_frame_phase;
extern ecs_entity_t pulse_cgpu_render_reset_backbuffer_phase;
extern ecs_entity_t pulse_cgpu_render_prepare_windows_phase;
extern ecs_entity_t pulse_cgpu_render_record_graph_phase;
extern ecs_entity_t pulse_cgpu_render_execute_graph_phase;
extern ecs_entity_t pulse_cgpu_render_submit_phase;
extern ecs_entity_t pulse_cgpu_render_present_phase;

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

void install_render_systems(pulse_cgpu_render_state* state, ecs_world_t* world);
void uninstall_render_systems(pulse_cgpu_render_state* state, ecs_world_t* world);

} // namespace pulse_cgpu_render_internal
