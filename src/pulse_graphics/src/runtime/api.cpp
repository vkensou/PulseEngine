#include "internal.h"

#include <new>
#include <algorithm>

namespace pulse_graphics_internal {

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

EPulseResult pulse_graphics_render_add_record_callback(
    PulseAppId app,
    const pulse_graphics_renderer_record_callback_desc* desc
) {
    pulse_graphics_state* state = pulse_graphics_internal::state_from_world(pulse_app_world(app));
    if (!state || !desc || !desc->callback) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    state->record_callbacks.push_back(*desc);
    state->sort_record_callbacks();
    return PULSE_RESULT_OK;
}

EPulseResult pulse_graphics_render_remove_record_callback(
    PulseAppId app,
    pulse_graphics_render_record_callback callback
) {
    pulse_graphics_state* state = pulse_graphics_internal::state_from_world(pulse_app_world(app));
    if (!state || !callback) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto it = std::find_if(state->record_callbacks.begin(), state->record_callbacks.end(),
        [callback](const pulse_graphics_renderer_record_callback_desc& d) {
            return d.callback == callback;
        });
    if (it != state->record_callbacks.end()) {
        state->record_callbacks.erase(it);
    }
    return PULSE_RESULT_OK;
}

const pulse_graphics_renderer* pulse_graphics_renderer_get(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(pulse_graphics_renderer) == 0) {
        return nullptr;
    }
    return ecs_singleton_get(world, pulse_graphics_renderer);
}

const pulse_graphics_surface* pulse_graphics_surface_get(PulseAppId app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) ||
        ecs_id(pulse_graphics_surface) == 0) {
        return nullptr;
    }
    return ecs_get(world, entity, pulse_graphics_surface);
}

const pulse_graphics_swapchain* pulse_graphics_swapchain_get(PulseAppId app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) ||
        ecs_id(pulse_graphics_swapchain) == 0) {
        return nullptr;
    }
    return ecs_get(world, entity, pulse_graphics_swapchain);
}

} // extern "C"
