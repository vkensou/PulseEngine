#include "internal.h"

#include <new>

namespace pulse_graphics_internal {

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

void on_sdl_window_set(ecs_iter_t* it) {
    pulse_graphics_state* state =
        static_cast<pulse_graphics_state*>(it->ctx);
    if (!state || !state->renderer.instance) {
        return;
    }

    PulseSdlWindow* sdl_windows = ecs_field(it, PulseSdlWindow, 0);
    for (int32_t i = 0; i < it->count; ++i) {
        if (!sdl_windows[i].native_view) {
            continue;
        }
        ensure_cgpu_surface(state, it->world, it->entities[i], sdl_windows[i], nullptr);
    }
}

void on_sdl_window_remove(ecs_iter_t* it) {
    pulse_graphics_state* state = state_from_world(it->world);
    if (state && state->renderer.graphics_queue) {
        cgpu_queue_wait_idle(state->renderer.graphics_queue);
    }

    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_has_id(it->world, entity, ecs_id(pulse_graphics_surface))) {
            ecs_remove_id(it->world, entity, ecs_id(pulse_graphics_surface));
        }
    }
}

void on_window_set_for_swapchain(ecs_iter_t* it) {
    pulse_graphics_state* state =
        static_cast<pulse_graphics_state*>(it->ctx);
    if (!state || !state->renderer.device) {
        return;
    }

    PulseWindow* windows = ecs_field(it, PulseWindow, 0);
    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_has_id(it->world, entity, PulseWindowCloseRequested)) {
            continue;
        }

        const pulse_graphics_surface* surface =
            ecs_get(it->world, entity, pulse_graphics_surface);
        if (!surface || !surface->surface) {
            continue;
        }

        pulse_graphics_swapchain* swapchain =
            ecs_get_mut(it->world, entity, pulse_graphics_swapchain);
        if (!swapchain || !swapchain->swapchain) {
            continue;
        }

        const bool window_resized =
            ecs_has_id(it->world, entity, PulseWindowResized);
        const bool needs_resize =
            swapchain->width != static_cast<uint32_t>(windows[i].width) ||
            swapchain->height != static_cast<uint32_t>(windows[i].height) ||
            window_resized;
        if (needs_resize) {
            cgpu_queue_wait_idle(state->renderer.graphics_queue);
            release_swapchain_resources(swapchain);
            ecs_modified(it->world, entity, pulse_graphics_swapchain);
        }
    }
}

ecs_entity_t create_named_observer(
    ecs_world_t* world,
    const char* name,
    ecs_entity_t component,
    ecs_entity_t event,
    bool yield_existing,
    ecs_iter_action_t callback,
    pulse_graphics_state* state
) {
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = name;

    ecs_observer_desc_t observer_desc{};
    observer_desc.entity = ecs_entity_init(world, &entity_desc);
    observer_desc.query.terms[0].id = component;
    observer_desc.events[0] = event;
    observer_desc.yield_existing = yield_existing;
    observer_desc.callback = callback;
    observer_desc.ctx = state;
    return ecs_observer_init(world, &observer_desc);
}

void bootstrap_existing_sdl_windows(pulse_graphics_state* state, ecs_world_t* world) {
    if (!state || !world || ecs_id(PulseSdlWindow) == 0) {
        return;
    }

    ecs_query_desc_t query_desc{};
    query_desc.terms[0].id = ecs_id(PulseSdlWindow);
    query_desc.cache_kind = EcsQueryCacheAuto;
    ecs_query_t* query = ecs_query_init(world, &query_desc);
    if (!query) {
        return;
    }

    ecs_iter_t it = ecs_query_iter(world, query);
    while (ecs_query_next(&it)) {
        PulseSdlWindow* sdl_windows = ecs_field(&it, PulseSdlWindow, 0);
        for (int32_t i = 0; i < it.count; ++i) {
            if (sdl_windows[i].native_view) {
                ensure_cgpu_surface(state, world, it.entities[i], sdl_windows[i], nullptr);
            }
        }
    }

    ecs_query_fini(query);
}

