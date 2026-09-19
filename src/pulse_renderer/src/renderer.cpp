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

void begin_extract_system(ecs_iter_t* it) {
    pulse_renderer_state* state = static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    FrameRenderPacket& packet = state->write_packet();
    packet.grow_staging(&packet.pool, (uint32_t)state->features.size());
    packet.cameras.clear();
    for (FeatureStaging& staging : packet.staging) {
        staging.items.clear();
        staging.data_arena.clear();
    }
}

void extract_cameras_system(ecs_iter_t* it) {
    pulse_renderer_state* state = static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    FrameRenderPacket& packet = state->write_packet();

    PulseCamera* cameras = ecs_field(it, PulseCamera, 0);
    PulseWorldTransform* world_transforms = ecs_field(it, PulseWorldTransform, 1);

    ecs_world_t* world = it->world;

    for (int i = 0; i < it->count; ++i) {
        ecs_entity_t entity = it->entities[i];
        PulseCamera& cam = cameras[i];
        HMM_Mat4& world_mat = world_transforms[i].value;

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

        CameraSnapshot snapshot = {};
        snapshot.camera_entity = entity;
        snapshot.window_entity = cam.window_entity;
        snapshot.view_matrix = build_view_matrix(world_mat);
        if (cam.orthographic) {
            float half_height = cam.orthographic_size;
            float half_width = half_height * aspect;
            snapshot.proj_matrix = HMM_Orthographic_LH_RO(-half_width, half_width, -half_height, half_height, cam.near_plane, cam.far_plane);
        } else {
            snapshot.proj_matrix = HMM_Perspective_LH_RO(fov_rad, aspect, cam.near_plane, cam.far_plane);
        }
        snapshot.fov = cam.fov;
        snapshot.orthographic_size = cam.orthographic_size;
        snapshot.near_plane = cam.near_plane;
        snapshot.far_plane = cam.far_plane;
        snapshot.width = width;
        snapshot.height = height;
        packet.cameras.push_back(snapshot);
    }
}

void extract_features_system(ecs_iter_t* it) {
    auto* state = static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    FrameRenderPacket& packet = state->write_packet();
    const uint32_t feature_count = (uint32_t)state->features.size();
    packet.grow_staging(&packet.pool, feature_count);

    for (uint32_t feature_id = 0; feature_id < feature_count; ++feature_id) {
        const RenderFeature& feature = state->features[feature_id];
        FeatureExtractContext ctx{ state, feature_id };
        feature.extract(state->app, it->world, ctx, feature.userdata);
    }
}

void packets_swap_system(ecs_iter_t* it) {
    pulse_renderer_state* state = static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    state->swap_packets();
    ++state->frame_index;
}

static float compute_view_depth(const HMM_Mat4& view_matrix, const HMM_Mat4& world_matrix) {
    auto t = HMM_M4GetTranslate(world_matrix);
    return view_matrix[0].Z * t.X + view_matrix[1].Z * t.Y + view_matrix[2].Z * t.Z + view_matrix[3].Z;
}

static bool compare_items(const FrameRenderPacket& snapshot, const DrawItem& a, const DrawItem& b, uint32_t flags) {
    const StagingItem& sa = snapshot.staging[a.feature_id].items[a.staging_index];
    const StagingItem& sb = snapshot.staging[b.feature_id].items[b.staging_index];
    if (flags & PULSE_SORT_SHADER) {
        if (sa.shader.index != sb.shader.index) return sa.shader.index < sb.shader.index;
    }
    if (flags & PULSE_SORT_MATERIAL) {
        if (sa.material.index != sb.material.index) return sa.material.index < sb.material.index;
    }
    if (flags & PULSE_SORT_MESH) {
        if (sa.mesh.index != sb.mesh.index) return sa.mesh.index < sb.mesh.index;
    }
    if (flags & PULSE_SORT_DEPTH_FRONT_TO_BACK) {
        if (a.view_depth != b.view_depth) return a.view_depth < b.view_depth;
    }
    if (flags & PULSE_SORT_DEPTH_BACK_TO_FRONT) {
        if (a.view_depth != b.view_depth) return a.view_depth > b.view_depth;
    }
    return a.submission_index < b.submission_index;
}

