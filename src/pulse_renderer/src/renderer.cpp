#include "renderer_internal.h"

#include <algorithm>
#include <string.h>
#include <cmath>

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

        view.cam_data.view_proj = HMM_Mul(view.proj_matrix, view.view_matrix);

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

        ObjectUniformData obj_ubo_data;
        obj_ubo_data.model = world_mat;

        // Add to all views (v0.1: no frustum culling)
        for (auto& view : packet.views) {
            view.render_objects.push_back(obj);
            view.render_data.push_back(obj_ubo_data);
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
    pulse_buffer_handle_t cam_ubo;
    pulse_buffer_handle_t object_ubo;
};

static void render_view_executable(pulse_renderpass_encoder_t* encoder, void* userdata) {
    ViewPassData* pass_data = static_cast<ViewPassData*>(userdata);
    if (!encoder || !pass_data || !pass_data->view) return;

    const RendererView& view = *pass_data->view;

    pulse_renderpass_encoder_set_viewport(
        encoder, 0.0f, 0.0f,
        static_cast<float>(view.width), static_cast<float>(view.height),
        0.0f, 1.0f);
    pulse_renderpass_encoder_set_scissor(
        encoder, 0, 0,
        static_cast<uint32_t>(view.width), static_cast<uint32_t>(view.height));

    size_t obj_count = view.render_objects.size();
    if (obj_count == 0) return;

    PulseAppId app = pass_data->app;

    // Bind camera VP UBO (set 0, binding 0)
    pulse_renderpass_encoder_set_global_buffer_handle(
        encoder, pass_data->cam_ubo, 0, 0);

    for (size_t idx = 0; idx < obj_count; ++idx) {
        const RenderObject& obj = view.render_objects[idx];

        // Acquire material
        PulseMaterial material_ref = {};
        if (!pulse_acquire_material(app, obj.material, &material_ref)) {
            continue;
        }

        // Acquire mesh
        PulseMesh mesh_ref = {};
        if (!pulse_acquire_mesh(app, obj.mesh, &mesh_ref)) {
            pulse_release_material(app, &material_ref);
            continue;
        }

        // Bind per-object world matrix UBO (set 2, binding 0) at offset
        uint64_t obj_offset = idx * sizeof(ObjectUniformData);
        pulse_renderpass_encoder_set_global_buffer_offset(
            encoder, pass_data->object_ubo, 2, 0,
            obj_offset, sizeof(ObjectUniformData));

        pulse_renderpass_encoder_draw(encoder, material_ref, mesh_ref);

        pulse_release_mesh(app, &mesh_ref);
        pulse_release_material(app, &material_ref);
    }
}

static void record_renderer_callback(
    PulseAppId app,
    pulse_rendergraph_t* graph,
    void* user_data)
{
    pulse_renderer_state* state =
        static_cast<pulse_renderer_state*>(user_data);
    if (!state || !graph) return;

    const FrameRenderPacket& packet = state->read_packet();
    if (packet.views.empty()) return;

    for (const auto& view : packet.views) {
        if (view.window_entity == 0) continue;

        // Import window backbuffer
        pulse_texture_handle_t target_handle =
            pulse_import_window_backbuffer(app, graph, view.window_entity);
        if (!pulse_rendergraph_texture_handle_valid(target_handle)) {
            continue;
        }

        auto cam_ubo = pulse_rendergraph_declare_uniform_buffer_quick(
            graph, sizeof(CameraUniformData), (void*)&view.cam_data);

        // Create combined object UBO (array of world matrices)
        size_t obj_count = view.render_objects.size();
        pulse_buffer_handle_t object_ubo = {};
        if (obj_count > 0) {
            object_ubo = pulse_rendergraph_declare_uniform_buffer_quick(
                graph, obj_count * sizeof(ObjectUniformData), (void*)view.render_data.data());
        }

        // Build render pass
        char pass_name[64];
        snprintf(pass_name, sizeof(pass_name), "RendererView_%llu",
                 static_cast<unsigned long long>(view.camera_entity));
        pulse_renderpass_builder_t pass =
            pulse_rendergraph_add_renderpass(graph, pass_name);

        pulse_renderpass_add_color_attachment(
            &pass, target_handle,
            CGPU_LOAD_ACTION_CLEAR,
            0xff000000,  // black clear color
            CGPU_STORE_ACTION_STORE);

        pulse_renderpass_use_buffer(&pass, cam_ubo);
        if (pulse_rendergraph_buffer_handle_valid(object_ubo)) {
            pulse_renderpass_use_buffer(&pass, object_ubo);
        }

        // Set executable callback
        ViewPassData* passdata = nullptr;
        pulse_renderpass_set_executable(
            &pass,
            render_view_executable,
            sizeof(ViewPassData),
            reinterpret_cast<void**>(&passdata));

        if (passdata) {
            passdata->app = app;
            passdata->view = &view;
            passdata->cam_ubo = cam_ubo;
            passdata->object_ubo = object_ubo;
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

} // extern "C"