bool create_or_resize_swapchain(
    const pulse_graphics_state* state,
    const pulse_graphics_surface* surface,
    pulse_graphics_swapchain* swapchain,
    uint32_t width,
    uint32_t height
) {
    const pulse_graphics_renderer& renderer = state->renderer;
    release_swapchain_resources(swapchain);

    swapchain->device = renderer.device;
    swapchain->width = width;
    swapchain->height = height;

    CGPUSwapChainDescriptor swapchain_desc{};
    swapchain_desc.present_queue_count = 1;
    swapchain_desc.p_present_queues = &renderer.present_queue;
    swapchain_desc.surface = surface->surface;
    swapchain_desc.image_count = renderer.image_count;
    swapchain_desc.width = width;
    swapchain_desc.height = height;
    swapchain_desc.enable_vsync = state->desc.enable_vsync;
    swapchain_desc.format = renderer.swapchain_format;

    swapchain->swapchain =
        cgpu_device_create_swap_chain(renderer.device, &swapchain_desc);
    if (!swapchain->swapchain) {
        return false;
    }

    const uint32_t count = swapchain->swapchain->back_buffer_count;
    swapchain->backbuffer_count = count;
    swapchain->backbuffer_views = new (std::nothrow) CGPUTextureViewId[count]{};
    swapchain->framebuffers = new (std::nothrow) CGPUFramebufferId[count]{};
    swapchain->image_available_semaphores = new (std::nothrow) CGPUSemaphoreId[count]{};
    swapchain->render_finished_semaphores = new (std::nothrow) CGPUSemaphoreId[count]{};
    if (!swapchain->backbuffer_views ||
        !swapchain->framebuffers ||
        !swapchain->image_available_semaphores ||
        !swapchain->render_finished_semaphores) {
        return false;
    }

    for (uint32_t i = 0; i < count; ++i) {
        CGPUTextureId backbuffer = swapchain->swapchain->p_back_buffers[i];

        CGPUTextureViewDescriptor view_desc{};
        view_desc.texture = backbuffer;
        view_desc.format = backbuffer->info->format;
        view_desc.usages = CGPU_TEXTURE_VIEW_USAGE_RTV_DSV;
        view_desc.aspects = CGPU_TEXTURE_VIEW_ASPECT_COLOR;
        view_desc.dims = CGPU_TEXTURE_DIMENSION_2D;
        view_desc.array_layer_count = 1;
        view_desc.mip_level_count = 1;
        swapchain->backbuffer_views[i] =
            cgpu_device_create_texture_view(renderer.device, &view_desc);
        if (!swapchain->backbuffer_views[i]) {
            return false;
        }

        CGPUFramebufferDescriptor framebuffer_desc{};
        framebuffer_desc.renderpass = renderer.render_pass;
        framebuffer_desc.attachment_count = 1;
        framebuffer_desc.p_attachments[0] = swapchain->backbuffer_views[i];
        framebuffer_desc.width = width;
        framebuffer_desc.height = height;
        framebuffer_desc.layers = 1;
        swapchain->framebuffers[i] =
            cgpu_device_create_framebuffer(renderer.device, &framebuffer_desc);
        if (!swapchain->framebuffers[i]) {
            return false;
        }

        swapchain->image_available_semaphores[i] =
            cgpu_device_create_semaphore(renderer.device);
        swapchain->render_finished_semaphores[i] =
            cgpu_device_create_semaphore(renderer.device);
        if (!swapchain->image_available_semaphores[i] ||
            !swapchain->render_finished_semaphores[i]) {
            return false;
        }
    }

    swapchain->backbuffers = new (std::nothrow) pulse_backbuffer_data_t[count]{};
    if (!swapchain->backbuffers) {
        return false;
    }
    for (uint32_t i = 0; i < count; ++i) {
        HGEGraphics::init_backbuffer(
            &static_cast<pulse_backbuffer_data_t*>(swapchain->backbuffers)[i],
            swapchain->swapchain,
            static_cast<int>(i)
        );
    }

    return true;
}

} // namespace

void install_observers(pulse_graphics_state* state, ecs_world_t* world) {
    if (!state || !world) {
        return;
    }

    ensure_component_relations(world);

    if (!state->window_on_set_observer && ecs_id(PulseWindow) != 0) {
        state->window_on_set_observer = create_named_observer(
            world,
            "PulseCgpuSwapchainFromWindow",
            ecs_id(PulseWindow),
            EcsOnSet,
            true,
            on_window_set_for_swapchain,
            state
        );
    }

    if (!state->sdl_window_on_set_observer && ecs_id(PulseSdlWindow) != 0) {
        state->sdl_window_on_set_observer = create_named_observer(
            world,
            "PulseCgpuSurfaceFromSdlWindow",
            ecs_id(PulseSdlWindow),
            EcsOnSet,
            true,
            on_sdl_window_set,
            state
        );
    }

    if (!state->sdl_window_on_remove_observer && ecs_id(PulseSdlWindow) != 0) {
        state->sdl_window_on_remove_observer = create_named_observer(
            world,
            "PulseCgpuSurfaceRemoveFromSdlWindow",
            ecs_id(PulseSdlWindow),
            EcsOnRemove,
            false,
            on_sdl_window_remove,
            state
        );
    }

    if (state->sdl_window_on_set_observer &&
        !state->existing_sdl_windows_bootstrapped) {
        bootstrap_existing_sdl_windows(state, world);
        state->existing_sdl_windows_bootstrapped = true;
    }
}