static void prepare_item_globals(pulse_renderer_state& state, ViewFrameData& view, RendererList& list, DrawItem& item, const FrameRenderPacket& snapshot) {
    const StagingItem& staging = snapshot.staging[item.feature_id].items[item.staging_index];
    if (staging.shader.index == 0) return;

    for (const auto& cached : list.global_columns) {
        if (cached.shader.index == staging.shader.index && cached.shader.generation == staging.shader.generation) {
            item.global_column_start = cached.first_column;
            item.global_column_count = cached.column_count;
            return;
        }
    }

    RendererGlobalColumns entry = {};
    entry.shader = staging.shader;
    entry.first_column = (uint32_t)view.ubo_columns.size();
    for (uint32_t u = 0; u < pulse_shader_get_ubo_info_count(state.app, staging.shader); ++u) {
        const auto& info = pulse_shader_get_ubo_info(state.app, staging.shader, u);
        if (info.set != PULSE_SHADER_SET_GLOBAL) continue;

        RendererUboColumn col = {};
        col.set = info.set;
        col.binding = info.binding;
        col.block_ref = alloc_ubo_block(state, view, info.size);
        fill_ubo_block(state.app, staging.shader, info.set, info.binding, col.block_ref, [&view](const char* name) { return view.properties.find(name); });
        view.ubo_columns.push_back(col);
    }
    entry.column_count = (uint32_t)view.ubo_columns.size() - entry.first_column;

    item.global_column_start = entry.first_column;
    item.global_column_count = entry.column_count;
    list.global_columns.push_back(entry);
}

void build_views_system(ecs_iter_t* it) {
    pulse_renderer_state* state = static_cast<pulse_renderer_state*>(it->ctx);
    if (!state) return;

    const FrameRenderPacket* snapshot = &state->read_packet();
    FrameViewData* vd = &state->view_data;

    state->ubo_cache.frame_end();
    state->ubo_cache.release_idle(state->frame_index);
    vd->reset(snapshot);

    std::vector<uint32_t> features_in_list(state->features.size());

    assert(snapshot->staging.size() >= state->features.size());

    for (uint32_t view_index = 0; view_index < (uint32_t)snapshot->cameras.size(); ++view_index) {
        const CameraSnapshot& camera = snapshot->cameras[view_index];
        ViewFrameData& view = vd->views.emplace_back(&vd->pool);
        if (camera.window_entity == 0) continue;

        for (const RendererListDesc& desc : state->list_registry) {
            view.lists.emplace_back(&vd->pool);
            view.lists.back().desc = desc;
        }

        for (uint32_t feature_id = 0; feature_id < (uint32_t)state->features.size(); ++feature_id) {
            const RenderFeature& feature = state->features[feature_id];
            FeatureCullContext cull_ctx{ state, snapshot, &camera, view_index, feature_id };
            const std::pmr::vector<StagingItem>& staging = snapshot->staging[feature_id].items;
            for (uint32_t index = 0; index < (uint32_t)staging.size(); ++index) {
                const StagingItem& staging_item = staging[index];
                int32_t list_id = feature.cull ? feature.cull(cull_ctx, staging_item, feature.userdata) : 0;
                if (list_id < 0 || (size_t)list_id >= view.lists.size()) continue;
                RendererList& list = view.lists[list_id];
                DrawItem item = {};
                item.staging_index = index;
                item.data_slot = staging_item.data_slot;
                item.feature_id = (uint16_t)feature_id;
                item.submission_index = (uint32_t)list.items.size();
                list.items.push_back(item);
            }
        }

        const HMM_Mat4 vp = HMM_Mul(camera.proj_matrix, camera.view_matrix);
        view.properties.set_mat4(kPropertyNameVPMatrix, vp);

        for (RendererList& list : view.lists) {
            const uint32_t flags = list.desc.sort_flags;
            if (flags & (PULSE_SORT_DEPTH_FRONT_TO_BACK | PULSE_SORT_DEPTH_BACK_TO_FRONT)) {
                for (DrawItem& item : list.items) {
                    item.view_depth = compute_view_depth(camera.view_matrix, snapshot->staging[item.feature_id].items[item.staging_index].world_matrix);
                }
            }
            std::sort(list.items.begin(), list.items.end(), [snapshot, flags](const DrawItem& a, const DrawItem& b) {
                return compare_items(*snapshot, a, b, flags);
            });

            for (DrawItem& item : list.items) {
                item.global_column_start = 0;
                item.global_column_count = 0;
                item.feature_column_start = 0;
                item.feature_column_count = 0;
                prepare_item_globals(*state, view, list, item, *snapshot);
            }
        }

        std::fill(features_in_list.begin(), features_in_list.end(), 0);
        for (RendererList& list : view.lists) {
            for (const DrawItem& item : list.items) features_in_list[item.feature_id] = 1;
        }

        for (uint32_t list_id = 0; list_id < (uint32_t)view.lists.size(); ++list_id) {
            for (uint32_t feature_id = 0; feature_id < (uint32_t)state->features.size(); ++feature_id) {
                if (!features_in_list[feature_id]) continue;
                const RenderFeature& feature = state->features[feature_id];
                if (!feature.prepare) continue;
                FeaturePrepareContext ctx{ state, view, snapshot, &camera, view_index, list_id, feature_id };
                feature.prepare(ctx, feature.userdata);
            }
        }
    }
}

