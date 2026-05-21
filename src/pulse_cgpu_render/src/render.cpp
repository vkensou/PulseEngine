#include "pulse_cgpu_render.h"

#include "pulse_window.h"

#include <new>
#include <vector>

ECS_COMPONENT_DECLARE(pulse_cgpu_renderer);
ECS_COMPONENT_DECLARE(pulse_cgpu_surface);
ECS_COMPONENT_DECLARE(pulse_cgpu_swapchain);

namespace {

constexpr const char* kPluginName = "PulseCgpuRenderPlugin";
constexpr uint32_t kDefaultImageCount = 3;

struct frame_data {
    CGPUFenceId fence = CGPU_NULLPTR;
    CGPUCommandPoolId pool = CGPU_NULLPTR;
    std::vector<CGPUCommandBufferId> available_cmds;
    std::vector<CGPUCommandBufferId> submitted_cmds;

    bool init(CGPUDeviceId device, CGPUQueueId queue) {
        fence = cgpu_device_create_fence(device);
        if (!fence) {
            return false;
        }

        CGPUCommandPoolDescriptor pool_desc{};
        pool_desc.name = "pulse_cgpu_render_frame_pool";
        pool = cgpu_queue_create_command_pool(queue, &pool_desc);
        return pool != CGPU_NULLPTR;
    }

    void begin_frame() {
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

    CGPUCommandBufferId request_command_buffer() {
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

    void destroy() {
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

    void reset() {
        frame_index = 0;
        frame = nullptr;
        active = false;
        submitted = false;
        failed = false;
        prepared_entities.clear();
        wait_semaphores.clear();
        signal_semaphores.clear();
    }
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

ECS_COMPONENT_DECLARE(pulse_cgpu_render_state_resource);

pulse_cgpu_render_state* state_from_world(ecs_world_t* world) {
    if (!world || ecs_id(pulse_cgpu_render_state_resource) == 0) {
        return nullptr;
    }

    const pulse_cgpu_render_state_resource* resource =
        ecs_singleton_get(world, pulse_cgpu_render_state_resource);
    return resource ? resource->state : nullptr;
}

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

bool validate_plugin_desc(const pulse_cgpu_render_plugin_desc* desc) {
    return !desc ||
        (desc->struct_size == sizeof(pulse_cgpu_render_plugin_desc) &&
         desc->version == PULSE_CGPU_RENDER_PLUGIN_DESC_VERSION);
}

pulse_cgpu_render_plugin_desc normalize_plugin_desc(
    const pulse_cgpu_render_plugin_desc* desc
) {
    pulse_cgpu_render_plugin_desc normalized = pulse_cgpu_render_plugin_desc_default();
    if (desc) {
        normalized = *desc;
    }

    normalized.struct_size = sizeof(pulse_cgpu_render_plugin_desc);
    normalized.version = PULSE_CGPU_RENDER_PLUGIN_DESC_VERSION;
    if (normalized.image_count == 0) {
        normalized.image_count = kDefaultImageCount;
    }
    if (normalized.swapchain_format == CGPU_TEXTURE_FORMAT_UNDEFINED) {
        normalized.swapchain_format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    }
    return normalized;
}

bool ensure_cgpu_surface(
    pulse_cgpu_render_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const pulse_sdl_window& sdl_window,
    pulse_cgpu_surface** out_surface
);

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, pulse_cgpu_renderer);
    ECS_COMPONENT_DEFINE(world, pulse_cgpu_surface);
    ECS_COMPONENT_DEFINE(world, pulse_cgpu_swapchain);
    ECS_COMPONENT_DEFINE(world, pulse_cgpu_render_state_resource);

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

void on_sdl_window_set(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state || !state->renderer.instance) {
        return;
    }

    pulse_sdl_window* sdl_windows = ecs_field(it, pulse_sdl_window, 0);
    for (int32_t i = 0; i < it->count; ++i) {
        if (!sdl_windows[i].native_view) {
            continue;
        }
        ensure_cgpu_surface(state, it->world, it->entities[i], sdl_windows[i], nullptr);
    }
}

void on_sdl_window_remove(ecs_iter_t* it) {
    pulse_cgpu_render_state* state = state_from_world(it->world);
    if (state && state->renderer.graphics_queue) {
        cgpu_queue_wait_idle(state->renderer.graphics_queue);
    }

    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_has_id(it->world, entity, ecs_id(pulse_cgpu_surface))) {
            ecs_remove_id(it->world, entity, ecs_id(pulse_cgpu_surface));
        }
    }
}

void on_window_set_for_swapchain(ecs_iter_t* it) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(it->ctx);
    if (!state || !state->renderer.device) {
        return;
    }

