#include "app.h"

#include <utility>

struct pulse_app_state_resource {
    PulseAppId app;
};

ECS_COMPONENT_DECLARE(PulseTimer);
ECS_COMPONENT_DECLARE(pulse_app_state_resource);

struct PulseApp {
    pulse::App impl;

    PulseApp(const char* name)
        : impl(this, name) {
    }
};

namespace {

// Per-frame timing: refresh the PulseTimer singleton from the world's
// measured timing data. Runs in PreUpdate so every system in the frame sees
// fresh values.
void time_system_run(ecs_iter_t* it) {
    const ecs_world_info_t* info = ecs_get_world_info(it->world);
    PulseTimer* ctx = ecs_field(it, PulseTimer, 0);
    if (!info || !ctx) {
        return;
    }

    ctx->delta_time = info->delta_time;
    ctx->time_since_startup = (float)info->world_time_total;
    ctx->delta_time_double = (double)info->delta_time;
    ctx->time_since_startup_double = info->world_time_total;
    ctx->fps = info->delta_time > 0.0f ? (int32_t)(1.0f / info->delta_time + 0.5f) : 0;
}

void install_time_system(ecs_world_t* world) {
    ecs_entity_desc_t entity_desc = {};
    entity_desc.name = "PulseTimeSystem";
    ecs_entity_t entity = ecs_entity_init(world, &entity_desc);

    ecs_system_desc_t desc = {};
    desc.entity = entity;
    desc.phase = EcsPreUpdate;
    // Matches the singleton entity holding PulseTimer
    desc.query.terms[0].id = ecs_id(PulseTimer);
    desc.query.terms[0].inout = EcsOut;
    desc.query.cache_kind = EcsQueryCacheAuto;
    desc.callback = time_system_run;
    ecs_system_init(world, &desc);
}

} // namespace

