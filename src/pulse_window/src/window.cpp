#include "pulse_window.h"

#include <SDL3/SDL.h>

#include <algorithm>

ECS_COMPONENT_DECLARE(pulse_window);
ECS_COMPONENT_DECLARE(pulse_sdl_window);
ECS_TAG_DECLARE(PulsePrimaryWindow);
ECS_TAG_DECLARE(PulseWindowCloseRequested);
ECS_TAG_DECLARE(PulseWindowResized);

namespace {

constexpr const char* kPluginName = "PulseWindowPlugin";

struct pulse_window_plugin_state {
    pulse_app_t app = nullptr;
    pulse_window_plugin_desc desc{};
    uint32_t initialized_sdl_flags = 0;
    bool quit_requested = false;
    ecs_query_t* window_query{};
    ecs_entity_t post_frame_system = 0;
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

SDL_Window* create_sdl_window(const char* title, float width, float height, uint32_t flags)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetBooleanProperty(
        props,
        SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,
        (flags & PULSE_WINDOW_FLAG_RESIZABLE) != 0
    );
    SDL_SetBooleanProperty(
        props,
        SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN,
        (flags & PULSE_WINDOW_FLAG_EXTERNAL_GRAPHICS_CONTEXT) != 0
    );

    SDL_Window* sdl_window = SDL_CreateWindowWithProperties(props);
    SDL_DestroyProperties(props);
    if (!sdl_window) {
        return nullptr;
    }

    return sdl_window;
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

void pulse_sdl_window_release(
    pulse_sdl_window* ptr)
{
    if (ptr->handle)
        SDL_DestroyWindow(ptr->handle);
    ptr->handle = nullptr;
    ptr->native_view = nullptr;
    ptr->window_id = 0;
}

ECS_DTOR(pulse_sdl_window, ptr, {
    pulse_sdl_window_release(ptr);
    })

ECS_MOVE(pulse_sdl_window, dst, src, {
    pulse_sdl_window_release(dst);
    *dst = *src;
    ecs_os_zeromem(src);
    })

void on_window_set(ecs_iter_t* it)
{
    ecs_world_t* world = it->world;

    pulse_window* windows = ecs_field(it, pulse_window, 0);
    for (int i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
		auto& window = windows[i];

		if (!ecs_has_id(world, entity, ecs_id(pulse_sdl_window))) {
			ecs_add_id(world, entity, ecs_id(pulse_sdl_window));
		}

        pulse_sdl_window* sdl_window = ecs_get_mut(world, entity, pulse_sdl_window);
        bool sdl_window_changed = false;

		if (sdl_window->handle == nullptr) {
			SDL_Window* raw_window = create_sdl_window(
				window.title,
				window.width,
				window.height,
				window.flags
			);
			if (raw_window) {
				sdl_window->handle = raw_window;
				sdl_window->window_id = SDL_GetWindowID(raw_window);
				sdl_window->native_view = native_view_from_window(raw_window);
                sdl_window_changed = true;
				SDL_PropertiesID props = SDL_GetWindowProperties(raw_window);
				SDL_SetNumberProperty(props, "sdl.window.entity", entity);

                int width = 0;
                int height = 0;
                SDL_GetWindowSize(raw_window, &width, &height);

				window.width = width;
				window.height = height;
			}
		}

        if (sdl_window_changed) {
            ecs_modified(world, entity, pulse_sdl_window);
        }
    }
}

void on_window_remove(ecs_iter_t* it)
{
    ecs_world_t* world = it->world;

    for (int i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_has_id(world, entity, ecs_id(pulse_sdl_window))) {
            ecs_remove_id(world, entity, ecs_id(pulse_sdl_window));
        }
        if (ecs_has_id(world, entity, PulseWindowCloseRequested)) {
            ecs_remove_id(world, entity, PulseWindowCloseRequested);
        }
        if (ecs_has_id(world, entity, PulseWindowResized)) {
            ecs_remove_id(world, entity, PulseWindowResized);
        }
    }
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

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, pulse_window);
    ECS_COMPONENT_DEFINE(world, pulse_sdl_window);
    ECS_COMPONENT_DEFINE(world, pulse_window_state_resource);
    ecs_add_pair(world, ecs_id(pulse_window), EcsWith, ecs_id(pulse_sdl_window));

    ecs_type_hooks_t pulse_sdl_window_hooks = {
        .ctor = flecs_default_ctor,
        .dtor = ecs_dtor(pulse_sdl_window),
        .move = ecs_move(pulse_sdl_window),
    };
    ecs_set_hooks_id(world, ecs_id(pulse_sdl_window), &pulse_sdl_window_hooks);

	ecs_type_hooks_t pulse_window_hooks = {
        .ctor = flecs_default_ctor,
        .on_set = on_window_set,
        .on_remove = on_window_remove,
    };
    ecs_set_hooks_id(world, ecs_id(pulse_window), &pulse_window_hooks);

    ECS_TAG_DEFINE(world, PulsePrimaryWindow);
    ECS_TAG_DEFINE(world, PulseWindowCloseRequested);
    ECS_TAG_DEFINE(world, PulseWindowResized);
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

void window_post_frame_system_run(ecs_iter_t* it) {
    pulse_window* windows = ecs_field(it, pulse_window, 0);

    for (int i = 0; i < it->count; i++) {
        ecs_entity_t entity = it->entities[i];
        pulse_window& window = windows[i];

        const pulse_sdl_window* sdl_window =
            ecs_get(it->world, entity, pulse_sdl_window);
        if (sdl_window && sdl_window->handle && window.width > 0 && window.height > 0) {
            int sdl_width = 0;
            int sdl_height = 0;
            SDL_GetWindowSize(sdl_window->handle, &sdl_width, &sdl_height);
            if (window.width != sdl_width || window.height != sdl_height) {
                SDL_SetWindowSize(sdl_window->handle, window.width, window.height);
            }
        }

        if (ecs_has_id(it->world, entity, PulseWindowResized)) {
            ecs_remove_id(it->world, entity, PulseWindowResized);
        }
    }
}

ecs_entity_t install_window_post_frame_system(
    ecs_world_t* world,
    pulse_window_plugin_state* state
) {
    if (!world || !state || state->post_frame_system) {
        return state ? state->post_frame_system : 0;
    }

    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "PulseWindowPostFrameSystem";
    ecs_entity_t system_entity = ecs_entity_init(world, &entity_desc);

    ecs_system_desc_t system_desc{};
    system_desc.entity = system_entity;
    system_desc.phase = EcsPostFrame;
    system_desc.query.terms[0].id = ecs_id(pulse_window);
    system_desc.query.cache_kind = EcsQueryCacheAuto;
    system_desc.callback = window_post_frame_system_run;
    system_desc.ctx = state;
    system_desc.immediate = true;

    state->post_frame_system = ecs_system_init(world, &system_desc);
    return state->post_frame_system;
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
