#include "internal.h"

ECS_COMPONENT_DECLARE(PulseGraphicsRenderer);
ECS_COMPONENT_DECLARE(PulseGraphicsSurface);
ECS_COMPONENT_DECLARE(PulseGraphicsSwapchain);

namespace pulse_graphics_internal {

ecs_entity_t pulse_graphics_render_begin_frame_phase = 0;
ecs_entity_t pulse_graphics_render_reset_backbuffer_phase = 0;
ecs_entity_t pulse_graphics_render_prepare_windows_phase = 0;
ecs_entity_t pulse_graphics_render_record_graph_phase = 0;
ecs_entity_t pulse_graphics_render_execute_graph_phase = 0;
ecs_entity_t pulse_graphics_render_submit_phase = 0;
ecs_entity_t pulse_graphics_render_present_phase = 0;

void reset_surface_handles(PulseGraphicsSurface* surface) {
    surface->instance = CGPU_NULLPTR;
    surface->surface = CGPU_NULLPTR;
}

void release_surface_resources(PulseGraphicsSurface* surface) {
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

ECS_CTOR(PulseGraphicsSurface, ptr, {
    reset_surface_handles(ptr);
})

ECS_DTOR(PulseGraphicsSurface, ptr, {
    release_surface_resources(ptr);
})

ECS_MOVE(PulseGraphicsSurface, dst, src, {
    release_surface_resources(dst);
    *dst = *src;
    reset_surface_handles(src);
})

void on_surface_set(ecs_iter_t* it)
{
    PulseGraphicsSurface* surfaces = ecs_field(it, PulseGraphicsSurface, 0);
    for (int32_t i = 0; i < it->count; ++i) {
        if (!surfaces[i].surface) {
            continue;
        }

        ecs_entity_t entity = it->entities[i];
        if (!ecs_has_id(it->world, entity, ecs_id(PulseGraphicsSwapchain))) {
            ecs_add_id(it->world, entity, ecs_id(PulseGraphicsSwapchain));
        }
    }
}

void on_surface_remove(ecs_iter_t* it)
{
    pulse_graphics_state* state = state_from_world(it->world);
    if (state && state->renderer.graphics_queue) {
        cgpu_queue_wait_idle(state->renderer.graphics_queue);
    }

    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_has_id(it->world, entity, ecs_id(PulseGraphicsSwapchain))) {
            ecs_remove_id(it->world, entity, ecs_id(PulseGraphicsSwapchain));
        }
    }
}

ECS_CTOR(PulseGraphicsSwapchain, ptr, {
    reset_swapchain_handles(ptr);
})

ECS_DTOR(PulseGraphicsSwapchain, ptr, {
    release_swapchain_resources(ptr);
})

ECS_MOVE(PulseGraphicsSwapchain, dst, src, {
    release_swapchain_resources(dst);
    *dst = *src;
    reset_swapchain_handles(src);
})

} // namespace

void reset_swapchain_handles(PulseGraphicsSwapchain* swapchain) {
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
    swapchain->backbuffers = nullptr;
    swapchain->current_backbuffer = nullptr;
}

void release_swapchain_resources(PulseGraphicsSwapchain* swapchain) {
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
    delete[] static_cast<pulse_backbuffer_data_t*>(swapchain->backbuffers);
    swapchain->backbuffers = nullptr;

    reset_swapchain_handles(swapchain);
}

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, PulseGraphicsRenderer);
    ECS_COMPONENT_DEFINE(world, PulseGraphicsSurface);
    ECS_COMPONENT_DEFINE(world, PulseGraphicsSwapchain);
    ECS_COMPONENT_DEFINE(world, pulse_graphics_state_resource);

    pulse_graphics_render_begin_frame_phase =
        create_phase(world, "PulseCgpuRenderBeginFrame", EcsOnStore);
    pulse_graphics_render_reset_backbuffer_phase = create_phase(
        world,
        "PulseCgpuRenderResetBackbuffer",
        pulse_graphics_render_begin_frame_phase
    );
    pulse_graphics_render_prepare_windows_phase = create_phase(
        world,
        "PulseCgpuRenderPrepareWindows",
        pulse_graphics_render_reset_backbuffer_phase
    );
    pulse_graphics_render_record_graph_phase = create_phase(
        world,
        "PulseCgpuRenderRecordGraph",
        pulse_graphics_render_prepare_windows_phase
    );
    pulse_graphics_render_execute_graph_phase = create_phase(
        world,
        "PulseCgpuRenderExecuteGraph",
        pulse_graphics_render_record_graph_phase
    );
    pulse_graphics_render_submit_phase = create_phase(
        world,
        "PulseCgpuRenderSubmit",
        pulse_graphics_render_execute_graph_phase
    );
    pulse_graphics_render_present_phase = create_phase(
        world,
        "PulseCgpuRenderPresent",
        pulse_graphics_render_submit_phase
    );

    ecs_type_hooks_t surface_hooks = {
        .ctor = ecs_ctor(PulseGraphicsSurface),
        .dtor = ecs_dtor(PulseGraphicsSurface),
        .move = ecs_move(PulseGraphicsSurface),
        .on_set = on_surface_set,
        .on_remove = on_surface_remove,
    };
    ecs_set_hooks_id(world, ecs_id(PulseGraphicsSurface), &surface_hooks);

    ecs_type_hooks_t swapchain_hooks = {
        .ctor = ecs_ctor(PulseGraphicsSwapchain),
        .dtor = ecs_dtor(PulseGraphicsSwapchain),
        .move = ecs_move(PulseGraphicsSwapchain),
    };
    ecs_set_hooks_id(world, ecs_id(PulseGraphicsSwapchain), &swapchain_hooks);

    if (ecs_id(PulseSdlWindow) != 0) {
        ecs_add_pair(world, ecs_id(PulseGraphicsSurface), EcsWith, ecs_id(PulseSdlWindow));
    }
    ecs_add_pair(world, ecs_id(PulseGraphicsSwapchain), EcsWith, ecs_id(PulseGraphicsSurface));
}

