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
        [](const PulseRenderRecordCallbackDesc& a,
           const PulseRenderRecordCallbackDesc& b) {
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

pulse_graphics_state* state_from_app(PulseAppId app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(pulse_graphics_state_resource) == 0) return nullptr;
    const pulse_graphics_state_resource* res = ecs_singleton_get(world, pulse_graphics_state_resource);
    return res ? res->state : nullptr;
}

CGPUDeviceId get_device(PulseAppId app) {
    const PulseRenderer* renderer = pulse_get_renderer(app);
    return renderer ? renderer->device : CGPUDeviceId{CGPU_NULLPTR};
}

namespace {

bool validate_plugin_desc(const PulseGraphicsPluginDesc* desc) {
    return !desc ||
        (desc->struct_size == sizeof(PulseGraphicsPluginDesc) &&
         desc->version == PULSE_GRAPHICS_PLUGIN_DESC_VERSION);
}

PulseGraphicsPluginDesc normalize_plugin_desc(
    const PulseGraphicsPluginDesc* desc
) {
    PulseGraphicsPluginDesc normalized = pulse_graphics_plugin_desc_default();
    if (desc) {
        normalized = *desc;
    }

    normalized.struct_size = sizeof(PulseGraphicsPluginDesc);
    normalized.version = PULSE_GRAPHICS_PLUGIN_DESC_VERSION;
    if (normalized.image_count == 0) {
        normalized.image_count = kDefaultImageCount;
    }
    if (normalized.swapchain_format == CGPU_TEXTURE_FORMAT_UNDEFINED) {
        normalized.swapchain_format = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM;
    }
    return normalized;
}

void create_blit_shader(PulseAppId app, pulse_graphics_state* state) {
	CGPUBlendAttachmentState blit_blend_attachments = {
		.enable = false,
		.src_factor = CGPU_BLEND_FACTOR_ONE,
		.dst_factor = CGPU_BLEND_FACTOR_ZERO,
		.src_alpha_factor = CGPU_BLEND_FACTOR_ONE,
		.dst_alpha_factor = CGPU_BLEND_FACTOR_ZERO,
		.blend_op = CGPU_BLEND_OP_ADD,
		.blend_alpha_op = CGPU_BLEND_OP_ADD,
		.color_mask = CGPU_COLOR_MASK_RGBA,
	};

    uint8_t blit_vert_spv[] = {
		#include "blit.vs.spv.h"
	};
	uint8_t blit_frag_spv[] = {
		#include "blit.ps.spv.h"
	};

    PulseShaderProperty shader_props[] = {
        {.name = "source",        .type = PULSE_SHADER_PROPERTY_TYPE_TEXTURE, .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL, .set = 0, .binding = 0, .offset = 0, .size = 0 },
        {.name = "linearSampler", .type = PULSE_SHADER_PROPERTY_TYPE_SAMPLER, .role = PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL, .set = 0, .binding = 1, .offset = 0, .size = 0 },
    };

    PulseShaderCreateFromBinaryDesc blit_shader_desc = {
        .vs_data = blit_vert_spv,
        .vs_size = sizeof(blit_vert_spv),
        .fs_data = blit_frag_spv,
        .fs_size = sizeof(blit_frag_spv),
        .blend_desc = {
            .attachment_count = 1,
            .p_attachments = &blit_blend_attachments,
            .alpha_to_coverage = false,
            .independent_blend = false,
        },
        .depth_desc = {
            .depth_test = false,
            .depth_write = false,
            .stencil_test = false,
        },
        .rasterizer_state = {
            .cull_mode = CGPU_CULL_MODE_NONE,
        },
        .property_count = 2,
        .p_properties = shader_props,
    };

    state->blit_shader.handle = pulse_create_shader_from_binary(app, &blit_shader_desc);
    state->blit_shader.ptr = pulse_graphics_internal::internal_borrow_shader(
        state->asset_system, state->blit_shader.handle);

	PulseSamplerCreateDesc blit_linear_sampler_desc = {
        .desc = {
            .min_filter = CGPU_FILTER_TYPE_LINEAR,
            .mag_filter = CGPU_FILTER_TYPE_LINEAR,
            .mipmap_mode = CGPU_MIP_MAP_MODE_LINEAR,
            .address_u = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_v = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
            .address_w = CGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mip_lod_bias = 0,
            .max_anisotropy = 1,
        }
	};
	state->blit_linear_sampler.handle = pulse_create_sampler(app, &blit_linear_sampler_desc);
    state->blit_linear_sampler.ptr = pulse_graphics_internal::internal_borrow_sampler(
        state->asset_system, state->blit_linear_sampler.handle);
}

EPulsePluginBuildResult graphic_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    pulse_graphics_state* state = static_cast<pulse_graphics_state*>(ctx);
    state->app = app;
    state->asset_system = pulse_get_asset_system(app);

