#include "renderer_internal.h"

#include <algorithm>
#include <string.h>
#include <cmath>
#include <utility>
#include "hash.h"
#include <optional>

namespace pulse_renderer_internal {

constexpr const char* kPluginName = "PulseRendererPlugin";

// ============================================================
// Math helpers
// ============================================================

static HMM_Mat4 build_view_matrix(const HMM_Mat4& world) {
    auto eye = HMM_M4GetTranslate(world);
    auto forward = HMM_M4GetForward(world);
    auto viewMat = HMM_LookAt2_LH(eye, forward, HMM_V3_Up);
    return viewMat;
}

// ============================================================
// ECS Systems
// ============================================================

// ExtractCamerasSystem: iterates all Camera + WorldTransform entities,
//   builds RendererView entries in the write packet.
void extract_cameras_system(ecs_iter_t* it) {
    pulse_renderer_state* state =
        static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    FrameRenderPacket& packet = state->write_packet();
    // NOTE: do not clear views here — this system may run once per matched
    // table in a frame, and sort_and_pack_system already clears the next
    // write packet after swapping.

    PulseCamera* cameras = ecs_field(it, PulseCamera, 0);
    PulseWorldTransform* world_transforms = ecs_field(it, PulseWorldTransform, 1);

    ecs_world_t* world = it->world;

    for (int i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        PulseCamera& cam = cameras[i];
        HMM_Mat4& world_mat = world_transforms[i].value;

        // Get window size from associated window entity
        int width = 800;
        int height = 600;
        if (cam.window_entity != 0) {
            const PulseWindow* win =
                static_cast<const PulseWindow*>(
                    ecs_get(world, cam.window_entity, PulseWindow));
            if (win && win->width > 0 && win->height > 0) {
                width = win->width;
                height = win->height;
            }
        }

        float aspect = (height > 0) ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;
        float fov_rad = cam.fov * HMM_DegToRad;

        packet.views.emplace_back(&packet.pool);
        RendererView& view = packet.views.back();
        view.camera_entity = entity;
        view.window_entity = cam.window_entity;
        view.view_matrix = build_view_matrix(world_mat);
        view.proj_matrix = HMM_Perspective_LH_RO(fov_rad, aspect, cam.near_plane, cam.far_plane);
        view.fov = cam.fov;
        view.near_plane = cam.near_plane;
        view.far_plane = cam.far_plane;
        view.width = width;
        view.height = height;
    }
}

// CollectRenderablesSystem: iterates all Renderable + WorldTransform entities,
//   adds RenderObject to every active camera view.
void collect_renderables_system(ecs_iter_t* it) {
    pulse_renderer_state* state =
        static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    FrameRenderPacket& packet = state->write_packet();
    if (packet.views.empty()) return;

    PulseRenderable* renderables = ecs_field(it, PulseRenderable, 0);
    PulseWorldTransform* world_transforms = ecs_field(it, PulseWorldTransform, 1);

    for (int i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        PulseRenderable& renderable = renderables[i];
        HMM_Mat4& world_mat = world_transforms[i].value;

        PulseShaderHandle shader = pulse_material_get_shader(state->app, renderable.material);
        uint64_t sort_key =
            (static_cast<uint64_t>(shader.index & 0xFFFFu) << 48) |
            (static_cast<uint64_t>(renderable.material.index & 0xFFFFu) << 32) |
            static_cast<uint64_t>(renderable.mesh.index);

        RenderObject obj = {
            .sort_key = sort_key,
            .entity = entity,
            .mesh = renderable.mesh,
            .material = renderable.material,
            .shader = shader,
            .world_matrix = world_mat,
        };

        // Add to all views (v0.1: no frustum culling)
        for (auto& view : packet.views) {
            view.render_objects.push_back(obj);
        }
    }
}

// SortAndPackSystem: sorts render objects per view and swaps double buffers.
void sort_and_pack_system(ecs_iter_t* it) {
    pulse_renderer_state* state =
        static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    FrameRenderPacket& packet = state->write_packet();

    // Sort render objects in each view by sort key
    for (auto& view : packet.views) {
        std::sort(view.render_objects.begin(), view.render_objects.end(),
            [](const RenderObject& a, const RenderObject& b) {
                return a.sort_key < b.sort_key;
            });
    }
}

void packets_swap_system(ecs_iter_t* it) {
    pulse_renderer_state* state =
        static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    // Swap double buffers
    state->swap_packets();

    ++state->frame_index;

    auto& packet = state->write_packet();
    packet.views.clear();
    packet.ubo_cache.frame_end();
    packet.ubo_cache.release_idle(state->frame_index);
}

// ============================================================
// Render Record Callback (registered with pulse_graphics)
// ============================================================

// Per-pass data passed to executable callback
struct ViewPassData {
    PulseAppId app;
    const RendererView* view;
    const pulse_renderer_state* state;
};

// Helper: get the property name mapped to a given data source type
static const char* get_mapped_name(const pulse_renderer_state* state, EPulseRendererPropertyType type) {
    if ((int)type >= 0 && (int)type < PULSE_RENDERER_PROPERTY_TYPE_COUNT && state->property_names[(int)type])
        return state->property_names[(int)type];
    return nullptr;
}

static void render_view_executable(PulseRenderPassEncoder* encoder, void* userdata) {
    ViewPassData* pass_data = static_cast<ViewPassData*>(userdata);
    if (!encoder || !pass_data || !pass_data->view) return;

    const RendererView& view = *pass_data->view;
    PulseAppId app = pass_data->app;

    size_t obj_count = view.render_objects.size();
    if (obj_count == 0) return;

    for (size_t idx = 0; idx < obj_count; ++idx) {
        const RenderObject& obj = view.render_objects[idx];

        PulseMaterialHandle material = obj.material;
        PulseMeshHandle mesh = obj.mesh;

        PulseShaderHandle shader = obj.shader;
        if (shader.index == 0)
            continue;

        for (size_t i = obj.ubo_start; i < obj.ubo_end; ++i) {
            const auto& col = view.ubo_columns[i];
            const auto& block = view.blocks[col.block_ref.index];
            pulse_render_pass_encoder_set_global_buffer_offset(
                encoder, block.gpu_handle, (uint32_t)col.set, col.binding,
                col.block_ref.offset, col.block_ref.size);
        }

        pulse_render_pass_encoder_draw(encoder, material, mesh);
    }
}

// Align a byte size up to the given byte alignment (e.g. UBO offset alignment)
static uint32_t align_up(uint32_t value, uint32_t alignment) {
    return (value + alignment - 1) / alignment * alignment;
}

static GpuBlockRef alloc_gpu_block(RendererView& view, UboBlockCache& ubo_cache, uint32_t frame_index, uint32_t size, uint32_t ubo_alignment) {
    size = align_up(size, ubo_alignment);
    for (size_t i = 0; i < view.blocks.size(); ++i) {
        size_t index = view.blocks.size() - i - 1;
        auto& block = view.blocks[index];
        if (block.size >= block.used + size) {
            auto offset = block.used;
            auto ptr = block.cpu_data + offset;
            block.used += size;
            return { index, offset, size, ptr };
        }
    }

    view.blocks.emplace_back();
    auto& block = view.blocks.back();
    block.cpu_data = ubo_cache.acquire(size, frame_index, block.size);
    block.used = 0;
    auto offset = block.used;
    auto ptr = block.cpu_data + offset;
    block.used += size;
    return { view.blocks.size() - 1, offset, size, ptr };
}

static std::optional<size_t> find_cached_ubo_column(
    RendererView& view,
    RenderObject& obj,
    const PulseUboInfo& info
    ) {
    for (size_t i = 0; i < view.ubo_columns.size(); ++i) {
        auto& col = view.ubo_columns[i];
        if (col.layout_hash == info.layout_hash && !info.per_draw
            && ((!info.material_managed) || (col.material.index == obj.material.index && col.material.generation == obj.material.generation))) {
            return i;
        }
    }
    return {};
}

static size_t build_ubo_column_for_shader2(
    PulseAppId app,
    const pulse_renderer_state* state,
    RendererView& view,
    UboBlockCache& ubo_cache,
    uint32_t frame_index,
    HMM_Mat4& vp,
    RenderObject& obj,
    PulseShaderHandle shader,
    uint32_t ubo_info_index,
    const PulseUboInfo& info,
    uint32_t ubo_alignment)
{
    uint32_t ubo_size = info.size;
    ubo_size = align_up(ubo_size, ubo_alignment);

    RendererUboColumn col = {};
    col.material = obj.material;
    col.shader = shader;
    col.ubo_info_index = ubo_info_index;
    col.layout_hash = info.layout_hash;
    col.set = info.set;
    col.binding = info.binding;

    // 尝试重用
    if (!info.per_draw)
    {
        auto s = find_cached_ubo_column(view, obj, info);
        // 找到了
        if (s.has_value()) {
            auto& cachedCol = view.ubo_columns[s.value()];
            col.block_ref = cachedCol.block_ref;
            view.ubo_columns.push_back(std::move(col));
            return view.ubo_columns.size() - 1;
        }
    }

    // 分配
    col.block_ref = alloc_gpu_block(view, ubo_cache, frame_index, info.size, ubo_alignment);

    // copy from
    if (info.material_managed) {
        auto mat_ubo_data = pulse_material_get_ubo_column(app, obj.material, ubo_info_index);
        memcpy(col.block_ref.ptr, mat_ubo_data, info.size);
    }

    // copy renderer property
    for (uint32_t p = 0; p < pulse_shader_get_shader_property_count(app, shader); ++p) {
        const auto& prop = pulse_shader_get_shader_property(app, shader, p);
        if (!prop.name || prop.set != info.set || prop.binding != info.binding) continue;
        if (prop.role != PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL) continue;

        if (prop.type == PULSE_SHADER_PROPERTY_TYPE_MAT4 && strcmp(prop.name, get_mapped_name(state, PULSE_RENDERER_PROPERTY_TYPE_VP_MATRIX)) == 0) {
            memcpy(col.block_ref.ptr + prop.offset, &vp, prop.size);
        }
        else if (prop.type == PULSE_SHADER_PROPERTY_TYPE_MAT4 && strcmp(prop.name, get_mapped_name(state, PULSE_RENDERER_PROPERTY_TYPE_MODEL_MATRIX)) == 0) {
            memcpy(col.block_ref.ptr + prop.offset, &obj.world_matrix, prop.size);
        }
    }

    view.ubo_columns.push_back(std::move(col));
    return view.ubo_columns.size() - 1;
}

static void record_renderer_callback(
    PulseAppId app,
    PulseRenderGraphId graph,
    void* user_data)
{
    pulse_renderer_state* state =
        static_cast<pulse_renderer_state*>(user_data);
    if (!state || !graph) return;

    FrameRenderPacket& packet = state->read_packet_mutable();
    if (packet.views.empty()) return;

    for (auto& view : packet.views) {
        if (view.window_entity == 0) continue;

        PulseRGTextureHandle target_handle =
            pulse_import_window_backbuffer(app, graph, view.window_entity);
        if (!pulse_rgtexture_handle_is_valid(target_handle))
            continue;

        // Build renderer-managed UBO columns per unique shader
        view.ubo_columns.clear();

        HMM_Mat4 vp = HMM_Mul(view.proj_matrix, view.view_matrix);

        for (auto& obj : view.render_objects) {
            PulseShaderHandle shader = obj.shader;
            if (shader.index == 0) continue;
            size_t ubo_start = 0, ubo_count = 0;
            for (uint32_t u = 0; u < pulse_shader_get_ubo_info_count(app, shader); ++u) {
                const auto& info = pulse_shader_get_ubo_info(app, shader, u);
                if (!info.renderer_managed) continue;
                auto buffer_alloc = build_ubo_column_for_shader2(app, state, view, packet.ubo_cache, state->frame_index, vp, obj, shader, u, info, state->ubo_alignment);
                if (ubo_count == 0) ubo_start = buffer_alloc;
                ++ubo_count;
            }
            obj.ubo_start = ubo_start;
            obj.ubo_end = ubo_start + ubo_count;
        }

        for (auto& block : view.blocks) {
            if (block.used == 0) continue;
            block.gpu_handle = pulse_render_graph_declare_uniform_buffer_quick(
                graph, block.used, block.cpu_data);
        }

        // Build render pass
        char pass_name[64];
        snprintf(pass_name, sizeof(pass_name), "RendererView_%llu",
                 static_cast<unsigned long long>(view.camera_entity));
        PulseRenderPassBuilder pass =
            pulse_render_graph_add_render_pass(graph, pass_name);

        pulse_render_pass_builder_add_color_attachment(
            &pass, target_handle,
            CGPU_LOAD_ACTION_CLEAR,
            0xff000000,  // black clear color
            CGPU_STORE_ACTION_STORE);

        // Register UBO handles as used in this pass
        for (const auto& block : view.blocks) {
            if (pulse_rgbuffer_handle_is_valid(block.gpu_handle))
                pulse_render_pass_builder_use_buffer(&pass, block.gpu_handle);
        }

        // Set executable callback
        ViewPassData* passdata = nullptr;
        pulse_render_pass_builder_set_executable(
            &pass,
            render_view_executable,
            sizeof(ViewPassData),
            reinterpret_cast<void**>(&passdata));

        if (passdata) {
            passdata->app = app;
            passdata->view = &view;
            passdata->state = state;
        }
    }
}

// ============================================================
// System installation
// ============================================================

void install_renderer_systems(ecs_world_t* world, pulse_renderer_state* state) {
    if (!world || !state) return;

    ecs_entity_t prev_system = 0;

    // ExtractCameras: Camera + WorldTransform → RendererView
    {
        ecs_entity_desc_t entity_desc = {};
        entity_desc.name = "PulseRendererExtractCameras";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc = {};
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.query.terms[0].id = ecs_id(PulseCamera);
        desc.query.terms[0].inout = EcsIn;
        desc.query.terms[1].id = ecs_id(PulseWorldTransform);
        desc.query.terms[1].inout = EcsIn;
        desc.callback = extract_cameras_system;
        desc.ctx = state;
        ecs_system_init(world, &desc);

        // Ensure this runs after PropagateWorldTransform (registered by pulse_transform)
        ecs_entity_t propagate = ecs_lookup(world, "PropagateWorldTransform");
        if (propagate != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, propagate);
        }
        prev_system = entity;
        state->extract_cameras_system = entity;
    }

