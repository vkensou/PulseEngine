#include "graphic_internal.h"
#include <cstdlib>
#include <cstring>
#include <vector>

#include "renderer.h"

// For texture loading
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "ktx.h"

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

static pulse_result_t start_bytecode(const pulse_asset_load_task* ctx, void** out_state, void* user_data) {
    (void)ctx; (void)user_data;
    *out_state = nullptr;
    return PULSE_OK;
}

static pulse_asset_loader_status_t step_bytecode(
    const pulse_asset_load_task* ctx, void* state, void* out_asset,
    const char** out_error, void* user_data)
{
    (void)state; (void)user_data;
    auto* bc = static_cast<PulseBytecodeSlot*>(out_asset);
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

// ── Shader loader (dependency-based) ───────────────────────────
struct ShaderLoaderState {
    bool from_file;
};

static void destroy_shader_loader_state(void* state, void*) {
    delete static_cast<ShaderLoaderState*>(state);
}

static pulse_result_t start_shader_from_deps(const pulse_asset_load_task* ctx, void** out_state, void* user_data) {
    (void)user_data;
    auto* s = new ShaderLoaderState{};
    s->from_file = ctx->dependency_count > 0;
    *out_state = s;
    return PULSE_OK;
}

static pulse_asset_loader_status_t step_shader_from_deps(
    const pulse_asset_load_task* ctx, void* state, void* out_asset,
    const char** out_error, void* user_data)
{
    auto* sls = static_cast<ShaderLoaderState*>(state);
    if (!sls->from_file) {
        // Created via create_from_binary — slot already populated
        return PULSE_ASSET_LOADER_DONE;
    }
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    if (!device) { *out_error = "shader loader: no device"; return PULSE_ASSET_LOADER_FAILED; }

    // Acquire dep slots to get bytecode
    pulse_asset_ref vs_ref{}, fs_ref{};
    if (ctx->dependency_count < 2 ||
        !pulse_asset_acquire(ctx->app, ctx->dependency_handles[0], &vs_ref) ||
        !pulse_asset_acquire(ctx->app, ctx->dependency_handles[1], &fs_ref))
    {
        *out_error = "shader loader: missing deps";
        return PULSE_ASSET_LOADER_FAILED;
    }
    auto* vs_bc = static_cast<PulseBytecodeSlot*>(vs_ref.ptr);
    auto* fs_bc = static_cast<PulseBytecodeSlot*>(fs_ref.ptr);

    CGPUBlendStateDescriptor default_blend{};
    CGPUDepthStateDescriptor default_depth{};
    CGPURasterizerStateDescriptor default_raster{};
    auto cpp_shader = HGEGraphics::create_shader(
        device, vs_bc->data, static_cast<uint32_t>(vs_bc->size),
        fs_bc->data, static_cast<uint32_t>(fs_bc->size),
        default_blend, default_depth, default_raster);

    pulse_asset_release(ctx->app, &vs_ref);
    pulse_asset_release(ctx->app, &fs_ref);

    if (!cpp_shader) { *out_error = "shader loader: create_shader failed"; return PULSE_ASSET_LOADER_FAILED; }

    auto* data = static_cast<pulse_shader_data_t*>(out_asset);
    data->root_sig = cpp_shader->root_sig;
    data->vs = cpp_shader->vs;
    data->ps = cpp_shader->ps;
    data->blend_desc = cpp_shader->blend_desc;
    for (size_t i = 0; i < cpp_shader->blend_attachment_states.size() && i < 8; ++i)
        data->blend_attachments[i] = cpp_shader->blend_attachment_states[i];
    data->blend_desc.p_attachments = data->blend_attachments;
    data->blend_desc.attachment_count = (uint32_t)(cpp_shader->blend_attachment_states.size() < 8
        ? cpp_shader->blend_attachment_states.size() : 8);
    data->depth_desc = cpp_shader->depth_desc;
    data->rasterizer_state = cpp_shader->rasterizer_state;
    cpp_shader->root_sig = CGPU_NULLPTR;
    cpp_shader->vs.library = CGPU_NULLPTR;
    cpp_shader->ps.library = CGPU_NULLPTR;

    return PULSE_ASSET_LOADER_DONE;
}

// ── Compute shader loader (dependency-based) ───────────────────
static pulse_result_t start_compute_shader_from_deps(const pulse_asset_load_task* ctx, void** out_state, void* user_data) {
    (void)user_data;
    auto* s = new ShaderLoaderState{};
    s->from_file = ctx->dependency_count > 0;
    *out_state = s;
    return PULSE_OK;
}

static pulse_asset_loader_status_t step_compute_shader_from_deps(
    const pulse_asset_load_task* ctx, void* state, void* out_asset,
    const char** out_error, void* user_data)
{
    auto* sls = static_cast<ShaderLoaderState*>(state);
    if (!sls->from_file) return PULSE_ASSET_LOADER_DONE;
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    if (!device) { *out_error = "cs loader: no device"; return PULSE_ASSET_LOADER_FAILED; }

    pulse_asset_ref cs_ref{};
    if (ctx->dependency_count < 1 ||
        !pulse_asset_acquire(ctx->app, ctx->dependency_handles[0], &cs_ref))
    {
        *out_error = "cs loader: missing dep";
        return PULSE_ASSET_LOADER_FAILED;
    }
    auto* bc = static_cast<PulseBytecodeSlot*>(cs_ref.ptr);

    auto cpp_cs = HGEGraphics::create_compute_shader(device, bc->data, static_cast<uint32_t>(bc->size));
    pulse_asset_release(ctx->app, &cs_ref);
    if (!cpp_cs) { *out_error = "cs loader: create_compute_shader failed"; return PULSE_ASSET_LOADER_FAILED; }

    auto* data = static_cast<pulse_compute_shader_data_t*>(out_asset);
    data->root_sig = cpp_cs->root_sig;
    data->cs = cpp_cs->cs;
    cpp_cs->root_sig = CGPU_NULLPTR;
    cpp_cs->cs.library = CGPU_NULLPTR;
    return PULSE_ASSET_LOADER_DONE;
}

// ── Texture loader (dependency-based) ──────────────────────────
static pulse_result_t start_texture_from_deps(const pulse_asset_load_task* ctx, void** out_state, void* user_data) {
    (void)ctx; (void)user_data;
    *out_state = nullptr;
    return PULSE_OK;
}

static pulse_asset_loader_status_t step_texture_from_deps(
    const pulse_asset_load_task* ctx, void* state, void* out_asset,
    const char** out_error, void* user_data)
{
    (void)state;
    if (ctx->dependency_count < 1) {
        *out_error = "texture loader: no deps";
        return PULSE_ASSET_LOADER_FAILED;
    }
    CGPUDeviceId device = static_cast<CGPUDeviceId>(user_data);
    if (!device) { *out_error = "texture loader: no device"; return PULSE_ASSET_LOADER_FAILED; }

    pulse_asset_ref img_ref{};
    if (!pulse_asset_acquire(ctx->app, ctx->dependency_handles[0], &img_ref)) {
        *out_error = "texture loader: dep not found";
        return PULSE_ASSET_LOADER_FAILED;
    }
    auto* bc = static_cast<PulseBytecodeSlot*>(img_ref.ptr);

    auto* tex = static_cast<pulse_texture_data_t*>(out_asset);

    // Try ktx first
    ktxTexture* kt = nullptr;
    bool is_ktx = ktxTexture_CreateFromMemory(bc->data, bc->size, KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT, &kt) == KTX_SUCCESS;

    if (is_ktx) {
        ECGPUTextureFormat fmt = CGPU_TEXTURE_FORMAT_UNDEFINED;
        int comp = 0;
        if (kt->classId == ktxTexture1_c) {
            auto* k1 = (ktxTexture1*)kt;
            if (k1->glInternalformat == 0x8058) { fmt = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM; comp = 4; }
            else if (k1->glInternalformat == 0x1908) { fmt = CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB; comp = 4; }
            else if (k1->glInternalformat == 0x881A) { fmt = CGPU_TEXTURE_FORMAT_R16G16B16A16_SFLOAT; comp = 8; }
        } else if (kt->classId == ktxTexture2_c) {
            auto* k2 = (ktxTexture2*)kt;
            if (k2->vkFormat == 37) { fmt = CGPU_TEXTURE_FORMAT_R8G8B8A8_UNORM; comp = 4; }
            else if (k2->vkFormat == 23) { fmt = CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB; comp = 4; }
        }
        if (fmt == CGPU_TEXTURE_FORMAT_UNDEFINED || kt->isCompressed) {
            ktxTexture_Destroy(kt);
            pulse_asset_release(ctx->app, &img_ref);
            *out_error = "texture loader: unsupported ktx format";
            return PULSE_ASSET_LOADER_FAILED;
        }
        CGPUTextureDescriptor td{};
        td.width = kt->baseWidth;
        td.height = kt->baseHeight;
        td.depth = 1;
        td.array_size = 1;
        td.format = fmt;
        td.mip_levels = kt->numLevels;
        td.descriptors = CGPU_RESOURCE_TYPE_TEXTURE;
        td.start_state = CGPU_RESOURCE_STATE_UNDEFINED;

        auto cpp_tex = HGEGraphics::create_texture(device, td);
        if (!cpp_tex) {
            ktxTexture_Destroy(kt);
            pulse_asset_release(ctx->app, &img_ref);
            *out_error = "texture loader: create_texture failed";
            return PULSE_ASSET_LOADER_FAILED;
        }
        tex->handle = cpp_tex->handle;
        tex->view = cpp_tex->view;
        tex->width = (uint32_t)kt->baseWidth;
        tex->height = (uint32_t)kt->baseHeight;
        tex->depth = 1;
        tex->mip_levels = kt->numLevels;
        tex->format = fmt;
        cpp_tex->handle = CGPU_NULLPTR;
        cpp_tex->view = CGPU_NULLPTR;

        ktxTexture_Destroy(kt);
        pulse_asset_release(ctx->app, &img_ref);
        return PULSE_ASSET_LOADER_DONE;
    }

    // Fallback to stb_image
    int w = 0, h = 0, comp = 0;
    auto* pixels = stbi_load_from_memory(bc->data, (int)bc->size, &w, &h, &comp, 4);
    if (!pixels) {
        pulse_asset_release(ctx->app, &img_ref);
        *out_error = "texture loader: stb decode failed";
        return PULSE_ASSET_LOADER_FAILED;
    }

    CGPUTextureDescriptor td{};
    td.width = (uint64_t)w;
    td.height = (uint64_t)h;
    td.depth = 1;
    td.array_size = 1;
    td.format = CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB;
    td.mip_levels = 1;
    td.descriptors = CGPU_RESOURCE_TYPE_TEXTURE;
    td.start_state = CGPU_RESOURCE_STATE_UNDEFINED;

    auto cpp_tex = HGEGraphics::create_texture(device, td);
    if (!cpp_tex) {
        stbi_image_free(pixels);
        pulse_asset_release(ctx->app, &img_ref);
        *out_error = "texture loader: create_texture failed";
        return PULSE_ASSET_LOADER_FAILED;
    }
    tex->handle = cpp_tex->handle;
    tex->view = cpp_tex->view;
    tex->width = (uint32_t)w;
    tex->height = (uint32_t)h;
    tex->depth = 1;
    tex->mip_levels = 1;
    tex->format = CGPU_TEXTURE_FORMAT_R8G8B8A8_SRGB;

    // TODO: upload pixel data via staging buffer in upload callback
    stbi_image_free(pixels);

    cpp_tex->handle = CGPU_NULLPTR;
    cpp_tex->view = CGPU_NULLPTR;

    pulse_asset_release(ctx->app, &img_ref);
    return PULSE_ASSET_LOADER_DONE;
}

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
    pulse_graphic_internal::material_internal_destroy(ptr);
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
    register_type(PULSE_TYPE_BYTECODE, sizeof(PulseBytecodeSlot), alignof(PulseBytecodeSlot), destroy_bytecode);

    // Register loaders
    auto register_loader = [app](uint64_t type_id, const char* ext,
                                 pulse_asset_loader_start_fn start,
                                 pulse_asset_loader_step_fn step,
                                 pulse_asset_loader_finally_fn fin,
                                 void* user_data)
    {
        pulse_asset_loader_desc ld{};
        ld.struct_size = sizeof(pulse_asset_loader_desc);
        ld.version = PULSE_ASSET_LOADER_DESC_VERSION;
        ld.type_id = type_id;
        ld.extensions = ext;
        ld.start = start;
        ld.step = step;
        ld.finally = fin;
        ld.user_data = user_data;
        return pulse_asset_register_loader(app, &ld);
    };

    register_loader(PULSE_TYPE_BYTECODE, "spv,dxc,obj,gltf,dds,ktx,png,jpg,bmp,tga",
                    start_bytecode, step_bytecode, nullptr, nullptr);
    register_loader(PULSE_TYPE_SHADER, "vert",
                    start_shader_from_deps, step_shader_from_deps, destroy_shader_loader_state,
                    static_cast<void*>(const_cast<struct CGPUDevice*>(device)));
    register_loader(PULSE_TYPE_COMPUTE_SHADER, "comp",
                    start_compute_shader_from_deps, step_compute_shader_from_deps, destroy_shader_loader_state,
                    static_cast<void*>(const_cast<struct CGPUDevice*>(device)));
    register_loader(PULSE_TYPE_TEXTURE, "ktx,dds,png,jpg,bmp,tga",
                    start_texture_from_deps, step_texture_from_deps, nullptr,
                    static_cast<void*>(const_cast<struct CGPUDevice*>(device)));

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
    for (const auto& entry : state->pending_uploads) {
        if (entry.handle.index == handle.index) return true;
    }
    return false;
}

void mark_upload_pending(pulse_app_t app, pulse_asset_handle handle) {
    pulse_graphic_state* state = state_from_app(app);
    if (!state) return;
    state->upload_pending = true;
    pulse_graphic_state::UploadEntry entry{};
    entry.handle = handle;
    entry.is_texture = true;
    state->pending_uploads.push_back(entry);
}

void clear_upload_pending(pulse_app_t app, pulse_asset_handle handle) {
    pulse_graphic_state* state = state_from_app(app);
    if (!state) return;
    auto& vec = state->pending_uploads;
    for (size_t i = 0; i < vec.size(); ) {
        if (vec[i].handle.index == handle.index) {
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

bool pulse_graphic_is_available(pulse_app_t app, pulse_shader_t handle) {
    return pulse_asset_is_available(app, handle.asset);
}

} // extern "C"
