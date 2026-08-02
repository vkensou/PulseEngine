#include "renderer_internal.h"

#include <algorithm>
#include <string.h>
#include <cmath>
#include <utility>
#include "hash.h"

namespace pulse_renderer_internal {

constexpr const char* kPluginName = "PulseRendererPlugin";

// ============================================================
// Math helpers
// ============================================================

// Build a perspective projection matrix (right-handed, standard)
static HMM_Mat4 build_perspective(float fov_radians, float aspect,
                                   float near_plane, float far_plane) {
    auto proj = HMM_Perspective_LH_RO(fov_radians, aspect, near_plane, far_plane);
    return proj;
}

// Fast inverse for a TRS matrix (affine, no scale shear)
// For a proper camera, we invert the world matrix.
// Using HMM_Mat4 inverse for correctness.
static HMM_Mat4 build_view_matrix(const HMM_Mat4& world) {
    auto eye = HMM_M4GetTranslate(world);
    auto forward = HMM_M4GetForward(world);
    (void)forward;
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
    packet.views.clear();

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

        RendererView view;
        view.camera_entity = entity;
        view.window_entity = cam.window_entity;
        view.view_matrix = build_view_matrix(world_mat);
        view.proj_matrix = build_perspective(fov_rad, aspect, cam.near_plane, cam.far_plane);
        view.fov = cam.fov;
        view.near_plane = cam.near_plane;
        view.far_plane = cam.far_plane;
        view.width = width;
        view.height = height;

        packet.views.push_back(view);
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

        // Build sort key: material_index in high 32 bits, mesh_index in low 32 bits
        uint64_t sort_key =
            (static_cast<uint64_t>(renderable.material.index) << 32) |
            static_cast<uint64_t>(renderable.mesh.index);

        RenderObject obj;
        obj.sort_key = sort_key;
        obj.entity = entity;
        obj.mesh = renderable.mesh;
        obj.material = renderable.material;
        obj.world_matrix = world_mat;

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

    // Swap double buffers
    state->swap_packets();

    // Clear the new write packet for next frame
    state->write_packet().views.clear();
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

// Helper: find the ubo_info for a renderer-managed UBO by property name
static const pulse_shader_ubo_info_t* find_managed_ubo(const PulseShaderData* shader, const char* prop_name) {
    if (!shader || !prop_name) return nullptr;
    const auto* prop = pulse_find_shader_property(shader, prop_name);
    if (!prop) return nullptr;
    for (uint32_t i = 0; i < shader->ubo_info_count; ++i) {
        if (shader->p_ubo_infos[i].set == prop->set && shader->p_ubo_infos[i].binding == prop->binding)
            return &shader->p_ubo_infos[i];
    }
    return nullptr;
}

static void render_view_executable(PulseRenderPassEncoder* encoder, void* userdata) {
    ViewPassData* pass_data = static_cast<ViewPassData*>(userdata);
    if (!encoder || !pass_data || !pass_data->view) return;

    const RendererView& view = *pass_data->view;
    PulseAppId app = pass_data->app;

    pulse_render_pass_encoder_set_viewport(
        encoder, 0.0f, 0.0f,
        static_cast<float>(view.width), static_cast<float>(view.height),
        0.0f, 1.0f);
    pulse_render_pass_encoder_set_scissor(
        encoder, 0, 0,
        static_cast<uint32_t>(view.width), static_cast<uint32_t>(view.height));

    size_t obj_count = view.render_objects.size();
    if (obj_count == 0) return;

    const PulseShaderData* last_shader = nullptr;

    for (size_t idx = 0; idx < obj_count; ++idx) {
        const RenderObject& obj = view.render_objects[idx];

        PulseMaterial material_ref = {};
        if (!pulse_acquire_material(app, obj.material, &material_ref))
            continue;

        PulseMesh mesh_ref = {};
        if (!pulse_acquire_mesh(app, obj.mesh, &mesh_ref)) {
            pulse_release_material(app, &material_ref);
            continue;
        }

        auto* mat_data = static_cast<PulseMaterialData*>(material_ref.ptr);
        auto* shader = mat_data ? mat_data->shader : nullptr;
        if (!shader) {
            pulse_release_mesh(app, &mesh_ref);
            pulse_release_material(app, &material_ref);
            continue;
        }

        bool shader_changed = (shader != last_shader);

        // Bind renderer-managed UBO columns matching this shader
        for (const auto& col : view.ubo_columns) {
            if (col.shader != shader) continue;
            if (!pulse_rgbuffer_handle_is_valid(col.gpu_handle))
                continue;

            if (col.is_per_draw) {
                // Per-draw UBO: bind every draw with current offset
                uint64_t obj_offset = idx * col.stride;
                pulse_render_pass_encoder_set_global_buffer_offset(
                    encoder, col.gpu_handle, (uint32_t)col.set, col.binding,
                    obj_offset, col.stride);
            } else if (shader_changed) {
                // Per-pass UBO: bind once when entering this shader
                pulse_render_pass_encoder_set_global_buffer_handle(
                    encoder, col.gpu_handle, (uint32_t)col.set, col.binding);
            }
        }

        last_shader = shader;

        pulse_render_pass_encoder_draw(encoder, material_ref, mesh_ref);

        pulse_release_mesh(app, &mesh_ref);
        pulse_release_material(app, &material_ref);
    }
}

// Helper: build a single renderer-managed UBO column for a given shader+ubo_info
static void build_ubo_column_for_shader(
    const pulse_renderer_state* state,
    PulseRenderGraphId graph,
    RendererView& view,
    const PulseShaderData* shader,
    const pulse_shader_ubo_info_t& info)
{
    RendererUboColumn col = {};
    col.shader = shader;
    col.set = info.set;
    col.binding = info.binding;
    col.layout_hash = info.layout_hash;

    bool has_pass = false;
    bool has_draw = false;
    uint32_t ubo_size = info.ubo_size;

    for (uint32_t p = 0; p < shader->property_count; ++p) {
        const auto& prop = shader->p_properties[p];
        if (prop.set != info.set || prop.binding != info.binding) continue;
        if (prop.role != PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL) continue;

        const char* vp_name = get_mapped_name(state, PULSE_RENDERER_PROPERTY_TYPE_VP_MATRIX);
        const char* model_name = get_mapped_name(state, PULSE_RENDERER_PROPERTY_TYPE_MODEL_MATRIX);
        if (vp_name && prop.name && strcmp(prop.name, vp_name) == 0) has_pass = true;
        if (model_name && prop.name && strcmp(prop.name, model_name) == 0) has_draw = true;
    }

    if (ubo_size == 0) return;

    if (has_draw) {
        col.is_per_draw = true;
        col.stride = ubo_size;
        size_t obj_count = view.render_objects.size();
        col.cpu_data.resize(ubo_size * obj_count, 0);

        for (size_t o = 0; o < obj_count; ++o) {
            for (uint32_t p = 0; p < shader->property_count; ++p) {
                const auto& prop = shader->p_properties[p];
                if (prop.set != info.set || prop.binding != info.binding) continue;
                if (prop.role != PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL) continue;

                const char* model_name = get_mapped_name(state, PULSE_RENDERER_PROPERTY_TYPE_MODEL_MATRIX);
                if (model_name && prop.name && strcmp(prop.name, model_name) == 0) {
                    uint32_t copy_size = prop.size > 0 ? prop.size : sizeof(HMM_Mat4);
                    memcpy(col.cpu_data.data() + o * ubo_size + prop.offset,
                           &view.render_objects[o].world_matrix, copy_size);
                }
            }
        }
    }

    if (has_pass) {
        if (!has_draw) {
            col.is_per_draw = false;
            col.stride = ubo_size;
            col.cpu_data.resize(ubo_size, 0);
        }
        HMM_Mat4 vp = HMM_Mul(view.proj_matrix, view.view_matrix);
        size_t vp_slot_count = has_draw ? view.render_objects.size() : 1;
        for (size_t o = 0; o < vp_slot_count; ++o) {
            for (uint32_t p = 0; p < shader->property_count; ++p) {
                const auto& prop = shader->p_properties[p];
                if (prop.set != info.set || prop.binding != info.binding) continue;
                if (prop.role != PULSE_SHADER_PROPERTY_ROLE_NON_MATERIAL) continue;

                const char* vp_name = get_mapped_name(state, PULSE_RENDERER_PROPERTY_TYPE_VP_MATRIX);
                if (vp_name && prop.name && strcmp(prop.name, vp_name) == 0) {
                    uint32_t copy_size = prop.size > 0 ? prop.size : sizeof(HMM_Mat4);
                    memcpy(col.cpu_data.data() + o * ubo_size + prop.offset, &vp, copy_size);
                }
            }
        }
    }

    // Compute data_hash from the filled byte buffer
    col.data_hash = col.layout_hash;
    HGEGraphics::hash_combine(col.data_hash, HGEGraphics::murmur3(
        (const uint32_t*)col.cpu_data.data(),
        (uint32_t)col.cpu_data.size() / 4, 0));

    // Declare GPU handle via rendergraph
    col.gpu_handle = pulse_render_graph_declare_uniform_buffer_quick(
        graph, (uint32_t)col.cpu_data.size(), (void*)col.cpu_data.data());

    view.ubo_columns.push_back(std::move(col));
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

        // Collect unique shaders from all renderables
        struct PulseShaderData* all_shaders[64] = {};
        uint32_t shader_count = 0;
        for (auto& obj : view.render_objects) {
            PulseMaterial mat_ref = {};
            if (!pulse_acquire_material(app, obj.material, &mat_ref)) continue;
            auto* mat_data = static_cast<PulseMaterialData*>(mat_ref.ptr);
            auto* sdr = mat_data ? mat_data->shader : nullptr;
            if (sdr) {
                bool found = false;
                for (uint32_t si = 0; si < shader_count; ++si) {
                    if (all_shaders[si] == sdr) { found = true; break; }
                }
                if (!found && shader_count < 64)
                    all_shaders[shader_count++] = sdr;
            }
            pulse_release_material(app, &mat_ref);
        }

        // Build UBO columns for each unique shader
        for (uint32_t si = 0; si < shader_count; ++si) {
            auto* shader = all_shaders[si];
            for (uint32_t u = 0; u < shader->ubo_info_count; ++u) {
                const auto& info = shader->p_ubo_infos[u];
                if (!info.renderer_managed) continue;
                build_ubo_column_for_shader(state, graph, view, shader, info);
            }
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
        for (const auto& col : view.ubo_columns) {
            if (pulse_rgbuffer_handle_is_valid(col.gpu_handle))
                pulse_render_pass_builder_use_buffer(&pass, col.gpu_handle);
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
    }
}

// ============================================================
// Plugin lifecycle
// ============================================================

EPulseResult renderer_plugin_build(PulseAppId app, void* ctx) {
    ecs_world_t* world = pulse_app_world(app);
    if (!world) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto* state = static_cast<pulse_renderer_state*>(ctx);
    if (!state) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    state->app = app;

    // Register ECS components
    register_renderer_components(world);

    // Store state as singleton for later retrieval
    pulse_renderer_state_resource state_res = {};
    state_res.state = state;
    ecs_singleton_set_ptr(world, pulse_renderer_state_resource, &state_res);

    // Install ECS systems
    install_renderer_systems(world, state);

    return PULSE_RESULT_OK;
}

EPulseResult renderer_plugin_post_build(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_renderer_state*>(ctx);
    if (!state) return PULSE_RESULT_ERROR_INVALID_ARGUMENT;

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
    return result;
}

void renderer_plugin_shutdown(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_renderer_state*>(ctx);
    if (!state) return;

    // Note: pulse_remove_render_record_callback is declared but not yet
    // implemented in pulse_graphics. The callback will be cleaned up
    // automatically when pulse_graphics shuts down.
    state->record_callback_registered = false;

    delete state;
}

} // namespace pulse_renderer_internal

using namespace pulse_renderer_internal;

// ============================================================
// Public C API
// ============================================================

extern "C" {

EPulseResult pulse_add_renderer_plugin(PulseAppId app) {
    if (!app) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (pulse_app_has_plugin(app, kPluginName)) {
        return PULSE_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    auto* state = new (std::nothrow) pulse_renderer_state();
    if (!state) {
        return PULSE_RESULT_ERROR_INTERNAL;
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

    EPulseResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
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
