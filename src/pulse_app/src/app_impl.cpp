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

namespace pulse {

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

std::string_view state_name(State state) {
    switch (state) {
        case State::Created: return "Created";
        case State::Building: return "Building";
        case State::PostBuilding: return "PostBuilding";
        case State::Running: return "Running";
        case State::Shutdown: return "Shutdown";
    }
    return "<unknown>";
}

Result App::validate_plugin(const Plugin& plugin) {
    if (plugin.name.empty()) {
        set_error("add_plugin: plugin name is required");
        return Result::InvalidArgument;
    }

    if (has_plugin(plugin.name) || has_pending_plugin(plugin.name)) {
        set_error(std::format("add_plugin: plugin '{}' is already registered", plugin.name));
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
    teardown();
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
        set_error(std::format("add_plugin: plugins can only be added before post-build (current state: {})", state_name(state_)));
        return Result::InvalidState;
    }

    Result result = validate_plugin(plugin);
    if (result != Result::Ok) {
        return result;
    }

    RegisteredPlugin entry;
    entry.plugin = plugin;
    pending_plugins_.push_back(std::move(entry));

    if (state_ == State::Building) {
        return Result::Ok;
    }

    return drain_pending_plugins();
}

Result App::run() {
    if (state_ != State::Created) {
        set_error(std::format("run: app can only be run once (current state: {})", state_name(state_)));
        return Result::InvalidState;
    }

    Result result = prepare();
    if (result != Result::Ok) {
        teardown();
        return result;
    }
    
    result = runner_fn_ ? runner_fn_(*this, runner_ctx_) : default_runner();

    teardown();
    return result;
}

Result App::update() {
    if (state_ != State::Running) {
        set_error(std::format(
            "update: can only be called while running (current state: {})",
            state_name(state_)));
        return Result::InvalidState;
    }

    world_.progress();

    for (auto& subapp : subapps_) {
        if (subapp.extract) {
            Result result = subapp.extract(*this, *subapp.app, subapp.extract_ctx);
            if (result != Result::Ok) {
                set_error(std::format("update: subapp '{}' extract failed", subapp.name));
                return result;
            }
        }

        Result result = subapp.app->update();
        if (result != Result::Ok) {
            set_error(std::format("update: subapp '{}' update() failed", subapp.name));
            return result;
        }
    }

    return Result::Ok;
}

void App::teardown() {
    if (state_ == State::Shutdown) {
        return;
    }

    ecs_quit(world_.c_ptr());

    for (auto it = subapps_.rbegin(); it != subapps_.rend(); ++it) {
        it->app->teardown();
    }

    for (auto it = plugins_.rbegin(); it != plugins_.rend(); ++it) {
        if (!it->shutdown_done && it->plugin.shutdown) {
            it->plugin.shutdown(*this, it->plugin.ctx);
        }
        it->shutdown_done = true;
    }

    state_ = State::Shutdown;
}

void App::finish() {
    request_finish_ = true;
}

bool App::should_quit() const {
    return request_finish_;
}

Result App::set_runner(Runner runner, void* ctx) {
    if (state_ == State::Running || state_ == State::Shutdown) {
        set_error(std::format("set_runner: runner can only be set before the app starts running (current state: {})", state_name(state_)));
        return Result::InvalidState;
    }

    runner_fn_ = std::move(runner);
    runner_ctx_ = ctx;
    return Result::Ok;
}

Result App::try_insert_subapp(std::string_view name, std::unique_ptr<App>& subapp) {
    if (name.empty() || !subapp) {
        set_error("try_insert_subapp: subapp name and app handle are required");
        return Result::InvalidArgument;
    }

    if (state_ == State::Running || state_ == State::Shutdown) {
        set_error(std::format("try_insert_subapp: subapps can only be inserted before the app starts running (current state: {})", state_name(state_)));
        return Result::InvalidState;
    }

    for (const auto& entry : subapps_) {
        if (entry.name == name) {
            set_error(std::format("try_insert_subapp: subapp '{}' is already registered", name));
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
    if (state_ == State::Running || state_ == State::Shutdown) {
        set_error(std::format("remove_subapp: subapps can only be removed before the app starts running (current state: {})", state_name(state_)));
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
        set_error("set_subapp_extract: subapp name is required");
        return Result::InvalidArgument;
    }

    if (state_ == State::Running || state_ == State::Shutdown) {
        set_error(std::format("set_subapp_extract: extract can only be set before the app starts running (current state: {})", state_name(state_)));
        return Result::InvalidState;
    }

    for (auto& entry : subapps_) {
        if (entry.name == name) {
            entry.extract = std::move(extract);
            entry.extract_ctx = ctx;
            return Result::Ok;
        }
    }

    set_error(std::format("set_subapp_extract: subapp '{}' not found", std::string(name)));
    return Result::NotFound;
}

Result App::post_build() {
    state_ = State::PostBuilding;

    for (auto& entry : plugins_) {
        if (entry.plugin.post_build) {
            Result result = entry.plugin.post_build(*this, entry.plugin.ctx);
            if (result != Result::Ok) {
                set_error(std::format("post_build: plugin '{}' post-build failed", entry.plugin.name));
                return result;
            }
        }
    }

    for (auto& subapp : subapps_) {
        Result result = subapp.app->post_build();
        if (result != Result::Ok) {
            set_error(std::format("post_build: subapp '{}' post-build failed", subapp.name));
            return result;
        }
    }

    state_ = State::Created;

    return Result::Ok;
}

Result App::default_runner() {
    return update();
}

Result App::drain_pending_plugins() {
    if (state_ == State::Building) {
        return Result::Ok;
    }

    state_ = State::Building;

    while (!pending_plugins_.empty()) {
        RegisteredPlugin entry = std::move(pending_plugins_.front());
        pending_plugins_.pop_front();

        if (entry.plugin.build) {
            Result result = entry.plugin.build(*this, entry.plugin.ctx);
            if (result != Result::Ok) {
                set_error(std::format("plugin build failed: {}", entry.plugin.name));
                state_ = State::Created;
                return result;
            }
        }

        plugins_.push_back(std::move(entry));
    }

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

Result App::prepare() {
    if (state_ != State::Created) {
        set_error(std::format("prepare: app can only be prepared once (current state: {})", state_name(state_)));
        return Result::InvalidState;
    }

    Result result = drain_pending_plugins();
    if (result != Result::Ok) {
        return result;
    }

    result = post_build();
    if (result != Result::Ok) {
        return result;
    }
    
    if (enable_rest_api_) {
        world_.import<flecs::stats>();
        world_.set<flecs::Rest>({});
    }

    for (auto& subapp : subapps_) {
        result = subapp.app->prepare();
        if (result != Result::Ok) {
            set_error(std::format("prepare: subapp '{}' failed", subapp.name));
            teardown();
            return result;
        }
    }

    state_ = State::Running;
    return Result::Ok;
}

void App::set_error(std::string_view message) {
    last_error_ = message;
}

} // namespace pulse
