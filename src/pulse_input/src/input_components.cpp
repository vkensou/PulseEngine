#include "input_internal.h"

#include "pulse_window.h"

#include <algorithm>
#include <cstring>

ECS_COMPONENT_DECLARE(PulseKeyboardInput);
ECS_COMPONENT_DECLARE(PulseMouseInput);
ECS_COMPONENT_DECLARE(PulseMouseMotion);
ECS_COMPONENT_DECLARE(PulseMouseScroll);
ECS_COMPONENT_DECLARE(PulseKeyEvent);
ECS_COMPONENT_DECLARE(PulseMouseButtonEvent);

namespace pulse_input_internal {

ECS_COMPONENT_DECLARE(pulse_input_state_resource);

namespace {

void input_system_run(ecs_iter_t* it) {
    auto* state = static_cast<pulse_input_plugin_state*>(it->ctx);
    if (!state || !state->app) {
        return;
    }

    ecs_world_t* world = it->world;

    // --- Keyboard: poll + edge detection ---
    int num_keys = 0;
    const bool* current_kbd = SDL_GetKeyboardState(&num_keys);

    // Get mutable access to the keyboard component singleton
    PulseKeyboardInput* kb = ecs_singleton_get_mut(world, PulseKeyboardInput);
    if (kb) {
        // Zero entire component first to avoid stale data from unused scancode indices
        std::memset(kb, 0, sizeof(PulseKeyboardInput));
        for (int i = 0; i < PULSE_SCANCODE_COUNT && i < num_keys; i++) {
            kb->pressed[i] = current_kbd[i];
            kb->just_pressed[i] = current_kbd[i] && !state->prev_keyboard[i];
            kb->just_released[i] = !current_kbd[i] && state->prev_keyboard[i];
        }
        ecs_singleton_modified(world, PulseKeyboardInput);
    }

    std::memcpy(state->prev_keyboard, current_kbd,
        (std::min)((size_t)PULSE_SCANCODE_COUNT, (size_t)num_keys) * sizeof(bool));

    // --- Emit PulseKeyEvent for each just_pressed / just_released key
    if (kb) {
        for (int i = 0; i < PULSE_SCANCODE_COUNT && i < num_keys; i++) {
            if (kb->just_pressed[i] || kb->just_released[i]) {
                PulseKeyEvent evt = {
                    .scancode = i,
                    .pressed = kb->just_pressed[i],
                    .repeat = false,
                };
                ecs_event_desc_t event_desc = {};
                event_desc.event = ecs_id(PulseKeyEvent);
                event_desc.entity = ecs_id(PulseKeyboardInput);
                event_desc.const_param = &evt;
                event_desc.observable = world;
                ecs_enqueue(world, &event_desc);
            }
        }
    }

    // --- Mouse buttons: poll + edge detection ---
    float mouse_x = 0.0f, mouse_y = 0.0f;
    uint32_t sdl_buttons = SDL_GetMouseState(&mouse_x, &mouse_y);
    uint8_t current_mouse = static_cast<uint8_t>(sdl_buttons & 0xFF);
    uint8_t mouse_jp = current_mouse & ~state->prev_mouse;
    uint8_t mouse_jr = state->prev_mouse & ~current_mouse;

    PulseMouseInput* mi = ecs_singleton_get_mut(world, PulseMouseInput);
    if (mi) {
        mi->state = current_mouse;
        mi->just_pressed = mouse_jp;
        mi->just_released = mouse_jr;
        ecs_singleton_modified(world, PulseMouseInput);
    }
    state->prev_mouse = current_mouse;

    // --- Emit PulseMouseButtonEvent for each just_pressed / just_released button
    if (mi) {
        for (int b = 0; b < 8; b++) {
            uint8_t mask = (uint8_t)(1u << b);
            if ((mouse_jp & mask) || (mouse_jr & mask)) {
                PulseMouseButtonEvent evt = {
                    .button = (uint8_t)b,
                    .pressed = (mouse_jp & mask) != 0,
                    .x = mouse_x,
                    .y = mouse_y,
                };
                ecs_event_desc_t event_desc = {};
                event_desc.event = ecs_id(PulseMouseButtonEvent);
                event_desc.entity = ecs_id(PulseMouseInput);
                event_desc.const_param = &evt;
                event_desc.observable = world;
                ecs_enqueue(world, &event_desc);
            }
        }
    }

    // --- Mouse motion ---
    float rel_x = 0.0f, rel_y = 0.0f;
    SDL_GetRelativeMouseState(&rel_x, &rel_y);

    PulseMouseMotion* mm = ecs_singleton_get_mut(world, PulseMouseMotion);
    if (mm) {
        mm->delta_x = rel_x;
        mm->delta_y = rel_y;
        mm->x = mouse_x;
        mm->y = mouse_y;
        ecs_singleton_modified(world, PulseMouseMotion);
    }

    // --- Mouse scroll: read from PulseWindowMouseScroll (accumulated by pulse_window's SDL event poll) ---
    const PulseWindowMouseScroll* raw_scroll =
        ecs_singleton_get(world, PulseWindowMouseScroll);
    PulseMouseScroll* ms = ecs_singleton_get_mut(world, PulseMouseScroll);
    if (ms) {
        ms->x = raw_scroll ? raw_scroll->x : 0.0f;
        ms->y = raw_scroll ? raw_scroll->y : 0.0f;
        ecs_singleton_modified(world, PulseMouseScroll);
    }
}

} // namespace

void register_components(ecs_world_t* world) {
    ECS_COMPONENT_DEFINE(world, PulseKeyboardInput);
    ECS_COMPONENT_DEFINE(world, PulseMouseInput);
    ECS_COMPONENT_DEFINE(world, PulseMouseMotion);
    ECS_COMPONENT_DEFINE(world, PulseMouseScroll);
    ECS_COMPONENT_DEFINE(world, PulseKeyEvent);
    ECS_COMPONENT_DEFINE(world, PulseMouseButtonEvent);
    ECS_COMPONENT_DEFINE(world, pulse_input_state_resource);
}

pulse_input_plugin_state* state_from_world(ecs_world_t* world) {
    if (!world || ecs_id(pulse_input_state_resource) == 0) {
        return nullptr;
    }
    const pulse_input_state_resource* resource =
        ecs_singleton_get(world, pulse_input_state_resource);
    return resource ? resource->state : nullptr;
}

pulse_input_plugin_state* state_from_app(PulseAppId app) {
    return state_from_world(pulse_app_world(app));
}

ecs_entity_t install_input_system(
    ecs_world_t* world,
    pulse_input_plugin_state* state
) {
    if (!world || !state) {
        return 0;
    }

    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "PulseInputSystem";
    ecs_entity_t system_entity = ecs_entity_init(world, &entity_desc);

    ecs_system_desc_t system_desc{};
    system_desc.entity = system_entity;
    system_desc.phase = EcsPreUpdate;
    system_desc.query.terms[0].id = ecs_id(PulseKeyboardInput);
    system_desc.query.cache_kind = EcsQueryCacheAuto;
    system_desc.callback = input_system_run;
    system_desc.ctx = state;

    return ecs_system_init(world, &system_desc);
}

} // namespace pulse_input_internal
