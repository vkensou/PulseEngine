#include "../graphics_internal.h"

#include <cstdlib>
#include <cstring>

namespace pulse_graphics_internal {

namespace {

struct PulseBytecodeSlot {
    uint8_t* data = nullptr;
    uint64_t size = 0;
};

void destroy_bytecode(void* ptr, void*) {
    auto* bc = static_cast<PulseBytecodeSlot*>(ptr);
    std::free(bc->data);
    bc->data = nullptr;
    bc->size = 0;
}

pulse_asset_loader_status_t step_bytecode(
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

void destroy_sampler(void* ptr, void* user_data) {
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    pulse_sampler_data_t* data = static_cast<pulse_sampler_data_t*>(ptr);
    if (data->handle) cgpu_device_free_sampler(device, data->handle);
}

} // namespace

void register_graphics_asset_types_and_loaders(pulse_app_t app, CGPUDeviceId device) {
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
}

} // namespace pulse_graphics_internal
