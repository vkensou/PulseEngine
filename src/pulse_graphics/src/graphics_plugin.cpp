#include "graphics_internal.h"

#include <algorithm>
#include <new>

#include "runtime/internal.h"

namespace pulse_graphics_internal {

constexpr uint32_t kDefaultImageCount = 3;
const char* kPluginName = "PulseGraphicPlugin";
ECS_COMPONENT_DECLARE(pulse_graphics_state_resource);

void pulse_graphics_state::sort_record_callbacks() {
    std::stable_sort(record_callbacks.begin(), record_callbacks.end(),
        [](const pulse_graphics_renderer_record_callback_desc& a,
           const pulse_graphics_renderer_record_callback_desc& b) {
            return a.priority < b.priority;
        });
}

pulse_graphics_state* state_from_world(ecs_world_t* world) {
    if (!world || ecs_id(pulse_graphics_state_resource) == 0) {
        return nullptr;
    }

    const pulse_graphics_state_resource* resource =
        ecs_singleton_get(world, pulse_graphics_state_resource);
    return resource ? resource->state : nullptr;
}

pulse_graphics_state* state_from_app(pulse_app_t app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(pulse_graphics_state_resource) == 0) return nullptr;
    const pulse_graphics_state_resource* res = ecs_singleton_get(world, pulse_graphics_state_resource);
    return res ? res->state : nullptr;
}

CGPUDeviceId get_device(pulse_app_t app) {
    const pulse_graphics_renderer* renderer = pulse_graphics_renderer_get(app);
    return renderer ? renderer->device : CGPUDeviceId{CGPU_NULLPTR};
}

namespace {

bool validate_plugin_desc(const pulse_graphics_plugin_desc* desc) {
    return !desc ||
        (desc->struct_size == sizeof(pulse_graphics_plugin_desc) &&
         desc->version == PULSE_GRAPHICS_PLUGIN_DESC_VERSION);
}

pulse_graphics_plugin_desc normalize_plugin_desc(
    const pulse_graphics_plugin_desc* desc
) {
    pulse_graphics_plugin_desc normalized = pulse_graphics_plugin_desc_default();
    if (desc) {
        normalized = *desc;
    }

    normalized.struct_size = sizeof(pulse_graphics_plugin_desc);
    normalized.version = PULSE_GRAPHICS_PLUGIN_DESC_VERSION;
    if (normalized.image_count == 0) {
        normalized.image_count = kDefaultImageCount;
    }
    if (normalized.swapchain_format == CGPU_TEXTURE_FORMAT_UNDEFINED) {
        normalized.swapchain_format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    }
    return normalized;
}

pulse_result_t graphic_plugin_build(pulse_app_t app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    pulse_graphics_state* state = static_cast<pulse_graphics_state*>(ctx);
    state->app = app;

    if (!create_renderer(state)) {
        destroy_renderer(state);
        return PULSE_ERROR_INTERNAL;
    }

    ECS_COMPONENT_DEFINE(world, pulse_graphics_state_resource);
    register_components(world);

    pulse_graphics_state_resource res{ state };
    ecs_singleton_set_ptr(world, pulse_graphics_state_resource, &res);
    ecs_singleton_set_ptr(world, pulse_graphics_renderer, &state->renderer);

    CGPUDeviceId device = get_device(app);
    register_graphics_asset_types_and_loaders(app, device);

    install_upload_callback(app);

    install_observers(state, world);
    install_render_systems(state, world);

    return PULSE_OK;
}

void graphic_plugin_shutdown(pulse_app_t app, void* ctx) {
    pulse_graphics_state* state =
        static_cast<pulse_graphics_state*>(ctx);
    if (!state) {
        return;
    }

    ecs_world_t* world = pulse_app_world(app);
    if (world && ecs_id(pulse_graphics_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_graphics_state_resource);
        if (ecs_is_alive(world, ecs_id(pulse_graphics_state_resource))) {
            ecs_delete(world, ecs_id(pulse_graphics_state_resource));
        }
        ecs_id(pulse_graphics_state_resource) = 0;
    }

    if (state->renderer.graphics_queue) {
        cgpu_queue_wait_idle(state->renderer.graphics_queue);
    }

    uninstall_render_systems(state, world);
    uninstall_observers(state, world);
    remove_render_window_components(world);

    if (world && ecs_id(pulse_graphics_renderer) != 0) {
        ecs_singleton_remove(world, pulse_graphics_renderer);
    }
    if (world && ecs_id(pulse_graphics_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_graphics_state_resource);
    }

    delete_render_components(world);
    destroy_renderer(state);

    delete state;
}

} // namespace

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

pulse_graphics_plugin_desc pulse_graphics_plugin_desc_default(void) {
    pulse_graphics_plugin_desc desc{};
    desc.struct_size = sizeof(pulse_graphics_plugin_desc);
    desc.version = PULSE_GRAPHICS_PLUGIN_DESC_VERSION;
    desc.backend = CGPU_BACKEND_VULKAN;
    desc.swapchain_format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    desc.image_count = kDefaultImageCount;
    desc.enable_debug_layer = false;
    desc.enable_gpu_based_validation = false;
    desc.enable_vsync = true;
    desc.record_callback = nullptr;
    desc.record_user_data = nullptr;
    return desc;
}

pulse_result_t pulse_graphics_add_plugin(pulse_app_t app, const pulse_graphics_plugin_desc* desc) {
    if (!app || !validate_plugin_desc(desc)) return PULSE_ERROR_INVALID_ARGUMENT;
    if (pulse_app_has_plugin(app, kPluginName)) return PULSE_ERROR_DUPLICATE_PLUGIN;

    pulse_graphics_state* state = new (std::nothrow) pulse_graphics_state();
    if (!state) return PULSE_ERROR_INTERNAL;
    state->desc = normalize_plugin_desc(desc);

    pulse_plugin_desc plugin_desc = {
        sizeof(pulse_plugin_desc),
        PULSE_PLUGIN_DESC_VERSION,
        kPluginName,
        state,
        graphic_plugin_build,
        nullptr,
        graphic_plugin_shutdown,
    };

    pulse_result_t result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

} // extern "C"
