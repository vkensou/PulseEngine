#include "imgui_internal.h"

#include <cstring>

ECS_COMPONENT_DECLARE(PulseImguiContext);

namespace pulse_imgui_internal {

ECS_COMPONENT_DECLARE(pulse_imgui_state_resource);

constexpr const char* kPluginName = "PulseImguiPlugin";

namespace {

PulseImguiPluginDesc normalize_plugin_desc(const PulseImguiPluginDesc* desc) {
    PulseImguiPluginDesc normalized = pulse_imgui_plugin_desc_default();
    if (desc) {
        normalized = *desc;
    }
    normalized.struct_size = sizeof(PulseImguiPluginDesc);
    normalized.version = PULSE_IMGUI_PLUGIN_DESC_VERSION;
    return normalized;
}

bool validate_plugin_desc(const PulseImguiPluginDesc* desc) {
    return !desc ||
        (desc->struct_size == sizeof(PulseImguiPluginDesc) &&
         desc->version == PULSE_IMGUI_PLUGIN_DESC_VERSION);
}

EPulsePluginBuildResult imgui_plugin_build(PulseAppId app, void* ctx) {
    pulse_imgui_plugin_state* state = static_cast<pulse_imgui_plugin_state*>(ctx);
    if (!state) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    state->app = app;

    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // 渲染初始化需要 graphics 插件先就绪（PulseRenderer singleton + asset system）。
    if (!pulse_get_renderer(app)) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE;
    }

    // 输入/窗口组件需要对应插件先注册（pulse_imgui 观察这些组件上的事件）。
    if (!ecs_id(PulseKeyboardInput) || !ecs_id(PulseMouseInput) ||
        !ecs_id(PulseMouseScroll) || !ecs_id(PulseWindow) ||
        !ecs_id(PulseTextInputEvent) || !ecs_id(PulseWindowFocusEvent) ||
        !ecs_id(PulseWindowMouseHoverEvent)) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE;
    }

    // 渲染目标窗口：当前固定为 primary window。
    ecs_entity_t window_entity = imgui_get_window_entity(world, state);
    if (!window_entity) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE;
    }

    // 创建 ImGui context（插件生命周期内唯一）。
    state->context = ImGui::CreateContext();
    if (!state->context) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }
    ImGui::SetCurrentContext(state->context);
    ImGuiIO& io = ImGui::GetIO();
    if (state->desc.flags & PULSE_IMGUI_PLUGIN_ENABLE_DOCKING) {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }
    if (state->ini_filename) {
        io.IniFilename = state->ini_filename;
    }

    ECS_COMPONENT_DEFINE(world, pulse_imgui_state_resource);
    ECS_COMPONENT_DEFINE(world, PulseImguiContext);

    // 把 ImGuiContext 作为组件挂到渲染目标窗口上。
    // 后续系统直接查询这个组件即可拿到 context，不再需要 ctx 或全局查找。
    PulseImguiContext context_component{};
    context_component.context = state->context;
    ecs_set_ptr(world, window_entity, PulseImguiContext, &context_component);

    pulse_imgui_state_resource resource{};
    resource.state = state;
    ecs_singleton_set_ptr(world, pulse_imgui_state_resource, &resource);

    // phase/frame system、输入 observer 都是纯 ECS 概念，
    // 在 build 阶段安装（这样用户在 app_run 前就能拿到 phase 注册 UI 系统）。
    create_imgui_phase(world, state);
    state->frame_system = install_imgui_frame_system(world, state);
    install_imgui_input(world, state);

    // 完整平台后端：clipboard / IME / 鼠标光标 / open-url 等。
    if (!imgui_platform_init(world, state)) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE;
    }

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult imgui_plugin_post_build(PulseAppId app, void* ctx) {
    pulse_imgui_plugin_state* state = static_cast<pulse_imgui_plugin_state*>(ctx);
    if (!state) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    // 渲染资源初始化需要 graphics 插件在 post_build 时已就绪。
    EPulseResult result = imgui_render_init(app, state);
    return result == PULSE_RESULT_OK ? PULSE_PLUGIN_BUILD_RESULT_OK : PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
}

void imgui_plugin_shutdown(PulseAppId app, void* ctx) {
    pulse_imgui_plugin_state* state = static_cast<pulse_imgui_plugin_state*>(ctx);
    if (!state) {
        return;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (world && ecs_id(pulse_imgui_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_imgui_state_resource);
    }
    if (world && ecs_id(PulseImguiContext) != 0) {
        ecs_remove_all(world, ecs_id(PulseImguiContext));
        ecs_delete(world, ecs_id(PulseImguiContext));
        ecs_id(PulseImguiContext) = 0;
    }

    imgui_render_shutdown(app, state);
    imgui_platform_shutdown(state);

    if (state->context) {
        ImGui::DestroyContext(state->context);
        state->context = nullptr;
    }
    if (state->ini_filename) {
        ecs_os_free(state->ini_filename);
        state->ini_filename = nullptr;
    }

    delete state;
}

} // namespace

pulse_imgui_plugin_state* state_from_world(ecs_world_t* world) {
    if (!world || ecs_id(pulse_imgui_state_resource) == 0) {
        return nullptr;
    }
    const pulse_imgui_state_resource* resource =
        ecs_singleton_get(world, pulse_imgui_state_resource);
    return resource ? resource->state : nullptr;
}

pulse_imgui_plugin_state* state_from_app(PulseAppId app) {
    return state_from_world(pulse_app_world(app));
}

} // namespace pulse_imgui_internal

using namespace pulse_imgui_internal;

extern "C" {

PulseImguiPluginDesc pulse_imgui_plugin_desc_default(void) {
    PulseImguiPluginDesc desc{};
    desc.struct_size = sizeof(PulseImguiPluginDesc);
    desc.version = PULSE_IMGUI_PLUGIN_DESC_VERSION;
    desc.flags = PULSE_IMGUI_PLUGIN_DEFAULT;
    desc.ini_filename = nullptr;
    return desc;
}

EPulseAppAddPluginResult pulse_add_imgui_plugin(
    PulseAppId app,
    const PulseImguiPluginDesc* desc
) {
    if (!app || !validate_plugin_desc(desc)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    pulse_imgui_plugin_state* state = new pulse_imgui_plugin_state();
    state->desc = normalize_plugin_desc(desc);
    if (state->desc.ini_filename) {
        state->ini_filename = ecs_os_strdup(state->desc.ini_filename);
        state->desc.ini_filename = nullptr;
    }

    const char* imgui_dependencies[] = {
        "PulseWindowPlugin",
        "PulseInputPlugin",
        "PulseAssetPlugin",
        "PulseGraphicPlugin"
    };
    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_IMGUI_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = state,
        .build = imgui_plugin_build,
        .post_build = imgui_plugin_post_build,
        .shutdown = imgui_plugin_shutdown,
        .dependency_count = 4,
        .dependencies = imgui_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

ImGuiContext* pulse_imgui_get_context(PulseAppId app) {
    pulse_imgui_plugin_state* state = state_from_app(app);
    return state ? state->context : nullptr;
}

ecs_entity_t pulse_imgui_get_phase(PulseAppId app) {
    pulse_imgui_plugin_state* state = state_from_app(app);
    return state ? state->imgui_phase : 0;
}

} // extern "C"