    // CollectRenderables: Renderable + WorldTransform → RenderObject
    {
        ecs_entity_desc_t entity_desc = {};
        entity_desc.name = "PulseRendererCollectRenderables";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc = {};
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.query.terms[0].id = ecs_id(PulseRenderable);
        desc.query.terms[0].inout = EcsIn;
        desc.query.terms[1].id = ecs_id(PulseWorldTransform);
        desc.query.terms[1].inout = EcsIn;
        desc.callback = collect_renderables_system;
        desc.ctx = state;
        ecs_system_init(world, &desc);

        // Must run after ExtractCameras
        if (prev_system != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, prev_system);
        }
        prev_system = entity;
        state->collect_renderables_system = entity;
    }

    // SortAndPack: sorts + swaps double buffers
    {
        ecs_entity_desc_t entity_desc = {};
        entity_desc.name = "PulseRendererSortAndPack";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc = {};
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.callback = sort_and_pack_system;
        desc.ctx = state;
        desc.immediate = true;  // run system — no query terms needed
        ecs_system_init(world, &desc);

        // Must run after CollectRenderables
        if (prev_system != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, prev_system);
        }
        state->sort_and_pack_system = entity;
    }

    // PacketsSwap
    {
        ecs_entity_desc_t entity_desc = {};
        entity_desc.name = "PulseRendererPacketsSwap";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc = {};
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.callback = packets_swap_system;
        desc.ctx = state;
        desc.immediate = true;  // run system — no query terms needed
        ecs_system_init(world, &desc);

        // Must run after CollectRenderables
        if (prev_system != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, prev_system);
        }
        state->packets_swap_system = entity;
    }
}

