#include "window_internal.h"

ECS_COMPONENT_DECLARE(PulseWindow);
ECS_COMPONENT_DECLARE(PulseSdlWindow);
ECS_TAG_DECLARE(PulsePrimaryWindow);
ECS_TAG_DECLARE(PulseWindowCloseRequested);
ECS_TAG_DECLARE(PulseWindowResized);
ECS_COMPONENT_DECLARE(PulseTextInputEvent);
ECS_COMPONENT_DECLARE(PulseWindowFocusEvent);
ECS_COMPONENT_DECLARE(PulseWindowMouseHoverEvent);

namespace pulse_window_internal {

ECS_COMPONENT_DECLARE(pulse_window_state_resource);

namespace {

SDL_Window* create_sdl_window(const char* title, float width, float height, bool resizable, bool external_graphics_context)
{
    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetStringProperty(props, SDL_PROP_WINDOW_CREATE_TITLE_STRING, title);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_X_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetNumberProperty(props, SDL_PROP_WINDOW_CREATE_Y_NUMBER, SDL_WINDOWPOS_CENTERED);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_WIDTH_NUMBER, width);
    SDL_SetFloatProperty(props, SDL_PROP_WINDOW_CREATE_HEIGHT_NUMBER, height);
    SDL_SetBooleanProperty(
        props,
        SDL_PROP_WINDOW_CREATE_RESIZABLE_BOOLEAN,
        resizable
    );
    SDL_SetBooleanProperty(
        props,
        SDL_PROP_WINDOW_CREATE_EXTERNAL_GRAPHICS_CONTEXT_BOOLEAN,
        external_graphics_context
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
    PulseSdlWindow* ptr)
{
    if (ptr->handle)
        SDL_DestroyWindow(ptr->handle);
    ptr->handle = nullptr;
    ptr->native_view = nullptr;
    ptr->window_id = 0;
}

ECS_DTOR(PulseSdlWindow, ptr, {
    pulse_sdl_window_release(ptr);
    })

ECS_MOVE(PulseSdlWindow, dst, src, {
    pulse_sdl_window_release(dst);
    *dst = *src;
    ecs_os_zeromem(src);
    })

// PulseWindow.title is owned by the component. These hooks keep the owned string valid
// across flecs copies/moves and release it on destruction.
const char* dup_title(const char* title) {
    return title ? ecs_os_strdup(title) : nullptr;
}

void release_title(PulseWindow* ptr) {
    if (ptr->title) {
        ecs_os_free(const_cast<char*>(ptr->title));
        ptr->title = nullptr;
    }
}

ECS_COPY(PulseWindow, dst, src, {
    release_title(dst);
    *dst = *src;
    dst->title = dup_title(src->title);
    })

ECS_MOVE(PulseWindow, dst, src, {
    release_title(dst);
    *dst = *src;
    ecs_os_zeromem(src);
    })

ECS_DTOR(PulseWindow, ptr, {
    release_title(ptr);
    })

void on_window_set(ecs_iter_t* it)
{
    ecs_world_t* world = it->world;

    PulseWindow* windows = ecs_field(it, PulseWindow, 0);
    for (int i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
		auto& window = windows[i];

		if (!ecs_has_id(world, entity, ecs_id(PulseSdlWindow))) {
			ecs_add_id(world, entity, ecs_id(PulseSdlWindow));
		}

        PulseSdlWindow* sdl_window = ecs_get_mut(world, entity, PulseSdlWindow);
        bool sdl_window_changed = false;

		if (sdl_window->handle == nullptr) {
			SDL_Window* raw_window = create_sdl_window(
				window.title,
				window.width,
				window.height,
				window.resizable,
                window.external_graphics_context
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
            ecs_modified(world, entity, PulseSdlWindow);
        }
    }
}

void on_window_remove(ecs_iter_t* it)
{
    ecs_world_t* world = it->world;

    for (int i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        if (ecs_has_id(world, entity, ecs_id(PulseSdlWindow))) {
            ecs_remove_id(world, entity, ecs_id(PulseSdlWindow));
        }
        if (ecs_has_id(world, entity, PulseWindowCloseRequested)) {
            ecs_remove_id(world, entity, PulseWindowCloseRequested);
        }
        if (ecs_has_id(world, entity, PulseWindowResized)) {
            ecs_remove_id(world, entity, PulseWindowResized);
        }
    }
}

} // namespace

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, PulseWindow);
    ECS_COMPONENT_DEFINE(world, PulseSdlWindow);
    ECS_COMPONENT_DEFINE(world, pulse_window_state_resource);
    ECS_COMPONENT_DEFINE(world, PulseTextInputEvent);
    ECS_COMPONENT_DEFINE(world, PulseWindowFocusEvent);
    ECS_COMPONENT_DEFINE(world, PulseWindowMouseHoverEvent);
    ecs_add_pair(world, ecs_id(PulseWindow), EcsWith, ecs_id(PulseSdlWindow));

    ecs_type_hooks_t pulse_sdl_window_hooks = {
        .ctor = flecs_default_ctor,
        .dtor = ecs_dtor(PulseSdlWindow),
        .move = ecs_move(PulseSdlWindow),
    };
    ecs_set_hooks_id(world, ecs_id(PulseSdlWindow), &pulse_sdl_window_hooks);

	ecs_type_hooks_t pulse_window_hooks = {
        .ctor = flecs_default_ctor,
        .dtor = ecs_dtor(PulseWindow),
        .copy = ecs_copy(PulseWindow),
        .move = ecs_move(PulseWindow),
        .on_set = on_window_set,
        .on_remove = on_window_remove,
    };
    ecs_set_hooks_id(world, ecs_id(PulseWindow), &pulse_window_hooks);

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

pulse_window_plugin_state* state_from_app(PulseAppId app) {
    return state_from_world(pulse_app_world(app));
}

} // namespace pulse_window_internal
