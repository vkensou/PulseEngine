#include "render_internal.h"

#include <new>
#include <algorithm>

namespace pulse_cgpu_render_internal {

constexpr const char* kPluginName = "PulseCgpuRenderPlugin";
constexpr uint32_t kDefaultImageCount = 3;

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

} // namespace

void pulse_cgpu_render_state::sort_record_callbacks() {
    std::stable_sort(record_callbacks.begin(), record_callbacks.end(),
        [](const pulse_cgpu_renderer_record_callback_desc& a,
           const pulse_cgpu_renderer_record_callback_desc& b) {
            return a.priority < b.priority;
        });
}

pulse_cgpu_render_state* state_from_world(ecs_world_t* world) {
    if (!world || ecs_id(pulse_cgpu_render_state_resource) == 0) {
        return nullptr;
    }

    const pulse_cgpu_render_state_resource* resource =
        ecs_singleton_get(world, pulse_cgpu_render_state_resource);
    return resource ? resource->state : nullptr;
}

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
        if (ecs_has_id(it->world, entity, PulseWindowCloseRequested)) {
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

        const bool window_resized =
            ecs_has_id(it->world, entity, PulseWindowResized);
        const bool needs_resize =
            swapchain->width != static_cast<uint32_t>(windows[i].width) ||
            swapchain->height != static_cast<uint32_t>(windows[i].height) ||
            window_resized;
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

void uninstall_observers(pulse_cgpu_render_state* state, ecs_world_t* world) {
    if (!state) {
        return;
    }

    delete_registered_entity(world, state->window_on_set_observer);
    delete_registered_entity(world, state->sdl_window_on_remove_observer);
    delete_registered_entity(world, state->sdl_window_on_set_observer);
    state->existing_sdl_windows_bootstrapped = false;
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
    remove_component_from_all_entities(world, ecs_id(pulse_cgpu_swapchain));
    remove_component_from_all_entities(world, ecs_id(pulse_cgpu_surface));
}

void delete_render_components(ecs_world_t* world) {
    delete_entity_if_alive(world, ecs_id(pulse_cgpu_swapchain));
    delete_entity_if_alive(world, ecs_id(pulse_cgpu_surface));
    delete_registered_entity(world, ecs_id(pulse_cgpu_renderer));
    delete_registered_entity(world, ecs_id(pulse_cgpu_render_state_resource));
    delete_registered_entity(world, pulse_cgpu_render_present_phase);
    delete_registered_entity(world, pulse_cgpu_render_submit_phase);
    delete_registered_entity(world, pulse_cgpu_render_execute_graph_phase);
    delete_registered_entity(world, pulse_cgpu_render_record_graph_phase);
    delete_registered_entity(world, pulse_cgpu_render_prepare_windows_phase);
    delete_registered_entity(world, pulse_cgpu_render_begin_frame_phase);

    ecs_id(pulse_cgpu_swapchain) = 0;
    ecs_id(pulse_cgpu_surface) = 0;
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

    uninstall_render_systems(state, world);
    uninstall_observers(state, world);
    remove_render_window_components(world);

    if (world && ecs_id(pulse_cgpu_renderer) != 0) {
        ecs_singleton_remove(world, pulse_cgpu_renderer);
    }
    if (world && ecs_id(pulse_cgpu_render_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_cgpu_render_state_resource);
    }

    delete_render_components(world);
    destroy_renderer(state);
    delete state;
}

} // namespace pulse_cgpu_render_internal

using namespace pulse_cgpu_render_internal;

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
    desc.record_callback = nullptr;
    desc.record_user_data = nullptr;
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

pulse_result_t pulse_cgpu_render_add_record_callback(
    pulse_app_t app,
    const pulse_cgpu_renderer_record_callback_desc* desc
) {
    pulse_cgpu_render_state* state = pulse_cgpu_render_internal::state_from_world(pulse_app_world(app));
    if (!state || !desc || !desc->callback) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }
    state->record_callbacks.push_back(*desc);
    state->sort_record_callbacks();
    return PULSE_OK;
}

pulse_result_t pulse_cgpu_render_remove_record_callback(
    pulse_app_t app,
    pulse_cgpu_render_record_callback callback
) {
    pulse_cgpu_render_state* state = pulse_cgpu_render_internal::state_from_world(pulse_app_world(app));
    if (!state || !callback) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }
    auto it = std::find_if(state->record_callbacks.begin(), state->record_callbacks.end(),
        [callback](const pulse_cgpu_renderer_record_callback_desc& d) {
            return d.callback == callback;
        });
    if (it != state->record_callbacks.end()) {
        state->record_callbacks.erase(it);
    }
    return PULSE_OK;
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
