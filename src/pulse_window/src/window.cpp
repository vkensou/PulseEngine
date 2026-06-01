#include "window_internal.h"

#include <algorithm>
#include <vector>

namespace pulse_window_internal {

constexpr const char* kPluginName = "PulseWindowPlugin";

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

void delete_registered_tag(
    ecs_world_t* world,
    ecs_entity_t& tag,
    ecs_entity_t& tag_id
) {
    ecs_entity_t entity = tag ? tag : tag_id;
    if (world && entity && ecs_is_alive(world, entity)) {
        ecs_delete(world, entity);
    }
    tag = 0;
    tag_id = 0;
}

void remove_id_from_all_entities(ecs_world_t* world, ecs_entity_t id) {
    if (!world || id == 0) {
        return;
    }

    ecs_query_desc_t query_desc{};
    query_desc.terms[0].id = id;
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
            ecs_remove_id(world, entity, id);
        }
    }
}

} // namespace

PulseWindowDesc normalize_window_desc(const PulseWindowDesc* desc) {
    PulseWindowDesc normalized = pulse_window_desc_default();
    if (desc) {
        normalized = *desc;
    }

    normalized.struct_size = sizeof(PulseWindowDesc);
    if (!normalized.title) {
        normalized.title = "Pulse Window";
    }
    if (normalized.width <= 0) {
        normalized.width = 1280;
    }
    if (normalized.height <= 0) {
        normalized.height = 720;
    }

    return normalized;
}

PulseWindowPluginDesc normalize_plugin_desc(const PulseWindowPluginDesc* desc) {
    PulseWindowPluginDesc normalized = pulse_window_plugin_desc_default();
    if (desc) {
        normalized = *desc;
    }

    normalized.struct_size = sizeof(PulseWindowPluginDesc);
    normalized.version = PULSE_WINDOW_PLUGIN_DESC_VERSION;
    if (normalized.sdl_init_flags == 0) {
        normalized.sdl_init_flags = SDL_INIT_VIDEO;
    }
    normalized.primary_window = normalize_window_desc(
        normalized.primary_window.struct_size == sizeof(PulseWindowDesc)
            ? &normalized.primary_window
            : nullptr
    );

    return normalized;
}

bool validate_window_desc(const PulseWindowDesc* desc) {
    return desc && desc->struct_size == sizeof(PulseWindowDesc);
}

bool validate_plugin_desc(const PulseWindowPluginDesc* desc) {
    return !desc ||
        (desc->struct_size == sizeof(PulseWindowPluginDesc) &&
         desc->version == PULSE_WINDOW_PLUGIN_DESC_VERSION);
}

EPulseResult pulse_window_create(
    PulseAppId app,
    const PulseWindowDesc* desc,
    ecs_entity_t* out_entity
) {
    if (!app || !validate_window_desc(desc)) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    PulseWindowDesc normalized = normalize_window_desc(desc);

    ecs_entity_desc_t entity_desc{};
    entity_desc.name = normalized.title;
    ecs_entity_t entity = ecs_entity_init(world, &entity_desc);
    if (!entity) {
        return PULSE_RESULT_ERROR_INTERNAL;
    }

    PulseWindow window_component{};
    window_component.title = normalized.title;
    window_component.width = normalized.width;
    window_component.height = normalized.height;
    window_component.resizable = normalized.resizable;
    window_component.external_graphics_context = normalized.external_graphics_context;
    ecs_set_ptr(world, entity, PulseWindow, &window_component);

    if (out_entity) {
        *out_entity = entity;
    }

    return PULSE_RESULT_OK;
}

void remove_window_components(pulse_window_plugin_state* state) {
    if (!state) {
        return;
    }

    ecs_world_t* world = pulse_app_world(state->app);
    if (!world) {
        return;
    }

    remove_id_from_all_entities(world, ecs_id(PulseSdlWindow));
    remove_id_from_all_entities(world, ecs_id(PulseWindow));
    remove_id_from_all_entities(world, PulseWindowCloseRequested);
    remove_id_from_all_entities(world, PulseWindowResized);
    remove_id_from_all_entities(world, PulsePrimaryWindow);
}

void delete_window_components(ecs_world_t* world) {
    delete_entity_if_alive(world, ecs_id(PulseSdlWindow));
    delete_entity_if_alive(world, ecs_id(PulseWindow));
    delete_registered_tag(world, PulseWindowCloseRequested, ecs_id(PulseWindowCloseRequested));
    delete_registered_tag(world, PulseWindowResized, ecs_id(PulseWindowResized));
    delete_registered_tag(world, PulsePrimaryWindow, ecs_id(PulsePrimaryWindow));
    delete_registered_entity(world, ecs_id(pulse_window_state_resource));

    ecs_id(PulseSdlWindow) = 0;
    ecs_id(PulseWindow) = 0;
}

