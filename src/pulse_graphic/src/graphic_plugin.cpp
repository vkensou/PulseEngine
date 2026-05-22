#include "graphic_internal.h"

namespace pulse_graphic_internal {

const char* kPluginName = "PulseGraphicPlugin";

static void destroy_shader(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_shader_data_t* data = static_cast<pulse_shader_data_t*>(ptr);
    if (data->root_sig) cgpu_device_free_root_signature(device, data->root_sig);
    if (data->vs.library) cgpu_device_free_shader_library(device, data->vs.library);
    if (data->ps.library) cgpu_device_free_shader_library(device, data->ps.library);
}

static void destroy_compute_shader(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_compute_shader_data_t* data = static_cast<pulse_compute_shader_data_t*>(ptr);
    if (data->root_sig) cgpu_device_free_root_signature(device, data->root_sig);
    if (data->cs.library) cgpu_device_free_shader_library(device, data->cs.library);
}

static void destroy_mesh(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_mesh_data_t* data = static_cast<pulse_mesh_data_t*>(ptr);
    if (data->vertex_buffer) cgpu_device_free_buffer(device, data->vertex_buffer);
    if (data->index_buffer) cgpu_device_free_buffer(device, data->index_buffer);
}

static void destroy_texture(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_texture_data_t* data = static_cast<pulse_texture_data_t*>(ptr);
    if (data->view) cgpu_device_free_texture_view(device, data->view);
    if (data->handle) cgpu_device_free_texture(device, data->handle);
}

static void destroy_buffer(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_buffer_data_t* data = static_cast<pulse_buffer_data_t*>(ptr);
    if (data->handle) cgpu_device_free_buffer(device, data->handle);
}

static void destroy_sampler(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_sampler_data_t* data = static_cast<pulse_sampler_data_t*>(ptr);
    if (data->handle) cgpu_device_free_sampler(device, data->handle);
}

static void destroy_material(void* ptr, void*) {
    pulse_material_data_t* data = static_cast<pulse_material_data_t*>(ptr);
    (void)data;
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

    register_type(PULSE_TYPE_SHADER, sizeof(pulse_shader_data_t), alignof(pulse_shader_data_t), destroy_shader);
    register_type(PULSE_TYPE_COMPUTE_SHADER, sizeof(pulse_compute_shader_data_t), alignof(pulse_compute_shader_data_t), destroy_compute_shader);
    register_type(PULSE_TYPE_MESH, sizeof(pulse_mesh_data_t), alignof(pulse_mesh_data_t), destroy_mesh);
    register_type(PULSE_TYPE_TEXTURE, sizeof(pulse_texture_data_t), alignof(pulse_texture_data_t), destroy_texture);
    register_type(PULSE_TYPE_BUFFER, sizeof(pulse_buffer_data_t), alignof(pulse_buffer_data_t), destroy_buffer);
    register_type(PULSE_TYPE_MATERIAL, sizeof(pulse_material_data_t), alignof(pulse_material_data_t), destroy_material);
    register_type(PULSE_TYPE_SAMPLER, sizeof(pulse_sampler_data_t), alignof(pulse_sampler_data_t), destroy_sampler);

    GraphStateResource res{gstate};
    ecs_singleton_set_ptr(world, GraphStateResource, &res);

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
    pulse_graphic_state* st = state_from_app(app);
    if (!st) return false;
    uint64_t key = (uint64_t)handle.type_id << 32 | handle.index;
    auto it = st->upload_pending_map.find(key);
    return it != st->upload_pending_map.end() && it->second;
}

void mark_upload_pending(pulse_app_t app, pulse_asset_handle handle) {
    pulse_graphic_state* st = state_from_app(app);
    if (!st) return;
    uint64_t key = (uint64_t)handle.type_id << 32 | handle.index;
    st->upload_pending_map[key] = true;
}

void clear_upload_pending(pulse_app_t app, pulse_asset_handle handle) {
    pulse_graphic_state* st = state_from_app(app);
    if (!st) return;
    uint64_t key = (uint64_t)handle.type_id << 32 | handle.index;
    st->upload_pending_map[key] = false;
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

bool pulse_graphic_is_available(pulse_app_t app, pulse_shader_t handle) {
    return pulse_asset_is_available(app, handle.asset);
}

} // extern "C"
