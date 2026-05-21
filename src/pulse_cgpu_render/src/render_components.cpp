#include "render_internal.h"

ECS_COMPONENT_DECLARE(pulse_cgpu_renderer);
ECS_COMPONENT_DECLARE(pulse_cgpu_surface);
ECS_COMPONENT_DECLARE(pulse_cgpu_swapchain);

namespace pulse_cgpu_render_internal {

ECS_COMPONENT_DECLARE(pulse_cgpu_render_state_resource);

ecs_entity_t pulse_cgpu_render_begin_frame_phase = 0;
ecs_entity_t pulse_cgpu_render_prepare_windows_phase = 0;
ecs_entity_t pulse_cgpu_render_record_graph_phase = 0;
ecs_entity_t pulse_cgpu_render_execute_graph_phase = 0;
ecs_entity_t pulse_cgpu_render_submit_phase = 0;
ecs_entity_t pulse_cgpu_render_present_phase = 0;

void reset_surface_handles(pulse_cgpu_surface* surface) {
    surface->instance = CGPU_NULLPTR;
    surface->surface = CGPU_NULLPTR;
}

void release_surface_resources(pulse_cgpu_surface* surface) {
    if (!surface) {
        return;
    }

    if (surface->instance && surface->surface) {
        cgpu_instance_free_surface(surface->instance, surface->surface);
    }

    reset_surface_handles(surface);
}

namespace {

ecs_entity_t create_phase(
    ecs_world_t* world,
    const char* name,
    ecs_entity_t depends_on
) {
    ecs_entity_desc_t desc{};
    desc.name = name;
    ecs_entity_t phase = ecs_entity_init(world, &desc);
    ecs_add_id(world, phase, EcsPhase);
    if (depends_on) {
        ecs_add_pair(world, phase, EcsDependsOn, depends_on);
    }
    return phase;
}

ECS_CTOR(pulse_cgpu_surface, ptr, {
    reset_surface_handles(ptr);
})

ECS_DTOR(pulse_cgpu_surface, ptr, {
    release_surface_resources(ptr);
})

ECS_MOVE(pulse_cgpu_surface, dst, src, {
    release_surface_resources(dst);
    *dst = *src;
    reset_surface_handles(src);
})

void on_surface_set(ecs_iter_t* it)
{
    pulse_cgpu_surface* surfaces = ecs_field(it, pulse_cgpu_surface, 0);
    for (int32_t i = 0; i < it->count; ++i) {
        if (!surfaces[i].surface) {
            continue;
        }

        ecs_entity_t entity = it->entities[i];
        if (!ecs_has_id(it->world, entity, ecs_id(pulse_cgpu_swapchain))) {
            ecs_add_id(it->world, entity, ecs_id(pulse_cgpu_swapchain));
        }
    }
}

void on_surface_remove(ecs_iter_t* it)
{
    pulse_cgpu_render_state* state = state_from_world(it->world);
    if (state && state->renderer.graphics_queue) {
        cgpu_queue_wait_idle(state->renderer.graphics_queue);
    }

    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_has_id(it->world, entity, ecs_id(pulse_cgpu_swapchain))) {
            ecs_remove_id(it->world, entity, ecs_id(pulse_cgpu_swapchain));
        }
    }
}

ECS_CTOR(pulse_cgpu_swapchain, ptr, {
    reset_swapchain_handles(ptr);
})

ECS_DTOR(pulse_cgpu_swapchain, ptr, {
    release_swapchain_resources(ptr);
})

ECS_MOVE(pulse_cgpu_swapchain, dst, src, {
    release_swapchain_resources(dst);
    *dst = *src;
    reset_swapchain_handles(src);
})

} // namespace

void reset_swapchain_handles(pulse_cgpu_swapchain* swapchain) {
    swapchain->device = CGPU_NULLPTR;
    swapchain->swapchain = CGPU_NULLPTR;
    swapchain->backbuffer_views = nullptr;
    swapchain->framebuffers = nullptr;
    swapchain->image_available_semaphores = nullptr;
    swapchain->render_finished_semaphores = nullptr;
    swapchain->backbuffer_count = 0;
    swapchain->width = 0;
    swapchain->height = 0;
    swapchain->current_backbuffer_index = 0;
}