void ensure_component_relations(ecs_world_t* world) {
    if (ecs_id(PulseGraphicsSurface) != 0 && ecs_id(PulseSdlWindow) != 0) {
        ecs_add_pair(world, ecs_id(PulseGraphicsSurface), EcsWith, ecs_id(PulseSdlWindow));
    }
    if (ecs_id(PulseGraphicsSwapchain) != 0 && ecs_id(PulseGraphicsSurface) != 0) {
        ecs_add_pair(world, ecs_id(PulseGraphicsSwapchain), EcsWith, ecs_id(PulseGraphicsSurface));
    }
}

namespace {

void delete_entity_if_alive(ecs_world_t* world, ecs_entity_t entity) {
    if (world && entity && ecs_is_alive(world, entity)) {
        ecs_delete(world, entity);
    }
}

void delete_registered_entity(ecs_world_t* world, ecs_entity_t& entity) {
    delete_entity_if_alive(world, entity);
    entity = 0;
}

void remove_component_from_all_entities(ecs_world_t* world, ecs_entity_t component) {
    if (!world || component == 0) {
        return;
    }

    ecs_query_desc_t query_desc{};
    query_desc.terms[0].id = component;
    query_desc.cache_kind = EcsQueryCacheAuto;
    ecs_query_t* query = ecs_query_init(world, &query_desc);
    if (!query) {
        return;
    }

    std::vector<ecs_entity_t> entities;
    ecs_iter_t it = ecs_query_iter(world, query);
    while (ecs_query_next(&it)) {
        for (int32_t i = 0; i < it.count; ++i) {
            entities.push_back(it.entities[i]);
        }
    }
    ecs_query_fini(query);

    for (ecs_entity_t entity : entities) {
        if (ecs_is_alive(world, entity)) {
            ecs_remove_id(world, entity, component);
        }
    }
}

} // namespace

void remove_render_window_components(ecs_world_t* world) {
    remove_component_from_all_entities(world, ecs_id(PulseGraphicsSwapchain));
    remove_component_from_all_entities(world, ecs_id(PulseGraphicsSurface));
}

void delete_render_components(ecs_world_t* world) {
    delete_entity_if_alive(world, ecs_id(PulseGraphicsSwapchain));
    delete_entity_if_alive(world, ecs_id(PulseGraphicsSurface));
    delete_registered_entity(world, ecs_id(PulseGraphicsRenderer));
    delete_registered_entity(world, ecs_id(pulse_graphics_state_resource));
    delete_registered_entity(world, pulse_graphics_render_present_phase);
    delete_registered_entity(world, pulse_graphics_render_submit_phase);
    delete_registered_entity(world, pulse_graphics_render_execute_graph_phase);
    delete_registered_entity(world, pulse_graphics_render_record_graph_phase);
    delete_registered_entity(world, pulse_graphics_render_prepare_windows_phase);
    delete_registered_entity(world, pulse_graphics_render_begin_frame_phase);

    ecs_id(PulseGraphicsSwapchain) = 0;
    ecs_id(PulseGraphicsSurface) = 0;
}

} // namespace pulse_graphics_internal
