#include "renderer_internal.h"

#include <algorithm>
#include <string.h>
#include <cmath>
#include <utility>

namespace pulse_renderer_internal {

constexpr const char* kPluginName = "pulse_renderer";

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
        if (cam.orthographic) {
            float half_height = cam.orthographic_size;
            float half_width  = half_height * aspect;
            view.proj_matrix = HMM_Orthographic_LH_RO(-half_width, half_width, -half_height, half_height, cam.near_plane, cam.far_plane);
        } else {
            view.proj_matrix = HMM_Perspective_LH_RO(fov_rad, aspect, cam.near_plane, cam.far_plane);
        }
        view.fov = cam.fov;
        view.orthographic_size = cam.orthographic_size;
        view.near_plane = cam.near_plane;
        view.far_plane = cam.far_plane;
        view.width = width;
        view.height = height;

        for (const auto& desc : state->list_registry) {
            view.lists.emplace_back(&packet.pool);
            view.lists.back().desc = desc;
            view.lists.back().init_feature_data((uint32_t)state->features.size());
        }
    }
}

void extract_features_system(ecs_iter_t* it) {
    auto* state = static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;
    FrameRenderPacket& packet = state->write_packet();
    for (uint32_t view_index = 0; view_index < (uint32_t)packet.views.size(); ++view_index) {
        RendererView& view = packet.views[view_index];
        for (uint32_t list_id = 0; list_id < (uint32_t)view.lists.size(); ++list_id) {
            for (uint32_t feature_id = 0; feature_id < (uint32_t)state->features.size(); ++feature_id) {
                const RenderFeature& feature = state->features[feature_id];
                FeatureExtractContext ctx{ state, view_index, list_id, feature_id };
                feature.extract(state->app, it->world, ctx, feature.userdata);
            }
        }
    }
}

static float compute_view_depth(const HMM_Mat4& view_matrix, const HMM_Mat4& world_matrix) {
    auto t = HMM_M4GetTranslate(world_matrix);
    return view_matrix[0].Z * t.X + view_matrix[1].Z * t.Y + view_matrix[2].Z * t.Z + view_matrix[3].Z;
}

static bool compare_items(const DrawItem& a, const DrawItem& b, uint32_t flags) {
    if (flags & PULSE_SORT_SHADER) {
        if (a.shader.index != b.shader.index) return a.shader.index < b.shader.index;
    }
    if (flags & PULSE_SORT_MATERIAL) {
        if (a.material.index != b.material.index) return a.material.index < b.material.index;
    }
    if (flags & PULSE_SORT_MESH) {
        if (a.mesh.index != b.mesh.index) return a.mesh.index < b.mesh.index;
    }
    if (flags & PULSE_SORT_DEPTH_FRONT_TO_BACK) {
        if (a.view_depth != b.view_depth) return a.view_depth < b.view_depth;
    }
    if (flags & PULSE_SORT_DEPTH_BACK_TO_FRONT) {
        if (a.view_depth != b.view_depth) return a.view_depth > b.view_depth;
    }
    return a.submission_index < b.submission_index;
}