    pulse_window* windows = ecs_field(it, pulse_window, 0);
    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (windows[i].close_requested) {
            continue;
        }

        const pulse_cgpu_surface* surface =
            ecs_get(it->world, entity, pulse_cgpu_surface);
        if (!surface || !surface->surface) {
            continue;
        }

        pulse_cgpu_swapchain* swapchain =
            ecs_get_mut(it->world, entity, pulse_cgpu_swapchain);
        if (!swapchain || !swapchain->swapchain) {
            continue;
        }

        const bool needs_resize =
            swapchain->width != static_cast<uint32_t>(windows[i].width) ||
            swapchain->height != static_cast<uint32_t>(windows[i].height) ||
            windows[i].resized;
        if (needs_resize) {
            cgpu_queue_wait_idle(state->renderer.graphics_queue);
            release_swapchain_resources(swapchain);
            ecs_modified(it->world, entity, pulse_cgpu_swapchain);
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
    pulse_cgpu_render_state* state
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

void bootstrap_existing_sdl_windows(pulse_cgpu_render_state* state, ecs_world_t* world) {
    if (!state || !world || ecs_id(pulse_sdl_window) == 0) {
        return;
    }

    ecs_query_desc_t query_desc{};
    query_desc.terms[0].id = ecs_id(pulse_sdl_window);
    query_desc.cache_kind = EcsQueryCacheAuto;
    ecs_query_t* query = ecs_query_init(world, &query_desc);
    if (!query) {
        return;
    }

    ecs_iter_t it = ecs_query_iter(world, query);
    while (ecs_query_next(&it)) {
        pulse_sdl_window* sdl_windows = ecs_field(&it, pulse_sdl_window, 0);
        for (int32_t i = 0; i < it.count; ++i) {
            if (sdl_windows[i].native_view) {
                ensure_cgpu_surface(state, world, it.entities[i], sdl_windows[i], nullptr);
            }
        }
    }

    ecs_query_fini(query);
}

void install_observers(pulse_cgpu_render_state* state, ecs_world_t* world) {
    if (!state || !world) {
        return;
    }

    ensure_component_relations(world);

    if (!state->window_on_set_observer && ecs_id(pulse_window) != 0) {
        state->window_on_set_observer = create_named_observer(
            world,
            "PulseCgpuSwapchainFromWindow",
            ecs_id(pulse_window),
            EcsOnSet,
            true,
            on_window_set_for_swapchain,
            state
        );
    }

    if (!state->sdl_window_on_set_observer && ecs_id(pulse_sdl_window) != 0) {
        state->sdl_window_on_set_observer = create_named_observer(
            world,
            "PulseCgpuSurfaceFromSdlWindow",
            ecs_id(pulse_sdl_window),
            EcsOnSet,
            true,
            on_sdl_window_set,
            state
        );
    }

    if (!state->sdl_window_on_remove_observer && ecs_id(pulse_sdl_window) != 0) {
        state->sdl_window_on_remove_observer = create_named_observer(
            world,
            "PulseCgpuSurfaceRemoveFromSdlWindow",
            ecs_id(pulse_sdl_window),
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

bool create_renderer(pulse_cgpu_render_state* state) {
    pulse_cgpu_renderer& renderer = state->renderer;

    CGPUInstanceDescriptor instance_desc{};
    instance_desc.backend = state->desc.backend;
    instance_desc.enable_debug_layer = state->desc.enable_debug_layer;
    instance_desc.enable_gpu_based_validation = state->desc.enable_gpu_based_validation;
    instance_desc.enable_set_name = state->desc.enable_debug_layer;

    renderer.instance = cgpu_create_instance(&instance_desc);
    if (!renderer.instance) {
        return false;
    }

    uint32_t adapter_count = 0;
    cgpu_instance_enum_adapters(renderer.instance, &adapter_count, CGPU_NULLPTR);
    if (adapter_count == 0) {
        return false;
    }

    std::vector<CGPUAdapterId> adapters(adapter_count);
    cgpu_instance_enum_adapters(renderer.instance, &adapter_count, adapters.data());
    renderer.adapter = adapters[0];

    CGPUQueueGroupDescriptor queue_group{};
    queue_group.queue_type = CGPU_QUEUE_TYPE_GRAPHICS;
    queue_group.queue_count = 1;

    CGPUDeviceDescriptor device_desc{};
    device_desc.queue_group_count = 1;
    device_desc.p_queue_groups = &queue_group;

    renderer.device = cgpu_adapter_create_device(renderer.adapter, &device_desc);
    if (!renderer.device) {
        return false;
    }

    renderer.graphics_queue =
        cgpu_device_get_queue(renderer.device, CGPU_QUEUE_TYPE_GRAPHICS, 0);
    if (!renderer.graphics_queue) {
        return false;
    }

    renderer.present_queue = renderer.graphics_queue;
    renderer.backend = state->desc.backend;
    renderer.swapchain_format = state->desc.swapchain_format;
    renderer.image_count = state->desc.image_count;
    renderer.frame_index = 0;

    CGPURenderPassDescriptor render_pass_desc{};
    render_pass_desc.sample_count = CGPU_SAMPLE_COUNT_1;
    render_pass_desc.color_attachments[0].format = renderer.swapchain_format;
    render_pass_desc.color_attachments[0].load_action = CGPU_LOAD_ACTION_CLEAR;
    render_pass_desc.color_attachments[0].store_action = CGPU_STORE_ACTION_STORE;
    renderer.render_pass =
        cgpu_device_create_render_pass(renderer.device, &render_pass_desc);
    if (!renderer.render_pass) {
        return false;
    }

    state->frames.resize(renderer.image_count);
    for (frame_data& frame : state->frames) {
        if (!frame.init(renderer.device, renderer.graphics_queue)) {
            return false;
        }
    }

    return true;
}

void destroy_renderer(pulse_cgpu_render_state* state) {
    pulse_cgpu_renderer& renderer = state->renderer;

    if (renderer.graphics_queue) {
        cgpu_queue_wait_idle(renderer.graphics_queue);
    }

    for (frame_data& frame : state->frames) {
        frame.destroy();
    }
    state->frames.clear();

    if (renderer.render_pass) {
        cgpu_device_free_render_pass(renderer.device, renderer.render_pass);
        renderer.render_pass = CGPU_NULLPTR;
    }

    if (renderer.graphics_queue) {
        cgpu_device_free_queue(renderer.device, renderer.graphics_queue);
        renderer.graphics_queue = CGPU_NULLPTR;
        renderer.present_queue = CGPU_NULLPTR;
    }

    if (renderer.device) {
        cgpu_adapter_free_device(renderer.device->adapter, renderer.device);
        renderer.device = CGPU_NULLPTR;
    }

    if (renderer.instance) {
        cgpu_free_instance(renderer.instance);
        renderer.instance = CGPU_NULLPTR;
    }

    renderer.adapter = CGPU_NULLPTR;
}

bool create_or_resize_swapchain(
    const pulse_cgpu_render_state* state,
    const pulse_cgpu_surface* surface,
    pulse_cgpu_swapchain* swapchain,
    uint32_t width,
    uint32_t height
) {
    const pulse_cgpu_renderer& renderer = state->renderer;
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

    return true;
}

bool ensure_cgpu_surface(
    pulse_cgpu_render_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const pulse_sdl_window& sdl_window,
    pulse_cgpu_surface** out_surface
) {
    if (!sdl_window.native_view) {
        return false;
    }

    if (!ecs_has_id(world, entity, ecs_id(pulse_cgpu_surface))) {
        ecs_add_id(world, entity, ecs_id(pulse_cgpu_surface));
    }

    pulse_cgpu_surface* surface = ecs_get_mut(world, entity, pulse_cgpu_surface);
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
        ecs_modified(world, entity, pulse_cgpu_surface);
        surface = ecs_get_mut(world, entity, pulse_cgpu_surface);
    }
    if (out_surface) {
        *out_surface = surface;
    }
    return true;
}

bool ensure_cgpu_swapchain(
    pulse_cgpu_render_state* state,
    ecs_world_t* world,
    ecs_entity_t entity,
    const pulse_window& window,
    const pulse_cgpu_surface* surface,
    pulse_cgpu_swapchain** out_swapchain
) {
    if (!surface || !surface->surface || window.width <= 0 || window.height <= 0) {
        return false;
    }

    if (!ecs_has_id(world, entity, ecs_id(pulse_cgpu_swapchain))) {
        ecs_add_id(world, entity, ecs_id(pulse_cgpu_swapchain));
    }

    surface = ecs_get(world, entity, pulse_cgpu_surface);
    if (!surface || !surface->surface) {
        return false;
    }

    pulse_cgpu_swapchain* swapchain =
        ecs_get_mut(world, entity, pulse_cgpu_swapchain);
    if (!swapchain) {
        return false;
    }

    const bool needs_swapchain =
        swapchain->swapchain == CGPU_NULLPTR ||
        swapchain->width != static_cast<uint32_t>(window.width) ||
        swapchain->height != static_cast<uint32_t>(window.height) ||
        window.resized;

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

    ecs_modified(world, entity, pulse_cgpu_swapchain);
    if (out_swapchain) {
        *out_swapchain = swapchain;
    }
    return swapchain->swapchain != CGPU_NULLPTR;
}

bool acquire_window_image(
    pulse_cgpu_swapchain* swapchain,
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

void encode_clear_pass(
    const pulse_cgpu_render_state* state,
    CGPUCommandBufferId cmd,
    pulse_cgpu_swapchain* swapchain
) {
    const uint32_t image_index = swapchain->current_backbuffer_index;
    CGPUTextureId backbuffer = swapchain->swapchain->p_back_buffers[image_index];

    CGPUTextureBarrier draw_barrier{};
    draw_barrier.texture = backbuffer;
    draw_barrier.src_state = CGPU_RESOURCE_STATE_UNDEFINED;
    draw_barrier.dst_state = CGPU_RESOURCE_STATE_RENDER_TARGET;
    CGPUResourceBarrierDescriptor draw_barrier_desc{};
    draw_barrier_desc.texture_barrier_count = 1;
    draw_barrier_desc.p_texture_barriers = &draw_barrier;
    cgpu_command_buffer_resource_barrier(cmd, &draw_barrier_desc);

    CGPUClearValue clear{};
    clear.color[0] = state->desc.clear_color[0];
    clear.color[1] = state->desc.clear_color[1];
    clear.color[2] = state->desc.clear_color[2];
    clear.color[3] = state->desc.clear_color[3];
    clear.is_color = true;

    CGPUBeginRenderPassInfo begin_info{};
    begin_info.render_pass = state->renderer.render_pass;
    begin_info.framebuffer = swapchain->framebuffers[image_index];
    begin_info.clear_value_count = 1;
    begin_info.p_clear_values = &clear;

    CGPURenderPassEncoderId encoder =
        cgpu_command_buffer_begin_render_pass(cmd, &begin_info);
    cgpu_render_pass_encoder_set_shading_rate(
        encoder,
        CGPU_SHADING_RATE_FULL,
        CGPU_SHADING_RATE_COMBINER_PASS_THROUGH,
        CGPU_SHADING_RATE_COMBINER_PASS_THROUGH
    );
    cgpu_render_pass_encoder_set_viewport(
        encoder,
        0.0f,
        0.0f,
        static_cast<float>(swapchain->width),
        static_cast<float>(swapchain->height),
        0.0f,
        1.0f
    );
    cgpu_render_pass_encoder_set_scissor(
        encoder,
        0,
        0,
        swapchain->width,
        swapchain->height
    );
    cgpu_command_buffer_end_render_pass(cmd, encoder);

    CGPUTextureBarrier present_barrier{};
    present_barrier.texture = backbuffer;
    present_barrier.src_state = CGPU_RESOURCE_STATE_RENDER_TARGET;
    present_barrier.dst_state = CGPU_RESOURCE_STATE_PRESENT;
    CGPUResourceBarrierDescriptor present_barrier_desc{};
    present_barrier_desc.texture_barrier_count = 1;
    present_barrier_desc.p_texture_barriers = &present_barrier;
    cgpu_command_buffer_resource_barrier(cmd, &present_barrier_desc);
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

    ecs_world_t* world = it->world;
    pulse_window* windows = ecs_field(it, pulse_window, 0);
    pulse_cgpu_surface* surfaces = ecs_field(it, pulse_cgpu_surface, 1);
    pulse_cgpu_swapchain* swapchains = ecs_field(it, pulse_cgpu_swapchain, 2);
    render_frame_context& frame = state->frame_context;

    for (int32_t i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (windows[i].close_requested) {
            continue;
        }

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
    system_desc.phase = EcsPostFrame;
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
    system_desc.phase = EcsPostFrame;
    system_desc.query.terms[0].id = ecs_id(pulse_window);
    system_desc.query.terms[1].id = ecs_id(pulse_cgpu_surface);
    system_desc.query.terms[2].id = ecs_id(pulse_cgpu_swapchain);
    system_desc.query.cache_kind = EcsQueryCacheAuto;
    system_desc.callback = render_prepare_windows_system_run;
    system_desc.ctx = state;
    system_desc.immediate = true;
    return ecs_system_init(world, &system_desc);
}

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

pulse_result_t render_plugin_build(pulse_app_t app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(ctx);
    if (!world || !state) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    state->app = app;
    register_components(world);

    pulse_cgpu_render_state_resource resource{};
    resource.state = state;
    ecs_singleton_set_ptr(world, pulse_cgpu_render_state_resource, &resource);

    if (!create_renderer(state)) {
        destroy_renderer(state);
        return PULSE_ERROR_INTERNAL;
    }
    ecs_singleton_set_ptr(world, pulse_cgpu_renderer, &state->renderer);
    install_observers(state, world);
    install_render_systems(state, world);

    return PULSE_OK;
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

void remove_render_window_components(ecs_world_t* world) {
    remove_component_from_all_entities(world, ecs_id(pulse_cgpu_surface));
    remove_component_from_all_entities(world, ecs_id(pulse_cgpu_swapchain));
}

void render_plugin_shutdown(pulse_app_t app, void* ctx) {
    pulse_cgpu_render_state* state =
        static_cast<pulse_cgpu_render_state*>(ctx);
    if (!state) {
        return;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (state->renderer.graphics_queue) {
        cgpu_queue_wait_idle(state->renderer.graphics_queue);
    }

    remove_render_window_components(world);

    if (world && ecs_id(pulse_cgpu_renderer) != 0) {
        ecs_singleton_remove(world, pulse_cgpu_renderer);
    }
    if (world && ecs_id(pulse_cgpu_render_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_cgpu_render_state_resource);
    }

    destroy_renderer(state);
    delete state;
}

} // namespace

extern "C" {

pulse_cgpu_render_plugin_desc pulse_cgpu_render_plugin_desc_default(void) {
    pulse_cgpu_render_plugin_desc desc{};
    desc.struct_size = sizeof(pulse_cgpu_render_plugin_desc);
    desc.version = PULSE_CGPU_RENDER_PLUGIN_DESC_VERSION;
    desc.backend = CGPU_BACKEND_VULKAN;
    desc.swapchain_format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.image_count = kDefaultImageCount;
    desc.enable_debug_layer = false;
    desc.enable_gpu_based_validation = false;
    desc.enable_vsync = true;
    desc.clear_color[0] = 0.02f;
    desc.clear_color[1] = 0.02f;
    desc.clear_color[2] = 0.025f;
    desc.clear_color[3] = 1.0f;
    return desc;
}

pulse_result_t pulse_cgpu_render_add_plugin(
    pulse_app_t app,
    const pulse_cgpu_render_plugin_desc* desc
) {
    if (!app || !validate_plugin_desc(desc)) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_ERROR_DUPLICATE_PLUGIN;
    }

    pulse_cgpu_render_state* state = new pulse_cgpu_render_state();
    state->desc = normalize_plugin_desc(desc);

    pulse_plugin_desc plugin_desc = {
        sizeof(pulse_plugin_desc),
        PULSE_PLUGIN_DESC_VERSION,
        kPluginName,
        state,
        render_plugin_build,
        nullptr,
        render_plugin_shutdown,
    };

    pulse_result_t result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

const pulse_cgpu_renderer* pulse_cgpu_renderer_get(pulse_app_t app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(pulse_cgpu_renderer) == 0) {
        return nullptr;
    }
    return ecs_singleton_get(world, pulse_cgpu_renderer);
}

const pulse_cgpu_surface* pulse_cgpu_surface_get(pulse_app_t app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) ||
        ecs_id(pulse_cgpu_surface) == 0) {
        return nullptr;
    }
    return ecs_get(world, entity, pulse_cgpu_surface);
}

const pulse_cgpu_swapchain* pulse_cgpu_swapchain_get(pulse_app_t app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) ||
        ecs_id(pulse_cgpu_swapchain) == 0) {
        return nullptr;
    }
    return ecs_get(world, entity, pulse_cgpu_swapchain);
}

} // extern "C"
