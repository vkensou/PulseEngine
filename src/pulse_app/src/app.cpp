#include "app.h"

#include <utility>

struct pulse_app_o {
    pulse::App impl;

    pulse_app_o()
        : impl(this) {
    }
};

namespace pulse {

App::App(pulse_app_t handle)
    : handle_(handle) {
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

pulse_result_t App::validate_plugin_desc(const pulse_plugin_desc& desc) {
    if (desc.struct_size != sizeof(pulse_plugin_desc)) {
        set_error("invalid plugin desc size");
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    if (desc.version != PULSE_PLUGIN_DESC_VERSION) {
        set_error("invalid plugin desc version");
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    if (!desc.name || !desc.name[0]) {
        set_error("plugin name is required");
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    if (has_plugin(desc.name) || has_pending_plugin(desc.name)) {
        last_error_ = "duplicate plugin: ";
        last_error_ += desc.name;
        return PULSE_ERROR_DUPLICATE_PLUGIN;
    }

    return PULSE_OK;
}

pulse_result_t App::add_plugin(const pulse_plugin_desc& desc) {
    if (state_ != AppState::Created && state_ != AppState::Building) {
        set_error("plugins can only be added before app run");
        return PULSE_ERROR_INVALID_STATE;
    }

    pulse_result_t result = validate_plugin_desc(desc);
    if (result != PULSE_OK) {
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
        return PULSE_OK;
    }

    return drain_pending_plugins();
}

pulse_result_t App::drain_pending_plugins() {
    if (draining_plugins_) {
        return PULSE_OK;
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
            pulse_result_t result = entry.build(handle_, entry.ctx);
            if (result != PULSE_OK) {
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
    return PULSE_OK;
}

pulse_result_t App::post_build() {
    if (shutdown_done_ || state_ == AppState::Running || state_ == AppState::Finished) {
        set_error("app is not in a post-buildable state");
        return PULSE_ERROR_INVALID_STATE;
    }

    if (post_build_done_) {
        return PULSE_OK;
    }

    pulse_result_t result = drain_pending_plugins();
    if (result != PULSE_OK) {
        return result;
    }

    state_ = AppState::ReadyToRun;

    for (auto& plugin : plugins_) {
        if (!plugin.post_build_done && plugin.post_build) {
            result = plugin.post_build(handle_, plugin.ctx);
            if (result != PULSE_OK) {
                last_error_ = "plugin post_build failed: ";
                last_error_ += plugin.name;
                return result;
            }
        }
        plugin.post_build_done = true;
    }

    for (auto& subapp : subapps_) {
        result = subapp.app->impl.post_build();
        if (result != PULSE_OK) {
            last_error_ = "subapp post_build failed: ";
            last_error_ += subapp.name;
            return result;
        }
    }

    post_build_done_ = true;
    return PULSE_OK;
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

pulse_result_t App::default_runner() {
    return update();
}

pulse_result_t App::run() {
    if (state_ == AppState::Running) {
        set_error("app is already running");
        return PULSE_ERROR_INVALID_STATE;
    }

    if (state_ == AppState::Finished || shutdown_done_) {
        set_error("app cannot be run after shutdown");
        return PULSE_ERROR_INVALID_STATE;
    }

    pulse_result_t result = post_build();
    if (result != PULSE_OK) {
        return result;
    }

    state_ = AppState::Running;
    result = runner_fn_ ? runner_fn_(handle_, runner_ctx_) : default_runner();
    if (result != PULSE_OK) {
        set_error("runner failed");
    }

    shutdown();
    return result;
}

pulse_result_t App::update() {
    if (shutdown_done_) {
        set_error("app cannot be updated after shutdown");
        return PULSE_ERROR_INVALID_STATE;
    }

    world_.progress();

    for (auto& subapp : subapps_) {
        if (subapp.extract) {
            pulse_result_t result = subapp.extract(handle_, subapp.app, subapp.extract_ctx);
            if (result != PULSE_OK) {
                last_error_ = "subapp extract failed: ";
                last_error_ += subapp.name;
                return result;
            }
        }

        pulse_result_t result = subapp.app->impl.update();
        if (result != PULSE_OK) {
            last_error_ = "subapp update failed: ";
            last_error_ += subapp.name;
            return result;
        }
    }

    return PULSE_OK;
}

pulse_result_t App::set_runner(pulse_runner_fn runner, void* ctx) {
    if (state_ != AppState::Created && state_ != AppState::ReadyToRun) {
        set_error("runner can only be set before app run");
        return PULSE_ERROR_INVALID_STATE;
    }

    runner_fn_ = runner;
    runner_ctx_ = ctx;
    return PULSE_OK;
}

pulse_result_t App::insert_subapp(const char* name, pulse_app_t subapp) {
    if (!name || !name[0] || !subapp) {
        set_error("subapp name and handle are required");
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    if (state_ == AppState::Running || state_ == AppState::Finished || shutdown_done_) {
        set_error("subapps can only be inserted before app run");
        return PULSE_ERROR_INVALID_STATE;
    }

    for (const auto& entry : subapps_) {
        if (entry.name == name) {
            last_error_ = "duplicate subapp: ";
            last_error_ += name;
            return PULSE_ERROR_DUPLICATE_SUBAPP;
        }
    }

    RegisteredSubApp entry;
    entry.name = name;
    entry.app = subapp;
    subapps_.push_back(entry);
    return PULSE_OK;
}

pulse_app_t App::get_subapp(const char* name) const {
    if (!name) return nullptr;

    for (const auto& subapp : subapps_) {
        if (subapp.name == name) {
            return subapp.app;
        }
    }

    return nullptr;
}

pulse_app_t App::remove_subapp(const char* name) {
    if (!name) return nullptr;

    for (auto it = subapps_.begin(); it != subapps_.end(); ++it) {
        if (it->name == name) {
            pulse_app_t subapp = it->app;
            subapps_.erase(it);
            return subapp;
        }
    }

    return nullptr;
}

pulse_result_t App::set_subapp_extract(const char* name, pulse_subapp_extract_fn extract, void* ctx) {
    if (!name || !name[0]) {
        set_error("subapp name is required");
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    for (auto& subapp : subapps_) {
        if (subapp.name == name) {
            subapp.extract = extract;
            subapp.extract_ctx = ctx;
            return PULSE_OK;
        }
    }

    last_error_ = "subapp not found: ";
    last_error_ += name;
    return PULSE_ERROR_NOT_FOUND;
}

pulse_result_t App::extract_subapps() {
    for (auto& subapp : subapps_) {
        if (!subapp.extract) {
            continue;
        }

        pulse_result_t result = subapp.extract(handle_, subapp.app, subapp.extract_ctx);
        if (result != PULSE_OK) {
            last_error_ = "subapp extract failed: ";
            last_error_ += subapp.name;
            return result;
        }
    }

    return PULSE_OK;
}

} // namespace pulse

extern "C" {

static inline pulse::App* to_app(pulse_app_t handle) {
    return handle ? &handle->impl : nullptr;
}

pulse_app_t pulse_app_create(void) {
    return new pulse_app_o();
}

void pulse_app_destroy(pulse_app_t app) {
    if (!app) {
        return;
    }

    app->impl.shutdown();
    delete app;
}

pulse_result_t pulse_app_run(pulse_app_t app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    return impl->run();
}

pulse_result_t pulse_app_update(pulse_app_t app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    return impl->update();
}

pulse_result_t pulse_app_set_runner(pulse_app_t app, pulse_runner_fn runner, void* ctx) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    return impl->set_runner(runner, ctx);
}

pulse_result_t pulse_app_add_plugin(pulse_app_t app, const pulse_plugin_desc* desc) {
    pulse::App* impl = to_app(app);
    if (!impl || !desc) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    return impl->add_plugin(*desc);
}

bool pulse_app_has_plugin(pulse_app_t app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->has_plugin(name) : false;
}

ecs_world_t* pulse_app_world(pulse_app_t app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->world_c() : nullptr;
}

const char* pulse_app_last_error(pulse_app_t app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->last_error() : "invalid app";
}

pulse_app_t pulse_subapp_create(const char* name) {
    pulse_app_t app = pulse_app_create();
    if (app && name) {
        app->impl.set_name(name);
    }
    return app;
}

pulse_result_t pulse_app_insert_subapp(pulse_app_t app, const char* name, pulse_app_t subapp) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    return impl->insert_subapp(name, subapp);
}

pulse_app_t pulse_app_get_subapp(pulse_app_t app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->get_subapp(name) : nullptr;
}

pulse_app_t pulse_app_remove_subapp(pulse_app_t app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->remove_subapp(name) : nullptr;
}

pulse_result_t pulse_app_set_subapp_extract(
    pulse_app_t app,
    const char* name,
    pulse_subapp_extract_fn extract,
    void* ctx
) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    return impl->set_subapp_extract(name, extract, ctx);
}

pulse_result_t pulse_app_extract_subapps(pulse_app_t app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_ERROR_INVALID_ARGUMENT;
    }

    return impl->extract_subapps();
}

} // extern "C"
