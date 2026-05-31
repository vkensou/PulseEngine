#include "internal.h"

#include <new>
#include <algorithm>

namespace pulse_graphics_internal {

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

EPulseResult pulse_graphics_render_add_record_callback(
    PulseAppId app,
    const PulseGraphicsRendererRecordCallbackDesc* desc
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
    PulseProcGraphicsRenderRecordCallback callback
) {
    pulse_graphics_state* state = pulse_graphics_internal::state_from_world(pulse_app_world(app));
    if (!state || !callback) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto it = std::find_if(state->record_callbacks.begin(), state->record_callbacks.end(),
        [callback](const PulseGraphicsRendererRecordCallbackDesc& d) {
            return d.callback == callback;
        });
    if (it != state->record_callbacks.end()) {
        state->record_callbacks.erase(it);
    }
    return PULSE_RESULT_OK;
}

const PulseGraphicsRenderer* pulse_graphics_renderer_get(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(PulseGraphicsRenderer) == 0) {
        return nullptr;
    }
    return ecs_singleton_get(world, PulseGraphicsRenderer);
}

const PulseGraphicsSurface* pulse_graphics_surface_get(PulseAppId app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) ||
        ecs_id(PulseGraphicsSurface) == 0) {
        return nullptr;
    }
    return ecs_get(world, entity, PulseGraphicsSurface);
}

const PulseGraphicsSwapchain* pulse_graphics_swapchain_get(PulseAppId app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) ||
        ecs_id(PulseGraphicsSwapchain) == 0) {
        return nullptr;
    }
    return ecs_get(world, entity, PulseGraphicsSwapchain);
}

} // extern "C"
