#include "render_internal.h"

namespace pulse_cgpu_render_internal {

bool frame_data::init(CGPUDeviceId device, CGPUQueueId queue) {
    fence = cgpu_device_create_fence(device);
    if (!fence) {
        return false;
    }

    CGPUCommandPoolDescriptor pool_desc{};
    pool_desc.name = "pulse_cgpu_render_frame_pool";
    pool = cgpu_queue_create_command_pool(queue, &pool_desc);
    return pool != CGPU_NULLPTR;
}

void frame_data::begin_frame() {
    if (fence) {
        cgpu_wait_fences(1, &fence);
    }
    if (pool) {
        cgpu_command_pool_reset(pool);
    }

    for (CGPUCommandBufferId cmd : submitted_cmds) {
        available_cmds.push_back(cmd);
    }
    submitted_cmds.clear();
}

CGPUCommandBufferId frame_data::request_command_buffer() {
    CGPUCommandBufferId cmd = CGPU_NULLPTR;
    if (!available_cmds.empty()) {
        cmd = available_cmds.back();
        available_cmds.pop_back();
    } else if (pool) {
        CGPUCommandBufferDescriptor cmd_desc{};
        cmd_desc.is_secondary = false;
        cmd = cgpu_command_pool_create_command_buffer(pool, &cmd_desc);
    }

    if (cmd) {
        submitted_cmds.push_back(cmd);
    }
    return cmd;
}

void frame_data::destroy() {
    if (pool) {
        for (CGPUCommandBufferId cmd : available_cmds) {
            cgpu_command_pool_free_command_buffer(pool, cmd);
        }
        for (CGPUCommandBufferId cmd : submitted_cmds) {
            cgpu_command_pool_free_command_buffer(pool, cmd);
        }
        available_cmds.clear();
        submitted_cmds.clear();
        cgpu_queue_free_command_pool(pool->queue, pool);
        pool = CGPU_NULLPTR;
    }

    if (fence) {
        cgpu_device_free_fence(fence->device, fence);
        fence = CGPU_NULLPTR;
    }
}

void render_frame_context::reset() {
    frame_index = 0;
    frame = nullptr;
    active = false;
    submitted = false;
    failed = false;
    prepared_entities.clear();
    wait_semaphores.clear();
    signal_semaphores.clear();
}

namespace {

void render_begin_frame_system_run(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state) {
        return;
    }

    state->frame_context.reset();
    if (!state->renderer.device ||
        state->frames.empty()) {
        return;
    }

    ecs_world_t* world = it->world;
    install_observers(state, world);

    const uint32_t frame_index =
        static_cast<uint32_t>(state->renderer.frame_index % state->frames.size());
    frame_data& frame = state->frames[frame_index];
    frame.begin_frame();

    state->frame_context.frame_index = frame_index;
    state->frame_context.frame = &frame;
    state->frame_context.active = true;
}

void render_prepare_windows_system_run(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state || !state->frame_context.active) {
        return;
    }

    ecs_world_t* world = it->world;
    pulse_window* windows = ecs_field(it, pulse_window, 0);
    pulse_cgpu_surface* surfaces = ecs_field(it, pulse_cgpu_surface, 1);
    pulse_cgpu_swapchain* swapchains = ecs_field(it, pulse_cgpu_swapchain, 2);
    render_frame_context& frame = state->frame_context;

    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];

        pulse_cgpu_swapchain* swapchain = &swapchains[i];
        if (!ensure_cgpu_swapchain(
                state,
                world,
                entity,
                windows[i],
                &surfaces[i],
                &swapchain)) {
            continue;
        }

        if (acquire_window_image(swapchain, frame.frame_index)) {
            frame.prepared_entities.push_back(entity);
            frame.wait_semaphores.push_back(
                swapchain->image_available_semaphores[
                    frame.frame_index % swapchain->backbuffer_count
                ]
            );
            frame.signal_semaphores.push_back(
                swapchain->render_finished_semaphores[
                    swapchain->current_backbuffer_index
                ]
            );
        }
    }
}

