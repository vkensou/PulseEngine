module;

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>
#include <format>

#include <flecs.h>
#include "pulse_app.h"
#include "app_internal.h"

module pulse_app;

namespace {

void time_system_run(flecs::iter& it, size_t i, PulseTimer& ctx) {
    const ecs_world_info_t* info = ecs_get_world_info(it.world());
    ctx.delta_time = info->delta_time;
    ctx.time_since_startup = static_cast<float>(info->world_time_total);
    ctx.delta_time_double = info->delta_time;
    ctx.time_since_startup_double = info->world_time_total;
    ctx.fps = info->delta_time > 0.0f ? static_cast<int32_t>(1.0f / info->delta_time + 0.5f) : 0;
}

void install_time_system(flecs::world& world) {
    world.system<PulseTimer>("PulseTimeSystem").kind(EcsPreUpdate).each(time_system_run);
}

} // namespace

namespace pulse {

Result App::validate_plugin(const Plugin& plugin) {
    if (plugin.name.empty()) {
        set_error("plugin name is required");
        return Result::InvalidArgument;
    }

    if (has_plugin(plugin.name) || has_pending_plugin(plugin.name)) {
        set_error(std::format("duplicate plugin: {}", plugin.name));
        return Result::DuplicatePlugin;
    }

    return Result::Ok;
}

App::App(const AppDesc& desc)
    : name_(desc.name), enable_rest_api_(desc.enable_rest_api) {
    ecs_world_t* w = world_.c_ptr();
    ECS_COMPONENT_DEFINE(w, PulseTimer);
    ECS_COMPONENT_DEFINE(w, pulse_app_state_resource);

    world_.component<PulseTimer>("PulseTimer", true, ecs_id(PulseTimer));

    PulseTimer time_ctx{};
    world_.set<PulseTimer>(time_ctx);

    install_time_system(world_);

    pulse_app_state_resource res{ .app = reinterpret_cast<PulseAppId>(this) };
    world_.set<pulse_app_state_resource>(res);
}

App::~App() {
    shutdown();
}

bool App::has_plugin(std::string_view name) const {
    for (const auto& entry : plugins_) {
        if (entry.plugin.name == name) {
            return true;
        }
    }
    return false;
}

Result App::add_plugin(Plugin plugin) {
    if (state_ != State::Created && state_ != State::Building) {
        set_error("plugins can only be added before app run");
        return Result::InvalidState;
    }

    Result result = validate_plugin(plugin);
    if (result != Result::Ok) {
        return result;
    }

    RegisteredPlugin entry;
    entry.plugin = plugin;
    pending_plugins_.push_back(std::move(entry));

    if (draining_plugins_) {
        return Result::Ok;
    }

    return drain_pending_plugins();
}

Result App::run() {
    if (state_ == State::Running) {
        set_error("app is already running");
        return Result::InvalidState;
    }

    if (state_ == State::Finished || state_ == State::Shutdown) {
        set_error("app cannot be run after shutdown");
        return Result::InvalidState;
    }

    Result result = post_build();
    if (result != Result::Ok) {
        return result;
    }

    if (enable_rest_api_) {
        world_.import<flecs::stats>();
        world_.set<flecs::Rest>({});
    }

    state_ = State::Running;
    result = runner_fn_ ? runner_fn_(*this, runner_ctx_) : default_runner();

    state_ = State::Finished;
    return result;
}

Result App::update() {
    if (state_ == State::Finished || state_ == State::Shutdown) {
        set_error("app cannot be updated after finished");
        return Result::InvalidState;
    }

    world_.progress();

    for (auto& subapp : subapps_) {
        if (subapp.extract) {
            Result result = subapp.extract(*this, *subapp.app, subapp.extract_ctx);
            if (result != Result::Ok) {
                set_error(std::format("subapp extract failed: {}", subapp.name));
                return result;
            }
        }

        Result result = subapp.app->update();
        if (result != Result::Ok) {
            set_error(std::format("subapp update failed: {}", subapp.name));
            return result;
        }
    }

    return Result::Ok;
}

void App::shutdown() {
    if (state_ == State::Shutdown) {
        return;
    }

    ecs_quit(world_.c_ptr());

    for (auto it = subapps_.rbegin(); it != subapps_.rend(); ++it) {
        it->app->shutdown();
    }

    for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
        if (!it->shutdown_done && it->plugin.shutdown) {
            it->plugin.shutdown(*this, it->plugin.ctx);
        }
        it->shutdown_done = true;
    }

    state_ = State::Shutdown;
}

