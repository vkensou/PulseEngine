#include "input_internal.h"

#include <algorithm>
#include <cstring>

ECS_COMPONENT_DECLARE(PulseKeyboardInput);
ECS_COMPONENT_DECLARE(PulseMouseInput);
ECS_COMPONENT_DECLARE(PulseMouseMotion);
ECS_COMPONENT_DECLARE(PulseMouseScroll);
ECS_COMPONENT_DECLARE(PulseKeyEvent);
ECS_COMPONENT_DECLARE(PulseMouseButtonEvent);
ECS_COMPONENT_DECLARE(PulseMouseScrollEvent);

namespace pulse_input_internal {

ECS_COMPONENT_DECLARE(pulse_input_state_resource);

namespace {

// --- Observer: PulseKeyEvent → update PulseKeyboardInput ---
void key_event_observer(ecs_iter_t* it) {
    auto* state = static_cast<pulse_input_plugin_state*>(it->ctx);
    if (!state || !state->app) return;

    const PulseKeyEvent* evt = static_cast<const PulseKeyEvent*>(it->param);
    if (!evt || evt->scancode < 0 || evt->scancode >= PULSE_SCANCODE_COUNT) return;

    ecs_world_t* world = it->world;
    PulseKeyboardInput* kb = ecs_singleton_get_mut(world, PulseKeyboardInput);
    if (kb) {
        if (evt->pressed) {
            kb->pressed[evt->scancode] = true;
            if (!evt->repeat) {
                kb->just_pressed[evt->scancode] = true;
            }
        } else {
            kb->pressed[evt->scancode] = false;
            kb->just_released[evt->scancode] = true;
        }
        ecs_singleton_modified(world, PulseKeyboardInput);
    }
}

// --- Observer: PulseMouseButtonEvent → update PulseMouseInput ---
void mouse_button_observer(ecs_iter_t* it) {
    auto* state = static_cast<pulse_input_plugin_state*>(it->ctx);
    if (!state || !state->app) return;

    const PulseMouseButtonEvent* evt =
        static_cast<const PulseMouseButtonEvent*>(it->param);
    if (!evt || evt->button > 7) return;

    ecs_world_t* world = it->world;
    PulseMouseInput* mi = ecs_singleton_get_mut(world, PulseMouseInput);
    if (mi) {
        uint8_t mask = (uint8_t)(1u << evt->button);
        if (evt->pressed) {
            mi->state |= mask;
            mi->just_pressed |= mask;
        } else {
            mi->state &= ~mask;
            mi->just_released |= mask;
        }
        ecs_singleton_modified(world, PulseMouseInput);
    }
}

// --- Observer: PulseMouseScrollEvent → accumulate into PulseMouseScroll ---
void mouse_scroll_observer(ecs_iter_t* it) {
    auto* state = static_cast<pulse_input_plugin_state*>(it->ctx);
    if (!state || !state->app) return;

    const PulseMouseScrollEvent* evt =
        static_cast<const PulseMouseScrollEvent*>(it->param);
    if (!evt) return;

    ecs_world_t* world = it->world;
    PulseMouseScroll* ms = ecs_singleton_get_mut(world, PulseMouseScroll);
    if (ms) {
        ms->x += evt->x;
        ms->y += evt->y;
        ecs_singleton_modified(world, PulseMouseScroll);
    }
}

// --- PreUpdate system: mouse motion polling only (keyboard/mouse buttons use events) ---
void input_system_run(ecs_iter_t* it) {
    auto* state = static_cast<pulse_input_plugin_state*>(it->ctx);
    if (!state || !state->app) {
        return;
    }

    ecs_world_t* world = it->world;

    // --- Mouse motion (no event-driven source, keep polling) ---
    float mouse_x = 0.0f, mouse_y = 0.0f;
    SDL_GetMouseState(&mouse_x, &mouse_y);

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
}

// --- PostFrame system: clear transient state for the next frame ---
void post_frame_clear_system_run(ecs_iter_t* it) {
    ecs_world_t* world = it->world;

    PulseKeyboardInput* kb = ecs_singleton_get_mut(world, PulseKeyboardInput);
    if (kb) {
        std::memset(kb->just_pressed, 0, sizeof(kb->just_pressed));
        std::memset(kb->just_released, 0, sizeof(kb->just_released));
        ecs_singleton_modified(world, PulseKeyboardInput);
    }

    PulseMouseInput* mi = ecs_singleton_get_mut(world, PulseMouseInput);
    if (mi) {
        mi->just_pressed = 0;
        mi->just_released = 0;
        ecs_singleton_modified(world, PulseMouseInput);
    }

    PulseMouseScroll* ms = ecs_singleton_get_mut(world, PulseMouseScroll);
    if (ms) {
        ms->x = 0.0f;
        ms->y = 0.0f;
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
    ECS_COMPONENT_DEFINE(world, PulseMouseScrollEvent);
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

    // Register observers for events emitted by pulse_window
    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseKeyboardInput);
        desc.events[0] = ecs_id(PulseKeyEvent);
        desc.callback = key_event_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }

    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseMouseInput);
        desc.events[0] = ecs_id(PulseMouseButtonEvent);
        desc.callback = mouse_button_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }

    {
        ecs_observer_desc_t desc{};
        desc.query.terms[0].id = ecs_id(PulseMouseScroll);
        desc.events[0] = ecs_id(PulseMouseScrollEvent);
        desc.callback = mouse_scroll_observer;
        desc.ctx = state;
        ecs_observer_init(world, &desc);
    }

    // PreUpdate system: mouse motion polling
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "PulseInputSystem";
    ecs_entity_t system_entity = ecs_entity_init(world, &entity_desc);

    ecs_system_desc_t system_desc{};
    system_desc.entity = system_entity;
    system_desc.phase = EcsPreUpdate;
    system_desc.query.terms[0].id = ecs_id(PulseMouseMotion);
    system_desc.query.cache_kind = EcsQueryCacheAuto;
    system_desc.callback = input_system_run;
    system_desc.ctx = state;
    ecs_system_init(world, &system_desc);

    // PostFrame system: clear transient state
    ecs_entity_desc_t clear_desc{};
    clear_desc.name = "PulseInputPostFrameClear";
    ecs_entity_t clear_entity = ecs_entity_init(world, &clear_desc);

    ecs_system_desc_t clear_system_desc{};
    clear_system_desc.entity = clear_entity;
    clear_system_desc.phase = EcsPostFrame;
    clear_system_desc.query.terms[0].id = ecs_id(PulseKeyboardInput);
    clear_system_desc.query.cache_kind = EcsQueryCacheAuto;
    clear_system_desc.callback = post_frame_clear_system_run;
    clear_system_desc.ctx = state;
    ecs_system_init(world, &clear_system_desc);

    return system_entity;
}

} // namespace pulse_input_internal