// SortAndPackSystem: sorts renderer list items per view and swaps double buffers.
void sort_and_pack_system(ecs_iter_t* it) {
    pulse_renderer_state* state =
        static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    FrameRenderPacket& packet = state->write_packet();

    for (auto& view : packet.views) {
        for (auto& list : view.lists) {
            const uint32_t flags = list.desc.sort_flags;
            if (flags & (PULSE_SORT_DEPTH_FRONT_TO_BACK | PULSE_SORT_DEPTH_BACK_TO_FRONT)) {
                for (auto& item : list.items) {
                    item.view_depth = compute_view_depth(view.view_matrix, item.world_matrix);
                }
            }
            std::sort(list.items.begin(), list.items.end(), [flags](const DrawItem& a, const DrawItem& b) {
                return compare_items(a, b, flags);
            });
        }
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

static void prepare_item_globals(pulse_renderer_state& state, RendererView& view, RendererList& list, DrawItem& item, const HMM_Mat4& vp) {
    if (item.shader.index == 0) return;

    for (const auto& cached : list.global_columns) {
        if (cached.shader.index == item.shader.index && cached.shader.generation == item.shader.generation) {
            item.global_column_start = cached.first_column;
            item.global_column_count = cached.column_count;
            return;
        }
    }

    RendererGlobalColumns entry = {};
    entry.shader = item.shader;
    entry.first_column = (uint32_t)view.ubo_columns.size();
    for (uint32_t u = 0; u < pulse_shader_get_ubo_info_count(state.app, item.shader); ++u) {
        const auto& info = pulse_shader_get_ubo_info(state.app, item.shader, u);
        if (info.set != PULSE_SHADER_SET_GLOBAL) continue;

        RendererUboColumn col = {};
        col.set = info.set;
        col.binding = info.binding;
        col.block_ref = alloc_ubo_block(state, view, info.size);
        if (info.binding == 0 && info.size >= sizeof(HMM_Mat4)) {
            memcpy(col.block_ref.ptr, &vp, sizeof(HMM_Mat4));
        }
        view.ubo_columns.push_back(col);
    }
    entry.column_count = (uint32_t)view.ubo_columns.size() - entry.first_column;

    item.global_column_start = entry.first_column;
    item.global_column_count = entry.column_count;
    list.global_columns.push_back(entry);
}

static void render_view_executable(PulseRenderPassEncoder* encoder, void* userdata) {
    ViewPassData* pass_data = static_cast<ViewPassData*>(userdata);
    if (!encoder || !pass_data || !pass_data->view) return;

    const RendererView& view = *pass_data->view;
    PulseAppId app = pass_data->app;

    for (const RendererList& list : view.lists) {
        for (const DrawItem& item : list.items) {
            if (item.shader.index == 0)
                continue;

            const RenderFeature& feature = pass_data->state->features[item.feature_id];
            FeatureDrawContext ctx{ pass_data->state, &view, &list, &item };
            feature.draw(app, encoder, ctx, feature.userdata);
        }
    }
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

    for (uint32_t view_index = 0; view_index < (uint32_t)packet.views.size(); ++view_index) {
        RendererView& view = packet.views[view_index];
        if (view.window_entity == 0) continue;

        PulseRGTextureHandle target_handle =
            pulse_import_window_backbuffer(app, graph, view.window_entity);
        if (!pulse_rgtexture_handle_is_valid(target_handle))
            continue;

        view.ubo_columns.clear();

        HMM_Mat4 vp = HMM_Mul(view.proj_matrix, view.view_matrix);

        for (auto& list : view.lists) {
            for (auto& item : list.items) {
                prepare_item_globals(*state, view, list, item, vp);
            }
        }

        for (uint32_t list_id = 0; list_id < (uint32_t)view.lists.size(); ++list_id) {
            for (uint32_t feature_id = 0; feature_id < (uint32_t)state->features.size(); ++feature_id) {
                const RenderFeature& feature = state->features[feature_id];
                if (!feature.prepare) continue;
                FeaturePrepareContext ctx{ state, app, graph, view_index, list_id, feature_id, vp };
                feature.prepare(ctx, feature.userdata);
            }
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

    {
        ecs_entity_desc_t entity_desc = {};
        entity_desc.name = "PulseRendererExtractFeatures";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc = {};
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.callback = extract_features_system;
        desc.ctx = state;
        desc.immediate = true;
        ecs_system_init(world, &desc);

        if (prev_system != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, prev_system);
        }
        prev_system = entity;
        state->extract_features_system = entity;
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

        // Must run after ExtractFeatures
        if (prev_system != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, prev_system);
        }
        prev_system = entity;
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

        // Must run after SortAndPack
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

    state->register_render_list({ "Default", kDefaultListSortFlags });

    // Register ECS components
    register_renderer_components(world);

    install_renderable_feature(state, world);

    // Store state as singleton for later retrieval
    pulse_renderer_state_resource state_res = {};
    state_res.state = state;
    ecs_singleton_set_ptr(world, pulse_renderer_state_resource, &state_res);

    // Install ECS systems
    install_renderer_systems(world, state);

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
    if (world && state->extract_features_system && ecs_is_alive(world, state->extract_features_system))
        ecs_delete(world, state->extract_features_system);
    if (world && state->sort_and_pack_system && ecs_is_alive(world, state->sort_and_pack_system))
        ecs_delete(world, state->sort_and_pack_system);
    if (world && state->packets_swap_system && ecs_is_alive(world, state->packets_swap_system))
        ecs_delete(world, state->packets_swap_system);

    for (auto& feature : state->features) {
        shutdown_renderable_feature(feature.userdata);
    }
    state->features.clear();

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

    const char* renderer_dependencies[] = {
        "pulse_window",
        "pulse_graphics",
        "pulse_transform"
    };
    PulsePluginDesc plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .plugin_version = PULSE_RENDERER_PLUGIN_DESC_VERSION,
        .name = kPluginName,
        .ctx = state,
        .build = renderer_plugin_build,
        .post_build = renderer_plugin_post_build,
        .shutdown = renderer_plugin_shutdown,
        .dependency_count = 3,
        .dependencies = renderer_dependencies,
    };

    EPulseAppAddPluginResult result = pulse_app_add_plugin(app, &plugin_desc);
    if (result != PULSE_APP_ADD_PLUGIN_RESULT_OK && !pulse_app_has_plugin(app, kPluginName)) {
        delete state;
    }
    return result;
}

} // extern "C"
