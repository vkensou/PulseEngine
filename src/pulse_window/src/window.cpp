#include "window_internal.h"

#include <algorithm>

namespace pulse_window_internal {

constexpr const char* kPluginName = "PulseWindowPlugin";

pulse_window_desc normalize_window_desc(const pulse_window_desc* desc) {
    pulse_window_desc normalized = pulse_window_desc_default();
    if (desc) {
        normalized = *desc;
    }

    normalized.struct_size = sizeof(pulse_window_desc);
    if (!normalized.title) {
        normalized.title = "Pulse Window";
    }
    if (normalized.width <= 0) {
        normalized.width = 1280;
    }
    if (normalized.height <= 0) {
        normalized.height = 720;
    }
    if (normalized.flags == 0) {
        normalized.flags = PULSE_WINDOW_DEFAULT_FLAGS;
    }

    return normalized;
}

pulse_window_plugin_desc normalize_plugin_desc(const pulse_window_plugin_desc* desc) {
    pulse_window_plugin_desc normalized = pulse_window_plugin_desc_default();
    if (desc) {
        normalized = *desc;
    }

    normalized.struct_size = sizeof(pulse_window_plugin_desc);
    normalized.version = PULSE_WINDOW_PLUGIN_DESC_VERSION;
    if (normalized.sdl_init_flags == 0) {
        normalized.sdl_init_flags = SDL_INIT_VIDEO;
    }
    normalized.primary_window = normalize_window_desc(
        normalized.primary_window.struct_size == sizeof(pulse_window_desc)
            ? &normalized.primary_window
            : nullptr
    );
    normalized.primary_window.primary = true;

    return normalized;
}

bool validate_window_desc(const pulse_window_desc* desc) {
    return desc && desc->struct_size == sizeof(pulse_window_desc);
}

bool validate_plugin_desc(const pulse_window_plugin_desc* desc) {
    return !desc ||
        (desc->struct_size == sizeof(pulse_window_plugin_desc) &&
         desc->version == PULSE_WINDOW_PLUGIN_DESC_VERSION);
}

pulse_result_t pulse_window_create(
    pulse_app_t app,
    const pulse_window_desc* desc,
    ecs_entity_t* out_entity
) {
    if (!app || !validate_window_desc(desc)) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_ERROR_INVALID_STATE;
    }

    pulse_window_desc normalized = normalize_window_desc(desc);
    if (normalized.primary && pulse_window_primary(app) != 0) {
        return PULSE_ERROR_INVALID_STATE;
    }

    ecs_entity_desc_t entity_desc{};
    entity_desc.name = normalized.title;
    ecs_entity_t entity = ecs_entity_init(world, &entity_desc);
    if (!entity) {
        return PULSE_ERROR_INTERNAL;
    }

    pulse_window window_component{};
    window_component.title = normalized.title;
    window_component.width = normalized.width;
    window_component.height = normalized.height;
    window_component.flags = normalized.flags;
    ecs_set_ptr(world, entity, pulse_window, &window_component);

    if (normalized.primary) {
        ecs_add_id(world, entity, PulsePrimaryWindow);
    }

    if (out_entity) {
        *out_entity = entity;
    }

    return PULSE_OK;
}

void destroy_all_windows(pulse_window_plugin_state* state) {
    if (!state) {
        return;
    }

    ecs_world_t* world = pulse_app_world(state->app);
    if (!world) {
        return;
    }

    ecs_iter_t it = ecs_query_iter(world, state->window_query);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; i++) {
            auto entity = it.entities[i];
			ecs_remove_id(world, entity, ecs_id(pulse_window));
        }
    }
}

void mark_window_close_requested(
    pulse_window_plugin_state* state,
    ecs_world_t* world,
    ecs_entity_t entity
) {
    if (ecs_has_id(world, entity, ecs_id(pulse_window))) {
        ecs_add_id(world, entity, PulseWindowCloseRequested);
    }

    if ((state->desc.flags & PULSE_WINDOW_PLUGIN_EXIT_ON_PRIMARY_CLOSE) &&
        ecs_has_id(world, entity, PulsePrimaryWindow)) {
        state->quit_requested = true;
    }
}

void mark_window_resized(
    ecs_world_t* world,
    ecs_entity_t entity,
    int32_t width,
    int32_t height
) {
    pulse_window* window = ecs_get_mut(world, entity, pulse_window);
    if (!window) {
        return;
    }

    window->width = width;
    window->height = height;
    ecs_add_id(world, entity, PulseWindowResized);
    ecs_modified(world, entity, pulse_window);
}

pulse_result_t pulse_window_poll_events(pulse_app_t app, pulse_window_plugin_state* state) {
    ecs_world_t* world = pulse_app_world(app);
    if (!state || !world) {
        return PULSE_ERROR_INVALID_STATE;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            state->quit_requested = true;
            continue;
        }

        const bool is_close_event = event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED;
        const bool is_resize_event =
            event.type == SDL_EVENT_WINDOW_RESIZED ||
            event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED;
        if (!is_close_event && !is_resize_event) {
            continue;
        }

        auto props = SDL_GetWindowProperties(SDL_GetWindowFromEvent(&event));
        ecs_entity_t entity = SDL_GetNumberProperty(props, "sdl.window.entity", 0);
        if (ecs_is_alive(world, entity)) {
            if (is_close_event) {
                mark_window_close_requested(state, world, entity);
            }
            else if (is_resize_event) {
                mark_window_resized(world, entity, event.window.data1, event.window.data2);
            }
        }
    }

    return PULSE_OK;
}