// ============================================================
// Plugin lifecycle
// ============================================================

EPulsePluginBuildResult renderer_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* state = static_cast<pulse_renderer_state*>(ctx);
    if (!state) {
        return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;
    }

    state->app = app;
    state->assetSystem = pulse_get_asset_system(app);

    // Register ECS components
    register_renderer_components(world);

    // Store state as singleton for later retrieval
    pulse_renderer_state_resource state_res = {};
    state_res.state = state;
    ecs_singleton_set_ptr(world, pulse_renderer_state_resource, &state_res);

    // Install ECS systems
    install_renderer_systems(world, state);

    const char *per_draw_shader_properties[] = {
        "wMatrix",
    };
    pulse_set_per_draw_shader_properties(app, sizeof(per_draw_shader_properties) / sizeof(const char*), per_draw_shader_properties);

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult renderer_plugin_post_build(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_renderer_state*>(ctx);
    if (!state) return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;

    // Query the device UBO offset alignment once and cache it.
    // Runs after every plugin's build phase, so PulseRenderer (and its
    // adapter) are guaranteed to exist by this point.
    const PulseRenderer* gfx_renderer = pulse_get_renderer(app);
    if (gfx_renderer && gfx_renderer->adapter) {
        const CGPUAdapterDetail* detail =
            cgpu_adapter_query_adapter_detail(gfx_renderer->adapter);
        if (detail && detail->uniform_buffer_alignment > 0)
            state->ubo_alignment = detail->uniform_buffer_alignment;
    }

    // Register render record callback with pulse_graphics
    // This must happen in post_build because pulse_graphics systems
    // are installed during its build phase.
    PulseRenderRecordCallbackDesc cb_desc = {};
    cb_desc.callback = record_renderer_callback;
    cb_desc.user_data = state;
    cb_desc.priority = 100;  // Run before user callbacks (lower = earlier)

    EPulseResult result = pulse_add_render_record_callback(app, &cb_desc);
    if (result == PULSE_RESULT_OK) {
        state->record_callback_registered = true;
    }
    return result == PULSE_RESULT_OK ? PULSE_PLUGIN_BUILD_RESULT_OK : PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
}

