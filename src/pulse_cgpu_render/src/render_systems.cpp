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
    if (!pool) {
        return false;
    }

    exec_memory = std::make_unique<std::pmr::unsynchronized_pool_resource>();
    exec_context =
        std::make_unique<HGEGraphics::ExecutorContext>(
            device,
            queue,
            false,
            exec_memory.get()
        );
    return exec_context != nullptr;
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

    if (exec_context) {
        exec_context->newFrame();
    }
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
    if (exec_context) {
        exec_context->destroy();
        exec_context.reset();
    }
    exec_memory.reset();

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
    graph.reset();
    graph_memory.reset();
    prepared_entities.clear();
    wait_semaphores.clear();
    signal_semaphores.clear();
}

namespace {

constexpr size_t kGraphResourceEstimate = 32;
constexpr size_t kGraphPassEstimate = 32;
constexpr size_t kGraphEdgeEstimate = 64;

bool ensure_frame_graph(render_frame_context& frame_context) {
    if (!frame_context.graph_memory) {
        frame_context.graph_memory =
            std::make_unique<std::pmr::unsynchronized_pool_resource>();
    }
    if (!frame_context.graph) {
        const size_t window_count = frame_context.prepared_entities.size();
        const size_t resource_estimate =
            window_count * 4 > kGraphResourceEstimate
                ? window_count * 4
                : kGraphResourceEstimate;
        const size_t pass_estimate =
            window_count * 4 > kGraphPassEstimate
                ? window_count * 4
                : kGraphPassEstimate;
        const size_t edge_estimate =
            window_count * 8 > kGraphEdgeEstimate
                ? window_count * 8
                : kGraphEdgeEstimate;
        frame_context.graph = std::make_unique<HGEGraphics::rendergraph_t>(
            resource_estimate,
            pass_estimate,
            edge_estimate,
            nullptr,
            CGPU_NULLPTR,
            frame_context.graph_memory.get()
        );
    }
    return frame_context.graph != nullptr;
}

void delete_registered_entity(ecs_world_t* world, ecs_entity_t& entity) {
    if (world && entity && ecs_is_alive(world, entity)) {
        ecs_delete(world, entity);
    }
    entity = 0;
}

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
    if (!state->desc.record_callback) {
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

void render_begin_graph_system_run(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state || !state->frame_context.active) {
        return;
    }

    render_frame_context& frame_context = state->frame_context;
    if (frame_context.prepared_entities.empty() || !frame_context.frame) {
        return;
    }
    if (!state->desc.record_callback) {
        return;
    }
    if (!ensure_frame_graph(frame_context)) {
        frame_context.failed = true;
        return;
    }

    state->desc.record_callback(
        state->app,
        *frame_context.graph.get(),
        state->desc.record_user_data
    );
}

void render_execute_graph_system_run(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state || !state->frame_context.active || state->frame_context.failed) {
        return;
    }

    render_frame_context& frame_context = state->frame_context;
    if (!frame_context.graph || !frame_context.graph_memory) {
        return;
    }
    if (!frame_context.frame || !frame_context.frame->exec_context) {
        frame_context.failed = true;
        return;
    }

    HGEGraphics::CompiledRenderGraph compiled =
        HGEGraphics::Compiler::Compile(
            *frame_context.graph,
            frame_context.graph_memory.get()
        );
    HGEGraphics::Executor::Execute(compiled, *frame_context.frame->exec_context);

    auto* graph_impl = HGEGraphics::to_impl(*frame_context.graph);
    for (HGEGraphics::Texture* imported : graph_impl->imported_textures) {
        if (imported) {
            imported->dynamic_handle = {};
        }
    }
    for (HGEGraphics::Buffer* imported : graph_impl->imported_buffers) {
        if (imported) {
            imported->dynamic_handle = {};
        }
    }
}

void render_submit_system_run(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state || !state->frame_context.active || state->frame_context.failed) {
        return;
    }

    render_frame_context& frame_context = state->frame_context;
    if (frame_context.prepared_entities.empty()) {
        return;
    }

    if (!frame_context.frame || !frame_context.frame->exec_context) {
        frame_context.failed = true;
        return;
    }

    std::pmr::vector<CGPUCommandBufferId>& cmds =
        frame_context.frame->exec_context->allocated_cmds;
    if (cmds.empty()) {
        return;
    }

    CGPUQueueSubmitDescriptor submit_desc{};
    submit_desc.cmd_count = static_cast<uint32_t>(cmds.size());
    submit_desc.p_cmds = cmds.data();
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
    ecs_entity_t phase
) {
    ecs_system_desc_t system_desc{};
    system_desc.entity = create_render_system_entity(world, name, 0);
    system_desc.phase = phase;
    system_desc.run = run;
    system_desc.ctx = state;
    system_desc.immediate = true;
    return ecs_system_init(world, &system_desc);
}

ecs_entity_t install_prepare_windows_system(
    ecs_world_t* world,
    pulse_cgpu_render_state* state,
    ecs_entity_t phase
) {
    ecs_system_desc_t system_desc{};
    system_desc.entity = create_render_system_entity(
        world,
        "PulseCgpuPrepareWindowsSystem",
        0
    );
    system_desc.phase = phase;
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
    if (!state || !world || state->begin_frame_system) {
        return;
    }

    state->begin_frame_system = install_render_run_system(
        world,
        "PulseCgpuBeginFrameSystem",
        render_begin_frame_system_run,
        state,
        pulse_cgpu_render_begin_frame_phase
    );
    state->prepare_windows_system =
        install_prepare_windows_system(
            world,
            state,
            pulse_cgpu_render_prepare_windows_phase
        );
    state->build_graph_system = install_render_run_system(
        world,
        "PulseCgpuRecordGraphSystem",
        render_begin_graph_system_run,
        state,
        pulse_cgpu_render_record_graph_phase
    );
    state->execute_graph_system = install_render_run_system(
        world,
        "PulseCgpuExecuteGraphSystem",
        render_execute_graph_system_run,
        state,
        pulse_cgpu_render_execute_graph_phase
    );
    state->submit_system = install_render_run_system(
        world,
        "PulseCgpuSubmitSystem",
        render_submit_system_run,
        state,
        pulse_cgpu_render_submit_phase
    );
    state->present_system = install_render_run_system(
        world,
        "PulseCgpuPresentSystem",
        render_present_system_run,
        state,
        pulse_cgpu_render_present_phase
    );
}

void uninstall_render_systems(pulse_cgpu_render_state* state, ecs_world_t* world) {
    if (!state) {
        return;
    }

    delete_registered_entity(world, state->present_system);
    delete_registered_entity(world, state->submit_system);
    delete_registered_entity(world, state->execute_graph_system);
    delete_registered_entity(world, state->build_graph_system);
    delete_registered_entity(world, state->prepare_windows_system);
    delete_registered_entity(world, state->begin_frame_system);
}

} // namespace pulse_cgpu_render_internal

pulse_texture_handle_t pulse_cgpu_render_import_window_backbuffer(
    pulse_app_t app,
    pulse_rendergraph_t* graph,
    ecs_entity_t window_entity
) {
    const pulse_cgpu_swapchain* swapchain =
        pulse_cgpu_swapchain_get(app, window_entity);
    if (!swapchain || !swapchain->backbuffers) {
        return pulse_texture_handle_t{};
    }
    void* backbuffer =
        &static_cast<HGEGraphics::Backbuffer*>(swapchain->backbuffers)[
            swapchain->current_backbuffer_index];
    return pulse_rendergraph_import_backbuffer(graph, backbuffer);
}