pulse_result_t window_runner(pulse_app_t app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    while (!state->quit_requested) {
        pulse_result_t result = pulse_window_poll_events(app, state);
        if (result != PULSE_OK) {
            return result;
        }

        result = pulse_app_update(app);
        if (result != PULSE_OK) {
            return result;
        }

        SDL_Delay(0);
    }

    return PULSE_OK;
}

pulse_result_t window_plugin_build(pulse_app_t app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    state->app = app;
    register_components(world);
    pulse_window_state_resource resource{};
    resource.state = state;
    ecs_singleton_set_ptr(world, pulse_window_state_resource, &resource);

    const uint32_t init_flags = state->desc.sdl_init_flags ?
        state->desc.sdl_init_flags :
        SDL_INIT_VIDEO;
    const uint32_t already_initialized = SDL_WasInit(init_flags);
    const uint32_t missing_flags = init_flags & ~already_initialized;
    if (missing_flags && !SDL_InitSubSystem(missing_flags)) {
        return PULSE_ERROR_INTERNAL;
    }
    state->initialized_sdl_flags = missing_flags;

    ecs_query_desc_t window_query_desc = {
        .terms = {
            {.id = ecs_id(pulse_window) }
        },
        .cache_kind = EcsQueryCacheAuto
    };

    state->window_query = ecs_query_init(world, &window_query_desc);

    if (state->desc.flags & PULSE_WINDOW_PLUGIN_CREATE_PRIMARY) {
        ecs_entity_t primary = 0;
        pulse_result_t result =
            pulse_window_create(app, &state->desc.primary_window, &primary);
        if (result != PULSE_OK) {
            return result;
        }
    }

    return PULSE_OK;
}

pulse_result_t window_plugin_post_build(pulse_app_t app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    install_window_post_frame_system(world, state);

    if (state->desc.flags & PULSE_WINDOW_PLUGIN_INSTALL_RUNNER) {
        return pulse_app_set_runner(app, window_runner, state);
    }

    return PULSE_OK;
}

void window_plugin_shutdown(pulse_app_t app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return;
    }

    ecs_world_t* world = pulse_app_world(app);
    destroy_all_windows(state);

    if (state->window_query)
        ecs_query_fini(state->window_query);

    if (world && ecs_id(pulse_window_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_window_state_resource);
    }

    if (state->initialized_sdl_flags) {
        SDL_QuitSubSystem(state->initialized_sdl_flags);
        state->initialized_sdl_flags = 0;
    }

    delete state;
}

} // namespace pulse_window_internal

using namespace pulse_window_internal;

extern "C" {

pulse_window_desc pulse_window_desc_default(void) {
    pulse_window_desc desc{};
    desc.struct_size = sizeof(pulse_window_desc);
    desc.title = "Pulse Window";
    desc.width = 1280;
    desc.height = 720;
    desc.flags = PULSE_WINDOW_DEFAULT_FLAGS;
    desc.primary = false;
    return desc;
}

pulse_window_plugin_desc pulse_window_plugin_desc_default(void) {
    pulse_window_plugin_desc desc{};
    desc.struct_size = sizeof(pulse_window_plugin_desc);
    desc.version = PULSE_WINDOW_PLUGIN_DESC_VERSION;
    desc.primary_window = pulse_window_desc_default();
    desc.primary_window.primary = true;
    desc.flags = PULSE_WINDOW_PLUGIN_DEFAULT_FLAGS;
    desc.sdl_init_flags = SDL_INIT_VIDEO;
    return desc;
}

pulse_result_t pulse_window_add_plugin(
    pulse_app_t app,
    const pulse_window_plugin_desc* desc
) {
    if (!app || !validate_plugin_desc(desc)) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_ERROR_DUPLICATE_PLUGIN;
    }

    pulse_window_plugin_state* state = new pulse_window_plugin_state();
    state->desc = normalize_plugin_desc(desc);

    pulse_plugin_desc plugin_desc = {
        sizeof(pulse_plugin_desc),
        PULSE_PLUGIN_DESC_VERSION,
        kPluginName,
        state,
        window_plugin_build,
        window_plugin_post_build,
        window_plugin_shutdown,
    };

    pulse_result_t result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

ecs_entity_t pulse_window_primary(pulse_app_t app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return 0;
    }

    ecs_query_desc_t primary_window_query_desc = {
        .terms = {
            {.id = ecs_id(PulsePrimaryWindow) }
        },
        .cache_kind = EcsQueryCacheAuto
    };

    auto primary_window_query = ecs_query_init(world, &primary_window_query_desc);

    ecs_entity_t finded = 0;
    ecs_iter_t it = ecs_query_iter(world, primary_window_query);
    while (ecs_query_next(&it)) {
        if (it.count > 0) {
            finded = it.entities[0];
            break;
        }
    }

    if (it.flags & EcsIterIsValid) {
        ecs_iter_fini(&it);
    }

    ecs_query_fini(primary_window_query);

    return finded;
}

SDL_Window* pulse_window_get_sdl_window(pulse_app_t app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity)) {
        return nullptr;
    }

    const pulse_sdl_window* raw = ecs_get(world, entity, pulse_sdl_window);
    return raw ? raw->handle : nullptr;
}

void* pulse_window_get_native_view(pulse_app_t app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity)) {
        return nullptr;
    }

    const pulse_sdl_window* raw = ecs_get(world, entity, pulse_sdl_window);
    return raw ? raw->native_view : nullptr;
}

} // extern "C"