void renderer_plugin_shutdown(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_renderer_state*>(ctx);
    if (!state) return;

    ecs_world_t* world = pulse_app_world(app);

    // Unregister the render record callback (its user_data points at state)
    if (state->record_callback_registered) {
        pulse_remove_render_record_callback(app, record_renderer_callback);
        state->record_callback_registered = false;
    }

    // Delete ECS systems whose ctx points at state
    if (world && state->extract_cameras_system && ecs_is_alive(world, state->extract_cameras_system))
        ecs_delete(world, state->extract_cameras_system);
    if (world && state->collect_renderables_system && ecs_is_alive(world, state->collect_renderables_system))
        ecs_delete(world, state->collect_renderables_system);
    if (world && state->sort_and_pack_system && ecs_is_alive(world, state->sort_and_pack_system))
        ecs_delete(world, state->sort_and_pack_system);
    if (world && state->packets_swap_system && ecs_is_alive(world, state->packets_swap_system))
        ecs_delete(world, state->packets_swap_system);

    // Remove the state singleton
    if (world && ecs_id(pulse_renderer_state_resource) != 0) {
        ecs_singleton_remove(world, pulse_renderer_state_resource);
    }

    delete state;
}

} // namespace pulse_renderer_internal

using namespace pulse_renderer_internal;