void release_swapchain_resources(pulse_cgpu_swapchain* swapchain) {
    if (!swapchain) {
        return;
    }

    CGPUDeviceId device = swapchain->device;
    if (device) {
        for (uint32_t i = 0; i < swapchain->backbuffer_count; ++i) {
            if (swapchain->framebuffers && swapchain->framebuffers[i]) {
                cgpu_device_free_framebuffer(device, swapchain->framebuffers[i]);
            }
            if (swapchain->backbuffer_views && swapchain->backbuffer_views[i]) {
                cgpu_device_free_texture_view(device, swapchain->backbuffer_views[i]);
            }
            if (swapchain->image_available_semaphores &&
                swapchain->image_available_semaphores[i]) {
                cgpu_device_free_semaphore(
                    device,
                    swapchain->image_available_semaphores[i]
                );
            }
            if (swapchain->render_finished_semaphores &&
                swapchain->render_finished_semaphores[i]) {
                cgpu_device_free_semaphore(
                    device,
                    swapchain->render_finished_semaphores[i]
                );
            }
        }

        if (swapchain->swapchain) {
            cgpu_device_free_swap_chain(device, swapchain->swapchain);
        }
    }

    delete[] swapchain->backbuffer_views;
    delete[] swapchain->framebuffers;
    delete[] swapchain->image_available_semaphores;
    delete[] swapchain->render_finished_semaphores;

    reset_swapchain_handles(swapchain);
}

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, pulse_cgpu_renderer);
    ECS_COMPONENT_DEFINE(world, pulse_cgpu_surface);
    ECS_COMPONENT_DEFINE(world, pulse_cgpu_swapchain);
    ECS_COMPONENT_DEFINE(world, pulse_cgpu_render_state_resource);

    pulse_cgpu_render_begin_frame_phase =
        create_phase(world, "PulseCgpuRenderBeginFrame", EcsOnStore);
    pulse_cgpu_render_prepare_windows_phase = create_phase(
        world,
        "PulseCgpuRenderPrepareWindows",
        pulse_cgpu_render_begin_frame_phase
    );
    pulse_cgpu_render_record_graph_phase = create_phase(
        world,
        "PulseCgpuRenderRecordGraph",
        pulse_cgpu_render_prepare_windows_phase
    );
    pulse_cgpu_render_execute_graph_phase = create_phase(
        world,
        "PulseCgpuRenderExecuteGraph",
        pulse_cgpu_render_record_graph_phase
    );
    pulse_cgpu_render_submit_phase = create_phase(
        world,
        "PulseCgpuRenderSubmit",
        pulse_cgpu_render_execute_graph_phase
    );
    pulse_cgpu_render_present_phase = create_phase(
        world,
        "PulseCgpuRenderPresent",
        pulse_cgpu_render_submit_phase
    );

    ecs_type_hooks_t surface_hooks = {
        .ctor = ecs_ctor(pulse_cgpu_surface),
        .dtor = ecs_dtor(pulse_cgpu_surface),
        .move = ecs_move(pulse_cgpu_surface),
        .on_set = on_surface_set,
        .on_remove = on_surface_remove,
    };
    ecs_set_hooks_id(world, ecs_id(pulse_cgpu_surface), &surface_hooks);

    ecs_type_hooks_t swapchain_hooks = {
        .ctor = ecs_ctor(pulse_cgpu_swapchain),
        .dtor = ecs_dtor(pulse_cgpu_swapchain),
        .move = ecs_move(pulse_cgpu_swapchain),
    };
    ecs_set_hooks_id(world, ecs_id(pulse_cgpu_swapchain), &swapchain_hooks);

    if (ecs_id(pulse_sdl_window) != 0) {
        ecs_add_pair(world, ecs_id(pulse_cgpu_surface), EcsWith, ecs_id(pulse_sdl_window));
    }
    ecs_add_pair(world, ecs_id(pulse_cgpu_swapchain), EcsWith, ecs_id(pulse_cgpu_surface));
}

void ensure_component_relations(ecs_world_t* world) {
    if (ecs_id(pulse_cgpu_surface) != 0 && ecs_id(pulse_sdl_window) != 0) {
        ecs_add_pair(world, ecs_id(pulse_cgpu_surface), EcsWith, ecs_id(pulse_sdl_window));
    }
    if (ecs_id(pulse_cgpu_swapchain) != 0 && ecs_id(pulse_cgpu_surface) != 0) {
        ecs_add_pair(world, ecs_id(pulse_cgpu_swapchain), EcsWith, ecs_id(pulse_cgpu_surface));
    }
}

} // namespace pulse_cgpu_render_internal