void uninstall_observers(pulse_graphics_state* state, ecs_world_t* world) {
    if (!state) {
        return;
    }

    delete_registered_entity(world, state->window_on_set_observer);
    delete_registered_entity(world, state->sdl_window_on_remove_observer);
    delete_registered_entity(world, state->sdl_window_on_set_observer);
    state->existing_sdl_windows_bootstrapped = false;
}

bool ensure_cgpu_surface(
    pulse_graphics_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const PulseSdlWindow& sdl_window,
    pulse_graphics_surface** out_surface
) {
    if (!sdl_window.native_view) {
        return false;
    }

    if (!ecs_has_id(world, entity, ecs_id(pulse_graphics_surface))) {
        ecs_add_id(world, entity, ecs_id(pulse_graphics_surface));
    }

    pulse_graphics_surface* surface = ecs_get_mut(world, entity, pulse_graphics_surface);
    if (!surface) {
        return false;
    }

    bool surface_created = false;
    if (!surface->surface) {
        surface->instance = state->renderer.instance;
        surface->surface = cgpu_instance_create_surface_from_native_view(
            state->renderer.instance,
            sdl_window.native_view
        );
        if (!surface->surface) {
            return false;
        }
        surface_created = true;
    }

    if (surface_created) {
        ecs_modified(world, entity, pulse_graphics_surface);
        surface = ecs_get_mut(world, entity, pulse_graphics_surface);
    }
    if (out_surface) {
        *out_surface = surface;
    }
    return true;
}

bool ensure_cgpu_swapchain(
    pulse_graphics_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const PulseWindow& window,
    const pulse_graphics_surface* surface,
    pulse_graphics_swapchain** out_swapchain
) {
    if (!surface || !surface->surface || window.width <= 0 || window.height <= 0) {
        return false;
    }

    if (!ecs_has_id(world, entity, ecs_id(pulse_graphics_swapchain))) {
        ecs_add_id(world, entity, ecs_id(pulse_graphics_swapchain));
    }

    surface = ecs_get(world, entity, pulse_graphics_surface);
    if (!surface || !surface->surface) {
        return false;
    }

    pulse_graphics_swapchain* swapchain =
        ecs_get_mut(world, entity, pulse_graphics_swapchain);
    if (!swapchain) {
        return false;
    }

    const bool needs_swapchain =
        swapchain->swapchain == CGPU_NULLPTR ||
        swapchain->width != static_cast<uint32_t>(window.width) ||
        swapchain->height != static_cast<uint32_t>(window.height) ||
        ecs_has_id(world, entity, PulseWindowResized);

    if (needs_swapchain) {
        if (state->renderer.graphics_queue) {
            cgpu_queue_wait_idle(state->renderer.graphics_queue);
        }

        if (!create_or_resize_swapchain(
                state,
                surface,
                swapchain,
                static_cast<uint32_t>(window.width),
                static_cast<uint32_t>(window.height))) {
            release_swapchain_resources(swapchain);
            return false;
        }
    }

    ecs_modified(world, entity, pulse_graphics_swapchain);
    if (out_swapchain) {
        *out_swapchain = swapchain;
    }
    return swapchain->swapchain != CGPU_NULLPTR;
}

bool acquire_window_image(
    pulse_graphics_swapchain* swapchain,
    uint32_t frame_index
) {
    swapchain->current_backbuffer_index = UINT32_MAX;
    if (!swapchain->swapchain || swapchain->backbuffer_count == 0) {
        return false;
    }

    CGPUSemaphoreId signal =
        swapchain->image_available_semaphores[frame_index % swapchain->backbuffer_count];
    CGPUAcquireNextDescriptor acquire_desc{};
    acquire_desc.signal_semaphore = signal;

    ECGPUAcquireNextImageError result = cgpu_swap_chain_acquire_next_image(
        swapchain->swapchain,
        &acquire_desc,
        &swapchain->current_backbuffer_index
    );

    if (result != CGPU_ACQUIRE_NEXT_IMAGE_ERROR_SUCCESS &&
        result != CGPU_ACQUIRE_NEXT_IMAGE_ERROR_SUB_OPTIMAL) {
        return false;
    }

    return swapchain->current_backbuffer_index < swapchain->backbuffer_count;
}

} // namespace pulse_graphics_internal