    if (!create_renderer(state)) {
        destroy_renderer(state);
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
    }

    ECS_COMPONENT_DEFINE(world, pulse_graphics_state_resource);
    register_components(world);

    pulse_graphics_state_resource res{ state };
    ecs_singleton_set_ptr(world, pulse_graphics_state_resource, &res);
    ecs_singleton_set_ptr(world, PulseRenderer, &state->renderer);

    CGPUDeviceId device = get_device(app);
    register_graphics_asset_types_and_loaders(state->asset_system, device);

    create_blit_shader(app, state);

    install_upload_callback(app);

    install_observers(state, world);
    install_render_systems(state, world);

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

void graphic_plugin_shutdown(PulseAppId app, void* ctx) {
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

    if (world && ecs_id(PulseRenderer) != 0) {
        ecs_singleton_remove(world, PulseRenderer);
    }
    if (world && ecs_id(pulse_graphics_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_graphics_state_resource);
    }

    delete_render_components(world);

    PulseAssetSystemId as = state->asset_system;
    pulse_asset_system_force_unload_assets(as, PULSE_TYPE_MATERIAL);
    pulse_asset_system_force_unload_assets(as, PULSE_TYPE_MESH);
    pulse_asset_system_force_unload_assets(as, PULSE_TYPE_COMPUTE_SHADER);
    pulse_asset_system_force_unload_assets(as, PULSE_TYPE_SHADER);
    pulse_asset_system_force_unload_assets(as, PULSE_TYPE_SHADER_LIBRARY);
    pulse_asset_system_force_unload_assets(as, PULSE_TYPE_SAMPLER);
    pulse_asset_system_force_unload_assets(as, PULSE_TYPE_GRAPHICS_BUFFER);
    pulse_asset_system_force_unload_assets(as, PULSE_TYPE_TEXTURE);

    if (state->default_texture_view)
        cgpu_device_free_texture_view(state->renderer.device, state->default_texture_view);
    state->default_texture_view = CGPU_NULLPTR;

    if (state->default_texture)
        cgpu_device_free_texture(state->renderer.device, state->default_texture);
    state->default_texture = CGPU_NULLPTR;

    if (state->default_sampler)
        cgpu_device_free_sampler(state->renderer.device, state->default_sampler);
    state->default_sampler = CGPU_NULLPTR;

    if (state->default_buffer)
        cgpu_device_free_buffer(state->renderer.device, state->default_buffer);
    state->default_buffer = CGPU_NULLPTR;

    destroy_renderer(state);

    delete state;
}

} // namespace

} // namespace pulse_graphics_internal

using namespace pulse_graphics_internal;

extern "C" {

PulseGraphicsPluginDesc pulse_graphics_plugin_desc_default(void) {
    PulseGraphicsPluginDesc desc{};
    desc.struct_size = sizeof(PulseGraphicsPluginDesc);
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

EPulseAppAddPluginResult pulse_add_graphics_plugin(PulseAppId app, const PulseGraphicsPluginDesc* desc) {
    if (!app || !validate_plugin_desc(desc)) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    if (pulse_app_has_plugin(app, kPluginName)) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;

    pulse_graphics_state* state = new (std::nothrow) pulse_graphics_state();
    if (!state) return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INTERNAL;
    state->desc = normalize_plugin_desc(desc);
    state->desc.per_draw_shader_property_count = 0;
    state->desc.p_per_draw_shader_properties = nullptr;
    for (size_t i = 0; i < desc->per_draw_shader_property_count; ++i) {
        state->per_draw_shader_properties.emplace_back(desc->p_per_draw_shader_properties[i]);
    }

    const char* graphics_dependencies[] = { "PulseWindowPlugin", "PulseAssetPlugin" };
    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_GRAPHICS_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = state,
        .build = graphic_plugin_build,
        .post_build = nullptr,
        .shutdown = graphic_plugin_shutdown,
        .dependency_count = 2,
        .dependencies = graphics_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

PULSE_GRAPHICS_API EPulseResult pulse_module_register(PulseAppId app, const void* config, uint32_t config_size) {
    if (config && config_size != sizeof(PulseGraphicsPluginDesc)) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    EPulseAppAddPluginResult r = pulse_add_graphics_plugin(app, static_cast<const PulseGraphicsPluginDesc*>(config));
    switch (r) {
        case PULSE_APP_ADD_PLUGIN_RESULT_OK: return PULSE_RESULT_OK;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT: return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_STATE: return PULSE_RESULT_ERROR_INVALID_STATE;
        case PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN: return PULSE_RESULT_ERROR_DUPLICATE_PLUGIN;
        default: return PULSE_RESULT_ERROR_INTERNAL;
    }
}

} // extern "C"