void render_submit_system_run(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state || !state->frame_context.active) {
        return;
    }

    render_frame_context& frame_context = state->frame_context;
    if (frame_context.prepared_entities.empty()) {
        return;
    }

    if (!frame_context.frame) {
        frame_context.failed = true;
        return;
    }

    ecs_world_t* world = it->world;
    CGPUCommandBufferId cmd = frame_context.frame->request_command_buffer();
    if (!cmd) {
        frame_context.failed = true;
        return;
    }

    cgpu_command_buffer_begin(cmd);
    for (ecs_entity_t entity : frame_context.prepared_entities) {
        pulse_cgpu_swapchain* swapchain =
            ecs_get_mut(world, entity, pulse_cgpu_swapchain);
        if (swapchain) {
            encode_clear_pass(state, cmd, swapchain);
        }
    }
    cgpu_command_buffer_end(cmd);

    CGPUQueueSubmitDescriptor submit_desc{};
    submit_desc.cmd_count = 1;
    submit_desc.p_cmds = &cmd;
    submit_desc.signal_fence = frame_context.frame->fence;
    submit_desc.wait_semaphore_count =
        static_cast<uint32_t>(frame_context.wait_semaphores.size());
    submit_desc.p_wait_semaphores = frame_context.wait_semaphores.data();
    submit_desc.signal_semaphore_count =
        static_cast<uint32_t>(frame_context.signal_semaphores.size());
    submit_desc.p_signal_semaphores = frame_context.signal_semaphores.data();
    cgpu_queue_submit(state->renderer.graphics_queue, &submit_desc);
    frame_context.submitted = true;
}

void render_present_system_run(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state || !state->frame_context.active || state->frame_context.failed) {
        return;
    }

    ecs_world_t* world = it->world;
    render_frame_context& frame_context = state->frame_context;

    if (frame_context.submitted) {
        for (ecs_entity_t entity : frame_context.prepared_entities) {
            pulse_cgpu_swapchain* swapchain =
                ecs_get_mut(world, entity, pulse_cgpu_swapchain);
            if (!swapchain) {
                continue;
            }

            CGPUSemaphoreId wait =
                swapchain->render_finished_semaphores[
                    swapchain->current_backbuffer_index
                ];
            CGPUQueuePresentDescriptor present_desc{};
            present_desc.swapchain = swapchain->swapchain;
            present_desc.wait_semaphore_count = 1;
            present_desc.p_wait_semaphores = &wait;
            present_desc.index =
                static_cast<uint8_t>(swapchain->current_backbuffer_index);
            cgpu_queue_present(state->renderer.present_queue, &present_desc);
        }
    }

    state->renderer.frame_index++;
    ecs_singleton_set_ptr(world, pulse_cgpu_renderer, &state->renderer);
}

ecs_entity_t create_render_system_entity(
    ecs_world_t* world,
    const char* name,
    ecs_entity_t depends_on
) {
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = name;
    ecs_entity_t system_entity = ecs_entity_init(world, &entity_desc);
    if (depends_on) {
        ecs_add_pair(world, system_entity, EcsDependsOn, depends_on);
    }
    return system_entity;
}

ecs_entity_t install_render_run_system(
    ecs_world_t* world,
    const char* name,
    ecs_run_action_t run,
    pulse_cgpu_render_state* state,
    ecs_entity_t depends_on
) {
    ecs_system_desc_t system_desc{};
    system_desc.entity = create_render_system_entity(world, name, depends_on);
    system_desc.phase = EcsOnStore;
    system_desc.run = run;
    system_desc.ctx = state;
    system_desc.immediate = true;
    return ecs_system_init(world, &system_desc);
}

ecs_entity_t install_prepare_windows_system(
    ecs_world_t* world,
    pulse_cgpu_render_state* state,
    ecs_entity_t depends_on
) {
    ecs_system_desc_t system_desc{};
    system_desc.entity = create_render_system_entity(
        world,
        "PulseCgpuPrepareWindowsSystem",
        depends_on
    );
    system_desc.phase = EcsOnStore;
    system_desc.query.terms[0].id = ecs_id(pulse_window);
    system_desc.query.terms[1].id = ecs_id(pulse_cgpu_surface);
    system_desc.query.terms[2].id = ecs_id(pulse_cgpu_swapchain);
    system_desc.query.terms[3].id = ecs_id(PulseWindowCloseRequested);
    system_desc.query.terms[3].oper = EcsNot;
    system_desc.query.cache_kind = EcsQueryCacheAuto;
    system_desc.callback = render_prepare_windows_system_run;
    system_desc.ctx = state;
    system_desc.immediate = true;
    return ecs_system_init(world, &system_desc);
}

} // namespace

void install_render_systems(pulse_cgpu_render_state* state, ecs_world_t* world) {
    ecs_entity_t begin = install_render_run_system(
        world,
        "PulseCgpuBeginFrameSystem",
        render_begin_frame_system_run,
        state,
        0
    );
    ecs_entity_t prepare = install_prepare_windows_system(world, state, begin);
    ecs_entity_t submit = install_render_run_system(
        world,
        "PulseCgpuSubmitSystem",
        render_submit_system_run,
        state,
        prepare
    );
    install_render_run_system(
        world,
        "PulseCgpuPresentSystem",
        render_present_system_run,
        state,
        submit
    );
}

} // namespace pulse_cgpu_render_internal
