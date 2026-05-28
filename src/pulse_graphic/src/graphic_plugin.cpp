#include "graphic_internal.h"
#include <cstdlib>
#include <cstring>
#include <vector>

#include "renderer.h"

namespace pulse_graphic_internal {

// ── Bytecode type ─────────────────────────────────────────────
struct PulseBytecodeSlot {
    uint8_t* data = nullptr;
    uint64_t size = 0;
};

static void destroy_bytecode(void* ptr, void*) {
    auto* bc = static_cast<PulseBytecodeSlot*>(ptr);
    std::free(bc->data);
    bc->data = nullptr;
    bc->size = 0;
}

static pulse_asset_loader_status_t step_bytecode(
    void* state, const pulse_asset_load_task* ctx,
    const char** out_error)
{
    (void)state;
    auto* bc = static_cast<PulseBytecodeSlot*>(ctx->out_asset);
    if (ctx->byte_size == 0 || !ctx->bytes) {
        *out_error = "bytecode loader: no data";
        return PULSE_ASSET_LOADER_FAILED;
    }
    bc->data = static_cast<uint8_t*>(std::malloc(ctx->byte_size));
    if (!bc->data) {
        *out_error = "bytecode loader: out of memory";
        return PULSE_ASSET_LOADER_FAILED;
    }
    std::memcpy(bc->data, ctx->bytes, ctx->byte_size);
    bc->size = ctx->byte_size;
    return PULSE_ASSET_LOADER_DONE;
}

const char* kPluginName = "PulseGraphicPlugin";

static void destroy_sampler(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_sampler_data_t* data = static_cast<pulse_sampler_data_t*>(ptr);
    if (data->handle) cgpu_device_free_sampler(device, data->handle);
}

struct GraphStateResource {
    pulse_graphic_state* state;
};
ECS_COMPONENT_DECLARE(GraphStateResource);

static pulse_result_t graphic_plugin_build(pulse_app_t app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    pulse_graphic_state* gstate = static_cast<pulse_graphic_state*>(ctx);
    gstate->app = app;
    ECS_COMPONENT_DEFINE(world, GraphStateResource);

    CGPUDeviceId device = get_device(app);
    auto register_type = [app, device](uint64_t type_id, uint32_t size, uint32_t align, pulse_asset_destroy_fn destroy) {
        pulse_asset_type_desc type_desc{};
        type_desc.struct_size = sizeof(pulse_asset_type_desc);
        type_desc.version = PULSE_ASSET_TYPE_DESC_VERSION;
        type_desc.type_id = type_id;
        type_desc.size = size;
        type_desc.align = align;
        type_desc.destroy = destroy;
        type_desc.user_data = const_cast<struct CGPUDevice*>(device);
        return pulse_asset_register_type(app, &type_desc);
    };

    register_shader_type(app, device);
    register_compute_shader_type(app, device);
    register_shader_library_type(app, device);
    register_mesh_type(app, device);
	register_texture_type(app, device);
    register_buffer_type(app, device);
    register_material_type(app, device);
    register_type(PULSE_TYPE_SAMPLER, sizeof(pulse_sampler_data_t), alignof(pulse_sampler_data_t), destroy_sampler);
    register_type(PULSE_TYPE_BYTECODE, sizeof(PulseBytecodeSlot), alignof(PulseBytecodeSlot), destroy_bytecode);

    // Register loaders
    auto register_loader = [app](uint64_t type_id, const char* ext,
                                  pulse_asset_loader_ctor_fn ctor,
                                  pulse_asset_loader_dtor_fn dtor,
                                  pulse_asset_loader_step_fn step,
                                  uint32_t loader_size,
                                  uint32_t loader_align,
                                  uint32_t settings_size,
                                  uint32_t settings_align,
                                  void* user_data)
    {
        pulse_asset_loader_desc ld{};
        ld.struct_size = sizeof(pulse_asset_loader_desc);
        ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
        ld.type_id = type_id;
        ld.extensions = ext;
        ld.ctor = ctor;
        ld.dtor = dtor;
        ld.step = step;
        ld.loader_size = loader_size;
        ld.loader_align = loader_align;
        ld.settings_size = settings_size;
        ld.settings_align = settings_align;
        ld.user_data = user_data;
        return pulse_asset_register_loader(app, &ld);
    };

    register_loader(PULSE_TYPE_BYTECODE, "dxc,gltf",
                    nullptr, nullptr, step_bytecode, 0, 0, 0, 0, nullptr);
    register_shader_library_load_loader(app, device);
    register_shader_library_create_loader(app, device);
    register_shader_create_loaders(app, device);
    register_compute_shader_create_loaders(app, device);
    register_texture_create_loader(app, device);
    register_texture_load_loader(app, device);
    register_buffer_create_loader(app, device);
    register_material_create_loader(app, device);
    register_mesh_create_loader(app, device);
    register_mesh_load_loader(app, device);

    GraphStateResource res{gstate};
    ecs_singleton_set_ptr(world, GraphStateResource, &res);

    pulse_graphic_internal::install_upload_callback(app);

    return PULSE_OK;
}

static void graphic_plugin_shutdown(pulse_app_t app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (world && ecs_id(GraphStateResource) != 0) {
        ecs_singleton_remove(world, GraphStateResource);
        if (ecs_is_alive(world, ecs_id(GraphStateResource))) {
            ecs_delete(world, ecs_id(GraphStateResource));
        }
        ecs_id(GraphStateResource) = 0;
    }
    delete static_cast<pulse_graphic_state*>(ctx);
}

pulse_graphic_state* state_from_app(pulse_app_t app) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world || ecs_id(GraphStateResource) == 0) return nullptr;
    const GraphStateResource* res = ecs_singleton_get(world, GraphStateResource);
    return res ? res->state : nullptr;
}