Result App::set_runner(Runner runner, void* ctx) {
    if (state_ != State::Created && state_ != State::ReadyToRun) {
        set_error("runner can only be set before app run");
        return Result::InvalidState;
    }

    runner_fn_ = std::move(runner);
    runner_ctx_ = ctx;
    return Result::Ok;
}

Result App::try_insert_subapp(std::string_view name, std::unique_ptr<App>& subapp) {
    if (name.empty() || !subapp) {
        set_error("subapp name and handle are required");
        return Result::InvalidArgument;
    }

    if (state_ == State::Running || state_ == State::Finished || state_ == State::Shutdown) {
        set_error("subapps can only be inserted before app run");
        return Result::InvalidState;
    }

    for (const auto& entry : subapps_) {
        if (entry.name == name) {
            set_error(std::format("duplicate subapp: {}", name));
            return Result::DuplicateSubapp;
        }
    }

    RegisteredSubApp entry;
    entry.name = name;
    entry.app = std::move(subapp);
    subapps_.push_back(std::move(entry));
    return Result::Ok;
}

App* App::get_subapp(std::string_view name) const {
    for (const auto& entry : subapps_) {
        if (entry.name == name) {
            return entry.app.get();
        }
    }
    return nullptr;
}

std::unique_ptr<App> App::remove_subapp(std::string_view name) {
    if (state_ == State::Running) {
        set_error("subapps can only be removed before app run");
        return {};
    }

    for (auto it = subapps_.begin(); it != subapps_.end(); ++it) {
        if (it->name == name) {
            std::unique_ptr<App> subapp = std::move(it->app);
            subapps_.erase(it);
            return std::move(subapp);
        }
    }
    return nullptr;
}

Result App::set_subapp_extract(std::string_view name, SubappExtract extract, void* ctx) {
    if (name.empty()) {
        set_error("subapp name is required");
        return Result::InvalidArgument;
    }

    for (auto& entry : subapps_) {
        if (entry.name == name) {
            entry.extract = std::move(extract);
            entry.extract_ctx = ctx;
            return Result::Ok;
        }
    }

    set_error(std::format("subapp not found: {}", std::string(name)));
    return Result::NotFound;
}

Result App::extract_subapps() {
    for (auto& entry : subapps_) {
        if (!entry.extract) {
            continue;
        }

        Result result = entry.extract(*this, *entry.app, entry.extract_ctx);
        if (result != Result::Ok) {
            set_error(std::format("subapp extract failed: {}", entry.name));
            return result;
        }
    }

    return Result::Ok;
}

Result App::post_build() {
    if (state_ == State::Shutdown || state_ == State::Running || state_ == State::Finished) {
        set_error("app is not in a post-buildable state");
        return Result::InvalidState;
    }

    if (post_build_done_) {
        return Result::Ok;
    }

    Result result = drain_pending_plugins();
    if (result != Result::Ok) {
        return result;
    }

    state_ = State::ReadyToRun;

    for (auto& entry : plugins_) {
        if (!entry.post_build_done && entry.plugin.post_build) {
            result = entry.plugin.post_build(*this, entry.plugin.ctx);
            if (result != Result::Ok) {
                set_error(std::format("plugin post_build failed: {}", entry.plugin.name));
                return result;
            }
        }
        entry.post_build_done = true;
    }

    for (auto& subapp : subapps_) {
        result = subapp.app->post_build();
        if (result != Result::Ok) {
            set_error(std::format("subapp post_build failed: {}", subapp.name));
            return result;
        }
    }

    post_build_done_ = true;
    return Result::Ok;
}

Result App::default_runner() {
    return update();
}

Result App::drain_pending_plugins() {
    if (draining_plugins_) {
        return Result::Ok;
    }

    draining_plugins_ = true;
    state_ = State::Building;

    while (!pending_plugins_.empty()) {
        RegisteredPlugin entry = std::move(pending_plugins_.front());
        pending_plugins_.pop_front();

        if (entry.plugin.build) {
            Result result = entry.plugin.build(*this, entry.plugin.ctx);
            if (result != Result::Ok) {
                set_error(std::format("plugin build failed: {}", entry.plugin.name));
                draining_plugins_ = false;
                state_ = State::BuildFailed;
                return result;
            }
        }

        plugins_.push_back(std::move(entry));
    }

    draining_plugins_ = false;
    state_ = State::Created;
    return Result::Ok;
}

bool App::has_pending_plugin(std::string_view name) const {
    for (const auto& entry : pending_plugins_) {
        if (entry.plugin.name == name) {
            return true;
        }
    }
    return false;
}

void App::set_error(std::string_view message) {
    last_error_ = message;
}

} // namespace pulse
