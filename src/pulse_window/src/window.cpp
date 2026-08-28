#include "window_internal.h"

#include "pulse_input.h"

#include <algorithm>
#include <cstring>
#include <vector>

namespace pulse_window_internal {

constexpr const char* kPluginName = "pulse_window";

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

// Helper: get the ECS entity for the window associated with an SDL event.
// Returns 0 if no valid window is found.
ecs_entity_t window_entity_from_event(ecs_world_t* world, const SDL_Event* event) {
    SDL_Window* sdl_window = SDL_GetWindowFromEvent(event);
    if (!sdl_window) return 0;
    SDL_PropertiesID props = SDL_GetWindowProperties(sdl_window);
    ecs_entity_t entity = SDL_GetNumberProperty(props, "sdl.window.entity", 0);
    if (!ecs_is_alive(world, entity)) return 0;
    return entity;
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
    remove_id_from_all_entities(world, PulsePrimaryWindowEntity);
}

void delete_window_components(ecs_world_t* world) {
    delete_entity_if_alive(world, ecs_id(PulseSdlWindow));
    delete_entity_if_alive(world, ecs_id(PulseWindow));
    delete_registered_tag(world, PulseWindowCloseRequested, ecs_id(PulseWindowCloseRequested));
    delete_registered_tag(world, PulseWindowResized, ecs_id(PulseWindowResized));
    delete_registered_tag(world, PulsePrimaryWindowEntity, ecs_id(PulsePrimaryWindowEntity));
    delete_registered_entity(world, ecs_id(PulseTextInputEvent));
    delete_registered_entity(world, ecs_id(PulseWindowFocusEvent));
    delete_registered_entity(world, ecs_id(PulseWindowMouseHoverEvent));
    delete_registered_entity(world, ecs_id(pulse_window_state_resource));

    ecs_id(PulseSdlWindow) = 0;
    ecs_id(PulseWindow) = 0;
    ecs_id(PulseTextInputEvent) = 0;
    ecs_id(PulseWindowFocusEvent) = 0;
    ecs_id(PulseWindowMouseHoverEvent) = 0;
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
        ecs_has_id(world, entity, PulsePrimaryWindowEntity)) {
        pulse_app_finish(state->app);
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

void emit_key_event(
    ecs_world_t* world,
    ecs_entity_t window_entity,
    const SDL_Event& event
) {
    PulseKeyEvent key_evt = {
        .scancode = event.key.scancode,
        .keycode = (int32_t)event.key.key,
        .mod = (uint16_t)event.key.mod,
        .pressed = (event.type == SDL_EVENT_KEY_DOWN),
        .repeat = event.key.repeat,
        .window = window_entity,
    };
    ecs_id_t comp_ids[] = { ecs_id(PulseKeyboardInput) };
    ecs_type_t ids = { .array = comp_ids, .count = 1 };
    ecs_event_desc_t event_desc = {};
    event_desc.event = ecs_id(PulseKeyEvent);
    event_desc.entity = ecs_id(PulseKeyboardInput);
    event_desc.ids = &ids;
    event_desc.const_param = &key_evt;
    event_desc.observable = world;
    ecs_enqueue(world, &event_desc);
}

void emit_mouse_button_event(
    ecs_world_t* world,
    ecs_entity_t window_entity,
    const SDL_Event& event
) {
    PulseMouseButtonEvent btn_evt = {
        .button = event.button.button,
        .pressed = (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN),
        .x = event.button.x,
        .y = event.button.y,
        .is_touch = (event.button.which == SDL_TOUCH_MOUSEID),
        .window = window_entity,
    };
    ecs_id_t comp_ids[] = { ecs_id(PulseMouseInput) };
    ecs_type_t ids = { .array = comp_ids, .count = 1 };
    ecs_event_desc_t event_desc = {};
    event_desc.event = ecs_id(PulseMouseButtonEvent);
    event_desc.entity = ecs_id(PulseMouseInput);
    event_desc.ids = &ids;
    event_desc.const_param = &btn_evt;
    event_desc.observable = world;
    ecs_enqueue(world, &event_desc);
}

void emit_mouse_scroll_event(
    ecs_world_t* world,
    ecs_entity_t window_entity,
    const SDL_Event& event
) {
    PulseMouseScrollEvent scroll_evt = {
        .x = event.wheel.x,
        .y = event.wheel.y,
        .is_touch = (event.wheel.which == SDL_TOUCH_MOUSEID),
        .window = window_entity,
    };
    ecs_id_t comp_ids[] = { ecs_id(PulseMouseScroll) };
    ecs_type_t ids = { .array = comp_ids, .count = 1 };
    ecs_event_desc_t event_desc = {};
    event_desc.event = ecs_id(PulseMouseScrollEvent);
    event_desc.entity = ecs_id(PulseMouseScroll);
    event_desc.ids = &ids;
    event_desc.const_param = &scroll_evt;
    event_desc.observable = world;
    ecs_enqueue(world, &event_desc);
}

void emit_text_input_event(
    ecs_world_t* world,
    ecs_entity_t window_entity,
    const SDL_Event& event
) {
    PulseTextInputEvent text_evt = {};
    strncpy(
        text_evt.text,
        event.text.text,
        sizeof(text_evt.text) - 1
    );
    text_evt.text[sizeof(text_evt.text) - 1] = '\0';
    text_evt.window = window_entity;
    ecs_id_t comp_ids[] = { ecs_id(PulseWindow) };
    ecs_type_t ids = { .array = comp_ids, .count = 1 };
    ecs_event_desc_t event_desc = {};
    event_desc.event = ecs_id(PulseTextInputEvent);
    event_desc.entity = window_entity;
    event_desc.ids = &ids;
    event_desc.const_param = &text_evt;
    event_desc.observable = world;
    ecs_enqueue(world, &event_desc);
}

void emit_window_focus_event(
    ecs_world_t* world,
    ecs_entity_t window_entity,
    const SDL_Event& event
) {
    PulseWindowFocusEvent focus_evt = {
        .focused = (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED),
        .window = window_entity,
    };
    ecs_id_t comp_ids[] = { ecs_id(PulseWindow) };
    ecs_type_t ids = { .array = comp_ids, .count = 1 };
    ecs_event_desc_t event_desc = {};
    event_desc.event = ecs_id(PulseWindowFocusEvent);
    event_desc.entity = window_entity;
    event_desc.ids = &ids;
    event_desc.const_param = &focus_evt;
    event_desc.observable = world;
    ecs_enqueue(world, &event_desc);
}

void emit_window_mouse_hover_event(
    ecs_world_t* world,
    ecs_entity_t window_entity,
    const SDL_Event& event
) {
    PulseWindowMouseHoverEvent hover_evt = {
        .entered = (event.type == SDL_EVENT_WINDOW_MOUSE_ENTER),
        .window = window_entity,
    };
    ecs_id_t comp_ids[] = { ecs_id(PulseWindow) };
    ecs_type_t ids = { .array = comp_ids, .count = 1 };
    ecs_event_desc_t event_desc = {};
    event_desc.event = ecs_id(PulseWindowMouseHoverEvent);
    event_desc.entity = window_entity;
    event_desc.ids = &ids;
    event_desc.const_param = &hover_evt;
    event_desc.observable = world;
    ecs_enqueue(world, &event_desc);
}

EPulseResult pulse_window_poll_events(PulseAppId app, pulse_window_plugin_state* state) {
    ecs_world_t* world = pulse_app_world(app);
    if (!state || !world) {
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            pulse_app_finish(app);
            continue;
        }

        // --- Window events (close, resize) ---
        ecs_entity_t window_entity = window_entity_from_event(world, &event);
        if (window_entity) {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                mark_window_close_requested(state, world, window_entity);
            }
            else if (event.type == SDL_EVENT_WINDOW_RESIZED || event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED) {
                mark_window_resized(world, window_entity, event.window.data1, event.window.data2);
            }
            // --- Keyboard events: emit PulseKeyEvent with window info ---
            else if (event.type == SDL_EVENT_KEY_DOWN || event.type == SDL_EVENT_KEY_UP) {
                emit_key_event(world, window_entity, event);
            }
            // --- Mouse button events: emit PulseMouseButtonEvent with window info ---
            else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN || event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                emit_mouse_button_event(world, window_entity, event);
            }
            // --- Mouse wheel: emit PulseMouseScrollEvent with window info ---
            else if (event.type == SDL_EVENT_MOUSE_WHEEL) {
                emit_mouse_scroll_event(world, window_entity, event);
            }
            // --- Text input: emit PulseTextInputEvent with window info ---
            else if (event.type == SDL_EVENT_TEXT_INPUT) {
                emit_text_input_event(world, window_entity, event);
            }
            // --- Window focus: emit PulseWindowFocusEvent with window info ---
            else if (event.type == SDL_EVENT_WINDOW_FOCUS_GAINED ||
                     event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
                emit_window_focus_event(world, window_entity, event);
            }
            else if (event.type == SDL_EVENT_WINDOW_MOUSE_ENTER ||
                     event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
                emit_window_mouse_hover_event(world, window_entity, event);
            }
        }
    }

    return PULSE_RESULT_OK;
}