CGPUDeviceId get_device(pulse_app_t app) {
    const pulse_cgpu_renderer* renderer = pulse_cgpu_renderer_get(app);
    return renderer ? renderer->device : CGPUDeviceId{CGPU_NULLPTR};
}

bool is_upload_pending(pulse_app_t app, pulse_asset_handle handle) {
    pulse_graphic_state* state = state_from_app(app);
    if (!state) return false;
    if (!pulse_asset_handle_is_valid(handle)) return false;
    for (const auto& entry : state->pending_uploads) {
        if (entry.content == UPLOAD_TEXTURE && pulse_asset_handle_equals(pulse_graphic_texture_to_handle(entry.texture), handle)) return true;
        if (entry.content == UPLOAD_BUFFER && pulse_asset_handle_equals(pulse_graphic_buffer_to_handle(entry.buffer), handle)) return true;
    }
    return false;
}

void clear_upload_pending(pulse_app_t app, pulse_asset_handle handle) {
    pulse_graphic_state* state = state_from_app(app);
    if (!state) return;
    if (!pulse_asset_handle_is_valid(handle)) return;
    auto& vec = state->pending_uploads;
    for (size_t i = 0; i < vec.size(); ) {
        bool match = false;
        if (vec[i].content == UPLOAD_TEXTURE && pulse_asset_handle_equals(pulse_graphic_texture_to_handle(vec[i].texture), handle)) match = true;
        if (vec[i].content == UPLOAD_BUFFER && pulse_asset_handle_equals(pulse_graphic_buffer_to_handle(vec[i].buffer), handle)) match = true;
        if (match) {
            vec.erase(vec.begin() + i);
        } else {
            ++i;
        }
    }
}

} // namespace pulse_graphic_internal

using namespace pulse_graphic_internal;

extern "C" {

pulse_graphic_plugin_desc pulse_graphic_plugin_desc_default(void) {
    pulse_graphic_plugin_desc desc{};
    desc.struct_size = sizeof(pulse_graphic_plugin_desc);
    desc.version = PULSE_GRAPHIC_PLUGIN_DESC_VERSION;
    return desc;
}

pulse_result_t pulse_graphic_add_plugin(pulse_app_t app, const pulse_graphic_plugin_desc* desc) {
    if (!app) return PULSE_ERROR_INVALID_ARGUMENT;
    if (pulse_app_has_plugin(app, kPluginName)) return PULSE_ERROR_DUPLICATE_PLUGIN;

    pulse_graphic_state* state = new (std::nothrow) pulse_graphic_state();
    if (!state) return PULSE_ERROR_INTERNAL;

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