void mark_window_close_requested(
    pulse_window_plugin_state* state,
    ecs_world_t* world,
    ecs_entity_t entity
) {
    if (ecs_has_id(world, entity, ecs_id(PulseWindow))) {
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
    PulseWindow* window = ecs_get_mut(world, entity, PulseWindow);
    if (!window) {
        return;
    }

    window->width = width;
    window->height = height;
    ecs_add_id(world, entity, PulseWindowResized);
    ecs_modified(world, entity, PulseWindow);
}

EPulseResult pulse_window_poll_events(PulseAppId app, pulse_window_plugin_state* state) {
    ecs_world_t* world = pulse_app_world(app);
    if (!state || !world) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
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

    return PULSE_RESULT_OK;
}

EPulseResult window_runner(PulseAppId app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    while (!state->quit_requested) {
        EPulseResult result = pulse_window_poll_events(app, state);
        if (result != PULSE_RESULT_OK) {
            return result;
        }

        result = pulse_app_update(app);
        if (result != PULSE_RESULT_OK) {
            return result;
        }

        SDL_Delay(0);
    }

    return PULSE_RESULT_OK;
}

EPulseResult window_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
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
        return PULSE_RESULT_ERROR_INTERNAL;
    }
    state->initialized_sdl_flags = missing_flags;

    if (state->desc.flags & PULSE_WINDOW_PLUGIN_CREATE_PRIMARY) {
        ecs_entity_t primary = 0;
        EPulseResult result =
            pulse_window_create(app, &state->desc.primary_window, &primary);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
        ecs_add_id(world, primary, PulsePrimaryWindow);
    }

    return PULSE_RESULT_OK;
}

EPulseResult window_plugin_post_build(PulseAppId app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    install_window_post_frame_system(world, state);

    if (state->desc.flags & PULSE_WINDOW_PLUGIN_INSTALL_RUNNER) {
        return pulse_app_set_runner(app, window_runner, state);
    }

    return PULSE_RESULT_OK;
}

void window_plugin_shutdown(PulseAppId app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return;
    }

    ecs_world_t* world = pulse_app_world(app);
    delete_registered_entity(world, state->post_frame_system);
    remove_window_components(state);

    if (world && ecs_id(pulse_window_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_window_state_resource);
    }

    delete_window_components(world);

    if (state->initialized_sdl_flags) {
        SDL_QuitSubSystem(state->initialized_sdl_flags);
        state->initialized_sdl_flags = 0;
    }

    delete state;
}

} // namespace pulse_window_internal

using namespace pulse_window_internal;

extern "C" {

PulseWindowDesc pulse_window_desc_default(void) {
    PulseWindowDesc desc{};
    desc.struct_size = sizeof(PulseWindowDesc);
    desc.title = "Pulse Window";
    desc.width = 1280;
    desc.height = 720;
    desc.resizable = true;
    desc.external_graphics_context = true;
    return desc;
}

PulseWindowPluginDesc pulse_window_plugin_desc_default(void) {
    PulseWindowPluginDesc desc{};
    desc.struct_size = sizeof(PulseWindowPluginDesc);
    desc.version = PULSE_WINDOW_PLUGIN_DESC_VERSION;
    desc.primary_window = pulse_window_desc_default();
    desc.flags = PULSE_WINDOW_PLUGIN_DEFAULT;
    desc.sdl_init_flags = SDL_INIT_VIDEO;
    return desc;
}

EPulseResult pulse_add_window_plugin(
    PulseAppId app,
    const PulseWindowPluginDesc* desc
) {
    if (!app || !validate_plugin_desc(desc)) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    pulse_window_plugin_state* state = new pulse_window_plugin_state();
    state->desc = normalize_plugin_desc(desc);

    PulsePluginDesc plugin_desc = {
        sizeof(PulsePluginDesc),
        PULSE_PLUGIN_DESC_VERSION,
        kPluginName,
        state,
        window_plugin_build,
        window_plugin_post_build,
        window_plugin_shutdown,
    };

    EPulseResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

ecs_entity_t pulse_window_get_primary(PulseAppId app) {
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

SDL_Window* pulse_window_get_sdl_window(PulseAppId app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity)) {
        return nullptr;
    }

    const PulseSdlWindow* raw = ecs_get(world, entity, PulseSdlWindow);
    return raw ? raw->handle : nullptr;
}

void* pulse_window_get_native_view(PulseAppId app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity)) {
        return nullptr;
    }

    const PulseSdlWindow* raw = ecs_get(world, entity, PulseSdlWindow);
    return raw ? raw->native_view : nullptr;
}

} // extern "C"