EPulseRunnerResult window_runner(PulseAppId app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_RUNNER_RESULT_ERROR_INVALID_ARGUMENT;
    }

    while (!pulse_app_should_quit(app)) {
        EPulseResult poll_result = pulse_window_poll_events(app, state);
        if (poll_result != PULSE_RESULT_OK) {
            switch (poll_result) {
                case PULSE_RESULT_ERROR_INVALID_ARGUMENT: return PULSE_RUNNER_RESULT_ERROR_INVALID_ARGUMENT;
                case PULSE_RESULT_ERROR_INVALID_STATE: return PULSE_RUNNER_RESULT_ERROR_INVALID_STATE;
                default: return PULSE_RUNNER_RESULT_ERROR_INTERNAL;
            }
        }

        EPulseAppUpdateResult update_result = pulse_app_update(app);
        if (update_result != PULSE_APP_UPDATE_RESULT_OK) {
            return static_cast<EPulseRunnerResult>(update_result);
        }

        SDL_Delay(0);
    }

    return PULSE_RUNNER_RESULT_OK;
}

EPulsePluginBuildResult window_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
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
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }
    state->initialized_sdl_flags = missing_flags;

    if (state->desc.flags & PULSE_WINDOW_PLUGIN_CREATE_PRIMARY) {
        ecs_entity_t primary = 0;
        EPulseResult result =
            pulse_window_create(app, &state->desc.primary_window, &primary);
        if (result != PULSE_RESULT_OK) {
            return result == PULSE_RESULT_ERROR_INVALID_ARGUMENT
                ? PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT
                : result == PULSE_RESULT_ERROR_INVALID_STATE
                    ? PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE
                    : PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
        }
        ecs_add_id(world, primary, PulsePrimaryWindowEntity);
    }

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult window_plugin_post_build(PulseAppId app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    install_window_post_frame_system(world, state);

    if (state->desc.flags & PULSE_WINDOW_PLUGIN_INSTALL_RUNNER) {
        EPulseAppSetRunnerResult result = pulse_app_set_runner(app, window_runner, state);
        return result == PULSE_APP_SET_RUNNER_RESULT_OK
            ? PULSE_PLUGIN_BUILD_RESULT_OK
            : result == PULSE_APP_SET_RUNNER_RESULT_ERROR_INVALID_STATE
                ? PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE
                : PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }

    return PULSE_PLUGIN_BUILD_RESULT_OK;
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

    if (state->owned_title) {
        ecs_os_free(state->owned_title);
        state->owned_title = nullptr;
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

EPulseAppAddPluginResult pulse_add_window_plugin(
    PulseAppId app,
    const PulseWindowPluginDesc* desc
) {
    if (!app || !validate_plugin_desc(desc)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    pulse_window_plugin_state* state = new pulse_window_plugin_state();
    state->desc = normalize_plugin_desc(desc);
    if (state->desc.primary_window.title) {
        state->owned_title = ecs_os_strdup(state->desc.primary_window.title);
        state->desc.primary_window.title = state->owned_title;
    }

    const char* window_dependencies[] = { "pulse_input" };
    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_WINDOW_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = state,
        .build = window_plugin_build,
        .post_build = window_plugin_post_build,
        .shutdown = window_plugin_shutdown,
        .dependency_count = 1,
        .dependencies = window_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
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
            {.id = ecs_id(PulsePrimaryWindowEntity) }
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

EPulseResult pulse_window_set_title(PulseAppId app, ecs_entity_t entity, const char* title) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) || !title) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Require the component to already exist: ecs_get_mut would otherwise
    // add it, which via EcsWith + the on_set hook would create a window.
    if (!ecs_has_id(world, entity, ecs_id(PulseWindow))) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    PulseWindow* window = ecs_get_mut(world, entity, PulseWindow);
    if (!window) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // Setting the same title again is a no-op (no allocation, no sync).
    if (window->title && std::strcmp(window->title, title) == 0) {
        return PULSE_RESULT_OK;
    }

    // The component owns the string; see the ownership contract in
    // pulse_window.h. The new value is applied to the OS window by the
    // post-frame sync system.
    ecs_os_free(const_cast<char*>(window->title));
    window->title = ecs_os_strdup(title);
    ecs_modified(world, entity, PulseWindow);

    return PULSE_RESULT_OK;
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
