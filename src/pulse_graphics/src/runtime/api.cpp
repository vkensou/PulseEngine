#include "internal.h"

#include <new>
#include <algorithm>

namespace pulse_graphics_internal {

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

EPulseResult pulse_add_render_record_callback(
    PulseAppId app,
    const PulseRenderRecordCallbackDesc* desc
) {
    pulse_graphics_state* state = pulse_graphics_internal::state_from_world(pulse_app_world(app));
    if (!state || !desc || !desc->callback) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    state->record_callbacks.push_back(*desc);
    state->sort_record_callbacks();
    return PULSE_RESULT_OK;
}

EPulseResult pulse_remove_render_record_callback(
    PulseAppId app,
    PulseProcRenderRecordCallback callback
) {
    pulse_graphics_state* state = pulse_graphics_internal::state_from_world(pulse_app_world(app));
    if (!state || !callback) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto it = std::find_if(state->record_callbacks.begin(), state->record_callbacks.end(),
        [callback](const PulseRenderRecordCallbackDesc& d) {
            return d.callback == callback;
        });
    if (it != state->record_callbacks.end()) {
        state->record_callbacks.erase(it);
    }
    return PULSE_RESULT_OK;
}

const PulseRenderer* pulse_get_renderer(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(PulseRenderer) == 0) {
        return nullptr;
    }
    return ecs_singleton_get(world, PulseRenderer);
}

void pulse_set_per_draw_shader_properties(PulseAppId app, const char** per_draw_shader_properties, size_t per_draw_shader_properties_count) {
    pulse_graphics_state* state = pulse_graphics_internal::state_from_app(app);
    state->per_draw_shader_properties.clear();
    for (size_t i = 0; i < per_draw_shader_properties_count; ++i) {
        state->per_draw_shader_properties.emplace_back(per_draw_shader_properties[i]);
    }
}

const PulseSurface* pulse_graphics_surface_get(PulseAppId app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) ||
        ecs_id(PulseSurface) == 0) {
        return nullptr;
    }
    return ecs_get(world, entity, PulseSurface);
}

const PulseSwapchain* pulse_get_swapchain(PulseAppId app, ecs_entity_t entity) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || !entity || !ecs_is_alive(world, entity) ||
        ecs_id(PulseSwapchain) == 0) {
        return nullptr;
    }
    return ecs_get(world, entity, PulseSwapchain);
}

} // extern "C"
