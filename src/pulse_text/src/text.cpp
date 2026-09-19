#include "text_internal.h"

namespace pulse_text_internal {

constexpr const char* kPluginName = "pulse_text";

ECS_COMPONENT_DECLARE(pulse_text_state_resource);

namespace {

bool desc_is_valid(const PulseTextBlockDesc* desc) {
    if (!desc || desc->chain == PULSE_FONT_ID_NONE || desc->size <= 0.0f) {
        return false;
    }
    if (desc->align_h < PULSE_TEXT_ALIGN_H_LEFT || desc->align_h >= PULSE_TEXT_ALIGN_H_COUNT) {
        return false;
    }
    if (desc->align_v < PULSE_TEXT_ALIGN_V_TOP || desc->align_v >= PULSE_TEXT_ALIGN_V_COUNT) {
        return false;
    }
    return true;
}

}

pulse_text_plugin_state* state_from_world(ecs_world_t* world) {
    if (!world || ecs_id(pulse_text_state_resource) == 0) {
        return nullptr;
    }
    const pulse_text_state_resource* resource = ecs_singleton_get(world, pulse_text_state_resource);
    return resource ? resource->state : nullptr;
}

pulse_text_plugin_state* state_from_app(PulseAppId app) {
    return state_from_world(pulse_app_world(app));
}

EPulsePluginBuildResult pulse_text_plugin_build(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_text_plugin_state*>(ctx);
    if (!state || !app) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    state->app = app;
    ecs_id(pulse_text_state_resource) = flecs::_::type<pulse_text_state_resource>::id(world);
    pulse_text_state_resource resource{};
    resource.state = state;
    ecs_singleton_set_ptr(world, pulse_text_state_resource, &resource);
    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void pulse_text_plugin_shutdown(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_text_plugin_state*>(ctx);
    if (!state) {
        return;
    }
    ecs_world_t* world = app ? pulse_app_world(app) : nullptr;
    if (world && ecs_id(pulse_text_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_text_state_resource);
        if (ecs_is_alive(world, ecs_id(pulse_text_state_resource))) {
            ecs_delete(world, ecs_id(pulse_text_state_resource));
        }
        ecs_id(pulse_text_state_resource) = 0;
    }
    delete state;
}

}

using namespace pulse_text_internal;

extern "C" {

EPulseAppAddPluginResult pulse_add_text_plugin(PulseAppId app) {
    if (!app) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, pulse_text_internal::kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    auto* state = new pulse_text_plugin_state();

    const char* text_dependencies[] = { "pulse_font" };
    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_TEXT_PLUGIN_DESC_VERSION,
        .name = pulse_text_internal::kPluginName,
        .ctx = state,
        .build = pulse_text_plugin_build,
        .post_build = nullptr,
        .shutdown = pulse_text_plugin_shutdown,
        .dependency_count = 1,
        .dependencies = text_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, pulse_text_internal::kPluginName)) {
        delete state;
    }
    return result;
}

PulseTextLayout* pulse_text_layout(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width, float box_height) {
    if (!state_from_app(app) || !desc_is_valid(desc)) {
        return nullptr;
    }
    return text_layout(app, desc, text, box_width, box_height);
}

PulseTextMeasure pulse_text_measure(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width) {
    PulseTextMeasure out{};
    if (!state_from_app(app) || !desc_is_valid(desc)) {
        return out;
    }
    return text_measure(app, desc, text, box_width);
}

void pulse_text_layout_free(PulseAppId app, PulseTextLayout* layout) {
    (void)app;
    if (!layout) {
        return;
    }
    delete[] layout->p_instances;
    delete layout;
}

}