// ============================================================
// Public C API
// ============================================================

extern "C" {

EPulseAppAddPluginResult pulse_add_renderer_plugin(PulseAppId app) {
    if (!app) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    auto* state = new (std::nothrow) pulse_renderer_state();
    if (!state) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INTERNAL;
    }

    // Set default property name mappings
    state->property_names[PULSE_RENDERER_PROPERTY_TYPE_VP_MATRIX] = "vpMatrix";
    state->property_names[PULSE_RENDERER_PROPERTY_TYPE_MODEL_MATRIX] = "wMatrix";

    PulsePluginDesc plugin_desc = {
        sizeof(PulsePluginDesc),
        PULSE_PLUGIN_DESC_VERSION,
        kPluginName,
        state,
        renderer_plugin_build,
        renderer_plugin_post_build,
        renderer_plugin_shutdown,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

void pulse_set_shader_property_name_mapper(PulseAppId app, EPulseRendererPropertyType type, const char* name)
{
    if (!app || !name) return;
    ecs_world_t* world = pulse_app_world(app);
    if (!world) return;

    auto* state = pulse_renderer_internal::state_from_app(app);
    if (!state) return;

    if ((int)type >= 0 && (int)type < PULSE_RENDERER_PROPERTY_TYPE_COUNT) {
        state->property_names[(int)type] = name;
    }
}

} // extern "C"
