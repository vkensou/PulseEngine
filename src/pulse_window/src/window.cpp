#include "pulse_window.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <string>
#include <vector>

ECS_COMPONENT_DECLARE(pulse_window);
ECS_COMPONENT_DECLARE(pulse_sdl_window);
ECS_TAG_DECLARE(PulsePrimaryWindow);

namespace {

constexpr const char* kPluginName = "PulseWindowPlugin";

struct WindowRecord {
    ecs_entity_t entity = 0;
    SDL_Window* window = nullptr;
    SDL_WindowID window_id = 0;
};

struct pulse_window_plugin_state {
    pulse_app_t app = nullptr;
    pulse_window_plugin_desc desc{};
    uint32_t initialized_sdl_flags = 0;
    bool quit_requested = false;
    std::vector<WindowRecord> windows;
};

typedef struct pulse_window_state_resource {
    pulse_window_plugin_state* state;
} pulse_window_state_resource;

ECS_COMPONENT_DECLARE(pulse_window_state_resource);

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

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, pulse_window);
    ECS_COMPONENT_DEFINE(world, pulse_sdl_window);
    ECS_COMPONENT_DEFINE(world, pulse_window_state_resource);
    ECS_TAG_DEFINE(world, PulsePrimaryWindow);
}

pulse_window_plugin_state* state_from_world(ecs_world_t* world) {
    if (!world || ecs_id(pulse_window_state_resource) == 0) {
        return nullptr;
    }

    const pulse_window_state_resource* resource =
        ecs_singleton_get(world, pulse_window_state_resource);
    return resource ? resource->state : nullptr;
}

pulse_window_plugin_state* state_from_app(pulse_app_t app) {
    return state_from_world(pulse_app_world(app));
}

auto find_record_by_entity(pulse_window_plugin_state* state, ecs_entity_t entity) {
    return std::find_if(
        state->windows.begin(),
        state->windows.end(),
        [entity](const WindowRecord& record) { return record.entity == entity; }
    );
}

auto find_record_by_window_id(pulse_window_plugin_state* state, SDL_WindowID window_id) {
    return std::find_if(
        state->windows.begin(),
        state->windows.end(),
        [window_id](const WindowRecord& record) { return record.window_id == window_id; }
    );
}

void* native_view_from_window(SDL_Window* window) {
    if (!window) {
        return nullptr;
    }

    SDL_PropertiesID props = SDL_GetWindowProperties(window);
#ifdef _WIN32
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
#elif defined(__ANDROID__)
    return SDL_GetPointerProperty(props, SDL_PROP_WINDOW_ANDROID_WINDOW_POINTER, nullptr);
#else
    return nullptr;
#endif
}

bool destroy_sdl_window(pulse_window_plugin_state* state, ecs_entity_t entity) {
    if (!state) {
        return false;
    }

    auto it = find_record_by_entity(state, entity);
    if (it == state->windows.end()) {
        return false;
    }

    if (it->window) {
        SDL_DestroyWindow(it->window);
    }

    state->windows.erase(it);
    return true;
}

void destroy_all_windows(pulse_window_plugin_state* state) {
    if (!state) {
        return;
    }

    for (auto it = state->windows.rbegin(); it != state->windows.rend(); ++it) {
        if (it->window) {
            SDL_DestroyWindow(it->window);
        }
    }
    state->windows.clear();
}

void clear_frame_state(pulse_window_plugin_state* state) {
    ecs_world_t* world = pulse_app_world(state->app);
    if (!world) {
        return;
    }

    for (const WindowRecord& record : state->windows) {
        if (!ecs_is_alive(world, record.entity)) {
            continue;
        }

        pulse_window* window = ecs_get_mut(world, record.entity, pulse_window);
        if (!window) {
            continue;
        }

        window->resized = false;
        ecs_modified(world, record.entity, pulse_window);
    }
}