namespace pulse {

App::App(PulseAppId handle, const char* name)
    : handle_(handle), name_(name) {
    // Register core pipeline tag components so they're available before any plugin
    ecs_world_t* w = world_.c_ptr();
    ECS_COMPONENT_DEFINE(w, PulseTimer);
    ECS_COMPONENT_DEFINE(w, pulse_app_state_resource);

    // Create the time singleton so it exists from the very first frame
    // (ecs_singleton_get_mut does not auto-create).
    PulseTimer time_ctx = {};
    ecs_singleton_set_ptr(w, PulseTimer, &time_ctx);

    // Install the per-frame time system (updates PulseTimer each frame)
    install_time_system(w);

    pulse_app_state_resource res{
        .app = handle,
    };
    ecs_singleton_set_ptr(w, pulse_app_state_resource, &res);
}

App::~App() {
    for (auto& subapp : subapps_) {
        delete subapp.app;
    }
    subapps_.clear();
}

void App::set_error(const char* message) {
    last_error_ = message ? message : "";
}

const char* App::last_error() const {
    return last_error_.empty() ? nullptr : last_error_.c_str();
}

void App::set_name(const char* name) {
    name_ = name ? name : "";
}

const char* App::get_name() const {
    return name_.empty() ? nullptr : name_.c_str();
}

bool App::has_plugin(const char* name) const {
    if (!name) return false;

    for (const auto& plugin : plugins_) {
        if (plugin.name == name) {
            return true;
        }
    }

    return false;
}

bool App::has_pending_plugin(const char* name) const {
    if (!name) return false;

    for (const auto& plugin : pending_plugins_) {
        if (plugin.name == name) {
            return true;
        }
    }

    return false;
}

EPulseResult App::validate_plugin_desc(const PulsePluginDesc& desc) {
    if (desc.struct_size != sizeof(PulsePluginDesc)) {
        set_error("invalid plugin desc size");
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (desc.version != PULSE_PLUGIN_DESC_VERSION) {
        set_error("invalid plugin desc version");
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (!desc.name || !desc.name[0]) {
        set_error("plugin name is required");
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (has_plugin(desc.name) || has_pending_plugin(desc.name)) {
        last_error_ = "duplicate plugin: ";
        last_error_ += desc.name;
        return PULSE_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    return PULSE_RESULT_OK;
}

EPulseResult App::add_plugin(const PulsePluginDesc& desc) {
    if (state_ != AppState::Created && state_ != AppState::Building) {
        set_error("plugins can only be added before app run");
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    EPulseResult result = validate_plugin_desc(desc);
    if (result != PULSE_RESULT_OK) {
        return result;
    }

    RegisteredPlugin plugin;
    plugin.name = desc.name;
    plugin.ctx = desc.ctx;
    plugin.build = desc.build;
    plugin.post_build = desc.post_build;
    plugin.shutdown = desc.shutdown;
    pending_plugins_.push_back(std::move(plugin));

    if (draining_plugins_) {
        return PULSE_RESULT_OK;
    }

    return drain_pending_plugins();
}

EPulseResult App::drain_pending_plugins() {
    if (draining_plugins_) {
        return PULSE_RESULT_OK;
    }

    draining_plugins_ = true;
    state_ = AppState::Building;

    while (!pending_plugins_.empty()) {
        RegisteredPlugin plugin = std::move(pending_plugins_.front());
        pending_plugins_.pop_front();

        plugins_.push_back(std::move(plugin));
        RegisteredPlugin& entry = plugins_.back();
        entry.build_done = true;

        if (entry.build) {
            EPulseResult result = entry.build(handle_, entry.ctx);
            if (result != PULSE_RESULT_OK) {
                last_error_ = "plugin build failed: ";
                last_error_ += entry.name;
                draining_plugins_ = false;
                state_ = AppState::Created;
                return result;
            }
        }
    }

    draining_plugins_ = false;
    state_ = AppState::Created;
    return PULSE_RESULT_OK;
}

EPulseResult App::post_build() {
    if (shutdown_done_ || state_ == AppState::Running || state_ == AppState::Finished) {
        set_error("app is not in a post-buildable state");
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    if (post_build_done_) {
        return PULSE_RESULT_OK;
    }

    EPulseResult result = drain_pending_plugins();
    if (result != PULSE_RESULT_OK) {
        return result;
    }

    state_ = AppState::ReadyToRun;

    for (auto& plugin : plugins_) {
        if (!plugin.post_build_done && plugin.post_build) {
            result = plugin.post_build(handle_, plugin.ctx);
            if (result != PULSE_RESULT_OK) {
                last_error_ = "plugin post_build failed: ";
                last_error_ += plugin.name;
                return result;
            }
        }
        plugin.post_build_done = true;
    }

    for (auto& subapp : subapps_) {
        result = subapp.app->impl.post_build();
        if (result != PULSE_RESULT_OK) {
            last_error_ = "subapp post_build failed: ";
            last_error_ += subapp.name;
            return result;
        }
    }

    post_build_done_ = true;
    return PULSE_RESULT_OK;
}

void App::shutdown() {
    if (shutdown_done_) {
        return;
    }

    for (auto it = subapps_.rbegin(); it != subapps_.rend(); ++it) {
        it->app->impl.shutdown();
    }

    for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
        if (!it->shutdown_done && it->shutdown) {
            it->shutdown(handle_, it->ctx);
        }
        it->shutdown_done = true;
    }

    shutdown_done_ = true;
    state_ = AppState::Finished;
}

EPulseResult App::default_runner() {
    return update();
}

EPulseResult App::run() {
    if (state_ == AppState::Running) {
        set_error("app is already running");
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    if (state_ == AppState::Finished || shutdown_done_) {
        set_error("app cannot be run after shutdown");
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    EPulseResult result = post_build();
    if (result != PULSE_RESULT_OK) {
        return result;
    }

    state_ = AppState::Running;
    result = runner_fn_ ? runner_fn_(handle_, runner_ctx_) : default_runner();
    if (result != PULSE_RESULT_OK) {
        set_error("runner failed");
    }

    return result;
}

EPulseResult App::update() {
    if (shutdown_done_) {
        set_error("app cannot be updated after shutdown");
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    world_.progress();

    for (auto& subapp : subapps_) {
        if (subapp.extract) {
            EPulseResult result = subapp.extract(handle_, subapp.app, subapp.extract_ctx);
            if (result != PULSE_RESULT_OK) {
                last_error_ = "subapp extract failed: ";
                last_error_ += subapp.name;
                return result;
            }
        }

        EPulseResult result = subapp.app->impl.update();
        if (result != PULSE_RESULT_OK) {
            last_error_ = "subapp update failed: ";
            last_error_ += subapp.name;
            return result;
        }
    }

    return PULSE_RESULT_OK;
}

EPulseResult App::set_runner(PulseProcRunnerFn runner, void* ctx) {
    if (state_ != AppState::Created && state_ != AppState::ReadyToRun) {
        set_error("runner can only be set before app run");
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    runner_fn_ = runner;
    runner_ctx_ = ctx;
    return PULSE_RESULT_OK;
}

EPulseResult App::insert_subapp(const char* name, PulseAppId subapp) {
    if (!name || !name[0] || !subapp) {
        set_error("subapp name and handle are required");
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (state_ == AppState::Running || state_ == AppState::Finished || shutdown_done_) {
        set_error("subapps can only be inserted before app run");
        return PULSE_RESULT_ERROR_INVALID_STATE;
    }

    for (const auto& entry : subapps_) {
        if (entry.name == name) {
            last_error_ = "duplicate subapp: ";
            last_error_ += name;
            return PULSE_RESULT_ERROR_DUPLICATE_SUBAPP;
        }
    }

    RegisteredSubApp entry;
    entry.name = name;
    entry.app = subapp;
    subapps_.push_back(entry);
    return PULSE_RESULT_OK;
}

PulseAppId App::get_subapp(const char* name) const {
    if (!name) return nullptr;

    for (const auto& subapp : subapps_) {
        if (subapp.name == name) {
            return subapp.app;
        }
    }

    return nullptr;
}

PulseAppId App::remove_subapp(const char* name) {
    if (!name) return nullptr;

    for (auto it = subapps_.begin(); it != subapps_.end(); ++it) {
        if (it->name == name) {
            PulseAppId subapp = it->app;
            subapps_.erase(it);
            return subapp;
        }
    }

    return nullptr;
}

EPulseResult App::set_subapp_extract(const char* name, PulseProcSubappExtractFn extract, void* ctx) {
    if (!name || !name[0]) {
        set_error("subapp name is required");
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    for (auto& subapp : subapps_) {
        if (subapp.name == name) {
            subapp.extract = extract;
            subapp.extract_ctx = ctx;
            return PULSE_RESULT_OK;
        }
    }

    last_error_ = "subapp not found: ";
    last_error_ += name;
    return PULSE_RESULT_ERROR_NOT_FOUND;
}

EPulseResult App::extract_subapps() {
    for (auto& subapp : subapps_) {
        if (!subapp.extract) {
            continue;
        }

        EPulseResult result = subapp.extract(handle_, subapp.app, subapp.extract_ctx);
        if (result != PULSE_RESULT_OK) {
            last_error_ = "subapp extract failed: ";
            last_error_ += subapp.name;
            return result;
        }
    }

    return PULSE_RESULT_OK;
}

} // namespace pulse

extern "C" {

static inline pulse::App* to_app(PulseAppId handle) {
    return handle ? &handle->impl : nullptr;
}

PulseAppId pulse_create_app(const char* name) {
    return new PulseApp(name);
}

void pulse_destroy_app(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return;
    }

    impl->shutdown();
    delete app;
}

EPulseResult pulse_app_run(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->run();
}

EPulseResult pulse_app_update(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->update();
}

EPulseResult pulse_app_set_runner(PulseAppId app, PulseProcRunnerFn runner, void* ctx) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->set_runner(runner, ctx);
}

EPulseResult pulse_app_add_plugin(PulseAppId app, const PulsePluginDesc* desc) {
    pulse::App* impl = to_app(app);
    if (!impl || !desc) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->add_plugin(*desc);
}

bool pulse_app_has_plugin(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->has_plugin(name) : false;
}

ecs_world_t* pulse_app_world(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->world_c() : nullptr;
}

const char* pulse_app_last_error(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->last_error() : "invalid app";
}

EPulseResult pulse_app_insert_subapp(PulseAppId app, const char* name, PulseAppId subapp) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->insert_subapp(name, subapp);
}

PulseAppId pulse_app_get_subapp(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->get_subapp(name) : nullptr;
}

PulseAppId pulse_app_remove_subapp(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->remove_subapp(name) : nullptr;
}

EPulseResult pulse_app_set_subapp_extract(
    PulseAppId app,
    const char* name,
    PulseProcSubappExtractFn extract,
    void* ctx
) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->set_subapp_extract(name, extract, ctx);
}

EPulseResult pulse_app_extract_subapps(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_RESULT_ERROR_INVALID_ARGUMENT;
    }

    return impl->extract_subapps();
}

PULSE_API PulseAppId pulse_get_app_from_world(ecs_world_t* world) {
    if (world == nullptr) return nullptr;
    const pulse_app_state_resource* resource = ecs_singleton_get(world, pulse_app_state_resource);
    return resource->app;
}

} // extern "C"
