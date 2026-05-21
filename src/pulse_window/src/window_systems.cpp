#include "window_internal.h"

namespace pulse_window_internal {

namespace {

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

} // namespace

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

} // namespace pulse_window_internal
