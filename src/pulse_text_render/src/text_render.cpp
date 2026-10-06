#include "text_render_internal.h"

#include "pulse_text_render_reflection.h"

#include <cstddef>
#include <cstring>

ECS_COMPONENT_DECLARE(PulseText);

namespace pulse_text_render_internal {

constexpr const char* kPluginName = "pulse_text_render";

namespace {

int serialize_text_content(const ecs_serializer_t* ser, const void* src) {
    char buffer[PULSE_TEXT_MAX_CHARS + 1];
    memcpy(buffer, src, PULSE_TEXT_MAX_CHARS);
    buffer[PULSE_TEXT_MAX_CHARS] = '\0';
    const char* value = buffer;
    return ser->value(ecs_id(ecs_string_t), &value);
}

void assign_text_content(void* dst, ecs_world_t*, const char* value) {
    char* out = static_cast<char*>(dst);
    size_t count = value ? strlen(value) : 0;
    if (count > (size_t)PULSE_TEXT_MAX_CHARS - 1) count = (size_t)PULSE_TEXT_MAX_CHARS - 1;
    while (count > 0 && (static_cast<unsigned char>(value[count]) & 0xC0u) == 0x80u) --count;
    if (count > 0) memcpy(out, value, count);
    out[count] = '\0';
}

void register_text_content_reflection(ecs_world_t* world) {
    ecs_entity_desc_t type_desc = {};
    type_desc.name = "PulseTextContent";
    const ecs_entity_t content_type = ecs_entity_init(world, &type_desc);
    EcsComponent component_info = {};
    component_info.size = (ecs_size_t)PULSE_TEXT_MAX_CHARS;
    component_info.alignment = 1;
    ecs_set_id(world, content_type, ecs_id(EcsComponent), sizeof(EcsComponent), &component_info);
    EcsOpaque opaque = {};
    opaque.as_type = ecs_id(ecs_string_t);
    opaque.serialize = serialize_text_content;
    opaque.assign_string = assign_text_content;
    ecs_set_id(world, content_type, ecs_id(EcsOpaque), sizeof(EcsOpaque), &opaque);
    ecs_member_t member_desc = {};
    member_desc.name = "text";
    member_desc.type = content_type;
    member_desc.offset = (int32_t)offsetof(PulseText, text);
    member_desc.use_offset = true;
    ecs_struct_add_member(world, ecs_id(PulseText), &member_desc);
}

}

EPulsePluginBuildResult text_render_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    auto* state = static_cast<pulse_text_render_state*>(ctx);
    if (!state) return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    state->app = app;

    pulse_text_render_register_reflection(world);
    register_text_content_reflection(world);
    install_text_feature(app, world);

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void text_render_plugin_shutdown(PulseAppId app, void* ctx) {
    if (app) pulse_remove_render_feature(app, kTextFeatureName);
    delete static_cast<pulse_text_render_state*>(ctx);
}

} // namespace pulse_text_render_internal

using namespace pulse_text_render_internal;

extern "C" {

EPulseAppAddPluginResult pulse_add_text_render_plugin(PulseAppId app) {
    if (!app) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    if (pulse_app_has_plugin(app, kPluginName)) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;

    auto* state = new pulse_text_render_state();

    const char* text_render_dependencies[] = { "pulse_graphics", "pulse_transform", "pulse_renderer", "pulse_text", "pulse_font" };

    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_TEXT_RENDER_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = state,
        .build = text_render_plugin_build,
        .post_build = nullptr,
        .shutdown = text_render_plugin_shutdown,
        .dependency_count = 5,
        .dependencies = text_render_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

} // extern "C"