void mark_window_close_requested(
    pulse_window_plugin_state* state,
    ecs_world_t* world,
    ecs_entity_t entity
) {
    pulse_window* window = ecs_get_mut(world, entity, pulse_window);
    if (window) {
        window->close_requested = true;
        ecs_modified(world, entity, pulse_window);
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
    window->resized = true;
    ecs_modified(world, entity, pulse_window);
}

pulse_result_t window_runner(pulse_app_t app, void* ctx) {
    pulse_window_plugin_state* state = static_cast<pulse_window_plugin_state*>(ctx);
    if (!state) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    while (!state->quit_requested) {
        pulse_result_t result = pulse_window_poll_events(app);
        if (result != PULSE_OK) {
            return result;
        }

        result = pulse_app_update(app);
        if (result != PULSE_OK) {
            return result;
        }

        result = pulse_window_sync(app);
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

    if (world && ecs_id(pulse_window_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_window_state_resource);
    }

    if (state->initialized_sdl_flags) {
        SDL_QuitSubSystem(state->initialized_sdl_flags);
        state->initialized_sdl_flags = 0;
    }

    delete state;
}

} // namespace

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

pulse_result_t pulse_window_create(
    pulse_app_t app,
    const pulse_window_desc* desc,
    ecs_entity_t* out_entity
) {
    if (!app || !validate_window_desc(desc)) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    ecs_world_t* world = pulse_app_world(app);
    pulse_window_plugin_state* state = state_from_app(app);
    if (!world || !state) {
        return PULSE_ERROR_INVALID_STATE;
    }

    pulse_window_desc normalized = normalize_window_desc(desc);
    if (normalized.primary && pulse_window_primary(app) != 0) {
        return PULSE_ERROR_INVALID_STATE;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, normalized.title);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, (float)normalized.width);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, (float)normalized.height);
    SDL_SetBooleanProperty(
        props,
        SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,
        (normalized.flags & PULSE_WINDOW_FLAG_RESIZABLE) != 0
    );
    SDL_SetBooleanProperty(
        props,
        SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN,
        (normalized.flags & PULSE_WINDOW_FLAG_EXTERNAL_GRAPHICS_CONTEXT) != 0
    );

    SDL_Window* sdl_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!sdl_window) {
        return PULSE_ERROR_INTERNAL;
    }

    int width = 0;
    int height = 0;
    SDL_GetWindowSize(sdl_window, &width, &height);

    ecs_entity_desc_t entity_desc{};
    entity_desc.name = normalized.title;
    ecs_entity_t entity = ecs_entity_init(world, &entity_desc);
    if (!entity) {
        SDL_DestroyWindow(sdl_window);
        return PULSE_ERROR_INTERNAL;
    }

    pulse_window window_component{};
    window_component.width = width;
    window_component.height = height;
    ecs_set_ptr(world, entity, pulse_window, &window_component);

    pulse_sdl_window raw_component{};
    raw_component.handle = sdl_window;
    raw_component.window_id = SDL_GetWindowID(sdl_window);
    raw_component.native_view = native_view_from_window(sdl_window);
    ecs_set_ptr(world, entity, pulse_sdl_window, &raw_component);

    if (normalized.primary) {
        ecs_add_id(world, entity, PulsePrimaryWindow);
    }

    state->windows.push_back({ entity, sdl_window, raw_component.window_id });

    if (out_entity) {
        *out_entity = entity;
    }

    return PULSE_OK;
}

pulse_result_t pulse_window_destroy(pulse_app_t app, ecs_entity_t entity) {
    if (!app || !entity) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    ecs_world_t* world = pulse_app_world(app);
    pulse_window_plugin_state* state = state_from_app(app);
    if (!world || !state) {
        return PULSE_ERROR_INVALID_STATE;
    }

    if (!ecs_is_alive(world, entity)) {
        return PULSE_ERROR_NOT_FOUND;
    }

    if (!destroy_sdl_window(state, entity)) {
        const pulse_sdl_window* raw = ecs_get(world, entity, pulse_sdl_window);
        if (raw && raw->handle) {
            SDL_DestroyWindow(raw->handle);
        }
    }

    ecs_delete(world, entity);
    return PULSE_OK;
}

pulse_result_t pulse_window_poll_events(pulse_app_t app) {
    pulse_window_plugin_state* state = state_from_app(app);
    ecs_world_t* world = pulse_app_world(app);
    if (!state || !world) {
        return PULSE_ERROR_INVALID_STATE;
    }

    clear_frame_state(state);

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

        auto record = find_record_by_window_id(state, event.window.windowID);
        if (record == state->windows.end() || !ecs_is_alive(world, record->entity)) {
            continue;
        }

        if (is_close_event) {
            mark_window_close_requested(state, world, record->entity);
        } else if (is_resize_event) {
            mark_window_resized(world, record->entity, event.window.data1, event.window.data2);
        }
    }

    return PULSE_OK;
}

pulse_result_t pulse_window_sync(pulse_app_t app) {
    pulse_window_plugin_state* state = state_from_app(app);
    ecs_world_t* world = pulse_app_world(app);
    if (!state || !world) {
        return PULSE_ERROR_INVALID_STATE;
    }

    for (const WindowRecord& record : state->windows) {
        if (!record.window || !ecs_is_alive(world, record.entity)) {
            continue;
        }

        const pulse_window* window = ecs_get(world, record.entity, pulse_window);
        if (!window || window->width <= 0 || window->height <= 0) {
            continue;
        }

        int sdl_width = 0;
        int sdl_height = 0;
        SDL_GetWindowSize(record.window, &sdl_width, &sdl_height);
        if (window->width != sdl_width || window->height != sdl_height) {
            SDL_SetWindowSize(record.window, window->width, window->height);
        }
    }

    return PULSE_OK;
}

pulse_result_t pulse_window_request_close(pulse_app_t app) {
    pulse_window_plugin_state* state = state_from_app(app);
    if (!state) {
        return PULSE_ERROR_INVALID_STATE;
    }

    state->quit_requested = true;
    return PULSE_OK;
}

bool pulse_window_should_close(pulse_app_t app) {
    pulse_window_plugin_state* state = state_from_app(app);
    return state ? state->quit_requested : true;
}

ecs_entity_t pulse_window_primary(pulse_app_t app) {
    pulse_window_plugin_state* state = state_from_app(app);
    ecs_world_t* world = pulse_app_world(app);
    if (!state || !world) {
        return 0;
    }

    for (const WindowRecord& record : state->windows) {
        if (ecs_is_alive(world, record.entity) &&
            ecs_has_id(world, record.entity, PulsePrimaryWindow)) {
            return record.entity;
        }
    }

    return 0;
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
