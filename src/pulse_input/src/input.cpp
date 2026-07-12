#include "input_internal.h"

#include <algorithm>
#include <cstring>

namespace pulse_input_internal {

constexpr const char* kPluginName = "PulseInputPlugin";

// ============================================================
// Plugin lifecycle
// ============================================================

EPulseResult input_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* state = static_cast<pulse_input_plugin_state*>(ctx);
    if (!state) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    state->app = app;

    // Register ECS components
    register_components(world);

    // Store state as singleton resource
    pulse_input_state_resource resource{};
    resource.state = state;
    ecs_singleton_set_ptr(world, pulse_input_state_resource, &resource);

    // Initialize previous keyboard state with current (avoids spurious just_pressed on first frame)
    int num_keys = 0;
    const bool* current_kbd = SDL_GetKeyboardState(&num_keys);
    const size_t copy_count = (std::min)((size_t)PULSE_SCANCODE_COUNT, (size_t)num_keys);
    std::memcpy(state->prev_keyboard, current_kbd, copy_count * sizeof(bool));

    // Initialize previous mouse state
    float mx = 0.0f, my = 0.0f;
    state->prev_mouse = static_cast<uint8_t>(SDL_GetMouseState(&mx, &my) & 0xFF);

    return PULSE_RESULT_OK;
}

EPulseResult input_plugin_post_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* state = static_cast<pulse_input_plugin_state*>(ctx);
    if (!state) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    install_input_system(world, state);
    return PULSE_RESULT_OK;
}

void input_plugin_shutdown(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_input_plugin_state*>(ctx);
    if (!state) {
        return;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (world) {
        if (ecs_id(pulse_input_state_resource) != 0) {
            ecs_singleton_remove(world, pulse_input_state_resource);
        }
    }

    delete state;
}

} // namespace pulse_input_internal

using namespace pulse_input_internal;

// ============================================================
// Public C API
// ============================================================

extern "C" {

EPulseResult pulse_add_input_plugin(PulseAppId app) {
    if (!app) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    auto* state = new pulse_input_plugin_state();

    PulsePluginDesc plugin_desc = {
        sizeof(PulsePluginDesc),
        PULSE_PLUGIN_DESC_VERSION,
        kPluginName,
        state,
        input_plugin_build,
        input_plugin_post_build,
        input_plugin_shutdown,
    };

    EPulseResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

// --- Keyboard ---

bool pulse_input_is_key_down(PulseAppId app, int32_t scancode) {
    if (!app || scancode < 0 || scancode >= PULSE_SCANCODE_COUNT) {
        return false;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return false;
    }
    const PulseKeyboardInput* kb = ecs_singleton_get(world, PulseKeyboardInput);
    if (!kb) {
        return false;
    }
    return kb->pressed[scancode];
}

bool pulse_input_key_just_pressed(PulseAppId app, int32_t scancode) {
    if (!app || scancode < 0 || scancode >= PULSE_SCANCODE_COUNT) {
        return false;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return false;
    }
    const PulseKeyboardInput* kb = ecs_singleton_get(world, PulseKeyboardInput);
    if (!kb) {
        return false;
    }
    return kb->just_pressed[scancode];
}

bool pulse_input_key_just_released(PulseAppId app, int32_t scancode) {
    if (!app || scancode < 0 || scancode >= PULSE_SCANCODE_COUNT) {
        return false;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return false;
    }
    const PulseKeyboardInput* kb = ecs_singleton_get(world, PulseKeyboardInput);
    if (!kb) {
        return false;
    }
    return kb->just_released[scancode];
}

// --- Mouse buttons ---

bool pulse_input_is_mouse_button_down(PulseAppId app, uint8_t button) {
    if (!app) {
        return false;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return false;
    }
    const PulseMouseInput* mi = ecs_singleton_get(world, PulseMouseInput);
    if (!mi) {
        return false;
    }
    return (mi->state >> button) & 1;
}

bool pulse_input_mouse_button_just_pressed(PulseAppId app, uint8_t button) {
    if (!app) {
        return false;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return false;
    }
    const PulseMouseInput* mi = ecs_singleton_get(world, PulseMouseInput);
    if (!mi) {
        return false;
    }
    return (mi->just_pressed >> button) & 1;
}

bool pulse_input_mouse_button_just_released(PulseAppId app, uint8_t button) {
    if (!app) {
        return false;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return false;
    }
    const PulseMouseInput* mi = ecs_singleton_get(world, PulseMouseInput);
    if (!mi) {
        return false;
    }
    return (mi->just_released >> button) & 1;
}

// --- Mouse position / delta / scroll ---

void pulse_input_get_mouse_position(PulseAppId app, float* out_x, float* out_y) {
    if (!app) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        return;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        return;
    }
    const PulseMouseMotion* mm = ecs_singleton_get(world, PulseMouseMotion);
    if (!mm) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        return;
    }
    if (out_x) *out_x = mm->x;
    if (out_y) *out_y = mm->y;
}

void pulse_input_get_mouse_delta(PulseAppId app, float* out_dx, float* out_dy) {
    if (!app) {
        if (out_dx) *out_dx = 0.0f;
        if (out_dy) *out_dy = 0.0f;
        return;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        if (out_dx) *out_dx = 0.0f;
        if (out_dy) *out_dy = 0.0f;
        return;
    }
    const PulseMouseMotion* mm = ecs_singleton_get(world, PulseMouseMotion);
    if (!mm) {
        if (out_dx) *out_dx = 0.0f;
        if (out_dy) *out_dy = 0.0f;
        return;
    }
    if (out_dx) *out_dx = mm->delta_x;
    if (out_dy) *out_dy = mm->delta_y;
}

void pulse_input_get_mouse_scroll(PulseAppId app, float* out_x, float* out_y) {
    if (!app) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        return;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        return;
    }
    const PulseMouseScroll* ms = ecs_singleton_get(world, PulseMouseScroll);
    if (!ms) {
        if (out_x) *out_x = 0.0f;
        if (out_y) *out_y = 0.0f;
        return;
    }
    if (out_x) *out_x = ms->x;
    if (out_y) *out_y = ms->y;
}

} // extern "C"