// ============================================================
// Render Record Callback (registered with pulse_graphics)
// ============================================================

struct ViewPassData {
    const pulse_renderer_state* state;
    const FrameViewData* view_data;
    uint32_t view_index;
};

static void render_view_executable(PulseRenderPassEncoder* encoder, void* userdata) {
    ViewPassData* pass_data = static_cast<ViewPassData*>(userdata);
    if (!encoder || !pass_data || !pass_data->view_data || !pass_data->state) return;

    const pulse_renderer_state* state = pass_data->state;
    const FrameViewData* vd = pass_data->view_data;
    const FrameRenderPacket* snapshot = vd->snapshot;
    const ViewFrameData& view = vd->views[pass_data->view_index];
    const CameraSnapshot& camera = snapshot->cameras[pass_data->view_index];

    for (const RendererList& list : view.lists) {
        for (const DrawItem& item : list.items) {
            const StagingItem& staging = snapshot->staging[item.feature_id].items[item.staging_index];
            if (staging.shader.index == 0) continue;

            const RenderFeature& feature = state->features[item.feature_id];
            FeatureDrawContext ctx{ state, snapshot, &view, &camera, &list, &item };
            feature.draw(state->app, encoder, ctx, feature.userdata);
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

    const FrameRenderPacket* snapshot = &state->read_packet();
    FrameViewData* vd = &state->view_data;
    if (vd->snapshot != snapshot) return;

    assert(vd->views.size() <= snapshot->cameras.size());

    for (uint32_t view_index = 0; view_index < (uint32_t)vd->views.size(); ++view_index) {
        const CameraSnapshot& camera = snapshot->cameras[view_index];
        if (camera.window_entity == 0) continue;

        PulseRGTextureHandle target_handle =
            pulse_import_window_backbuffer(app, graph, camera.window_entity);
        if (!pulse_rgtexture_handle_is_valid(target_handle))
            continue;

        ViewFrameData& view = vd->views[view_index];

        for (GpuBlock& block : view.blocks) {
            if (block.used == 0) continue;
            block.gpu_handle = pulse_render_graph_declare_uniform_buffer_quick(
                graph, block.used, block.cpu_data);
        }

        char pass_name[64];
        snprintf(pass_name, sizeof(pass_name), "RendererView_%llu",
                 static_cast<unsigned long long>(camera.camera_entity));
        PulseRenderPassBuilder pass =
            pulse_render_graph_add_render_pass(graph, pass_name);

        pulse_render_pass_builder_add_color_attachment(
            &pass, target_handle,
            CGPU_LOAD_ACTION_CLEAR,
            0xff000000,
            CGPU_STORE_ACTION_STORE);

        for (const GpuBlock& block : view.blocks) {
            if (pulse_rgbuffer_handle_is_valid(block.gpu_handle))
                pulse_render_pass_builder_use_buffer(&pass, block.gpu_handle);
        }

        ViewPassData* passdata = nullptr;
        pulse_render_pass_builder_set_executable(
            &pass,
            render_view_executable,
            sizeof(ViewPassData),
            reinterpret_cast<void**>(&passdata));

        if (passdata) {
            passdata->state = state;
            passdata->view_data = vd;
            passdata->view_index = view_index;
        }
    }
}

// ============================================================
// System installation
// ============================================================

void install_renderer_systems(ecs_world_t* world, pulse_renderer_state* state) {
    if (!world || !state) return;

    ecs_entity_t prev_system = 0;

    {
        ecs_entity_desc_t entity_desc = {};
        entity_desc.name = "PulseRendererBeginExtract";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc = {};
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.callback = begin_extract_system;
        desc.ctx = state;
        desc.immediate = true;
        ecs_system_init(world, &desc);

        prev_system = entity;
        state->begin_extract_system = entity;
    }

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

        ecs_entity_t propagate = ecs_lookup(world, "PropagateWorldTransform");
        if (propagate != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, propagate);
        }
        if (prev_system != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, prev_system);
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

    {
        ecs_entity_desc_t entity_desc = {};
        entity_desc.name = "PulseRendererPacketsSwap";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc = {};
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.callback = packets_swap_system;
        desc.ctx = state;
        desc.immediate = true;
        ecs_system_init(world, &desc);

        if (prev_system != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, prev_system);
        }
        prev_system = entity;
        state->packets_swap_system = entity;
    }

    {
        ecs_entity_desc_t entity_desc = {};
        entity_desc.name = "PulseRendererBuildViews";
        ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

        ecs_system_desc_t desc = {};
        desc.entity = entity;
        desc.phase = EcsPostUpdate;
        desc.callback = build_views_system;
        desc.ctx = state;
        desc.immediate = true;
        ecs_system_init(world, &desc);

        if (prev_system != 0) {
            ecs_add_pair(world, entity, EcsDependsOn, prev_system);
        }
        state->build_views_system = entity;
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

    register_renderer_components(world);

    install_renderable_feature(state, world);

    pulse_renderer_state_resource state_res = {};
    state_res.state = state;
    ecs_singleton_set_ptr(world, pulse_renderer_state_resource, &state_res);

    install_renderer_systems(world, state);

    return PULSE_PLUGIN_BUILD_RESULT_OK;
}

EPulsePluginBuildResult renderer_plugin_post_build(PulseAppId app, void* ctx) {
    auto* state = static_cast<pulse_renderer_state*>(ctx);
    if (!state) return PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT;

    const PulseRenderer* gfx_renderer = pulse_get_renderer(app);
    if (gfx_renderer && gfx_renderer->adapter) {
        const CGPUAdapterDetail* detail =
            cgpu_adapter_query_adapter_detail(gfx_renderer->adapter);
        if (detail && detail->uniform_buffer_alignment > 0)
            state->ubo_alignment = detail->uniform_buffer_alignment;
    }

    PulseRenderRecordCallbackDesc cb_desc = {};
    cb_desc.callback = record_renderer_callback;
    cb_desc.user_data = state;
    cb_desc.priority = 100;

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

    if (state->record_callback_registered) {
        pulse_remove_render_record_callback(app, record_renderer_callback);
        state->record_callback_registered = false;
    }

    if (world && state->begin_extract_system && ecs_is_alive(world, state->begin_extract_system))
        ecs_delete(world, state->begin_extract_system);
    if (world && state->extract_cameras_system && ecs_is_alive(world, state->extract_cameras_system))
        ecs_delete(world, state->extract_cameras_system);
    if (world && state->extract_features_system && ecs_is_alive(world, state->extract_features_system))
        ecs_delete(world, state->extract_features_system);
    if (world && state->packets_swap_system && ecs_is_alive(world, state->packets_swap_system))
        ecs_delete(world, state->packets_swap_system);
    if (world && state->build_views_system && ecs_is_alive(world, state->build_views_system))
        ecs_delete(world, state->build_views_system);

    for (auto& feature : state->features) {
        shutdown_renderable_feature(feature.userdata);
    }
    state->features.clear();

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
