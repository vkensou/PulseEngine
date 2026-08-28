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

namespace {

RunResult to_run_result(PrepareResult result) {
    switch (result) {
        case PrepareResult::Ok: return RunResult::Ok;
        case PrepareResult::InvalidArgument: return RunResult::InvalidArgument;
        case PrepareResult::InvalidState: return RunResult::InvalidState;
        case PrepareResult::PluginBuildFailed: return RunResult::PluginBuildFailed;
        case PrepareResult::MissingPluginDependency: return RunResult::MissingPluginDependency;
        case PrepareResult::CircularPluginDependency: return RunResult::CircularPluginDependency;
        case PrepareResult::PluginPostBuildFailed: return RunResult::PluginPostBuildFailed;
        case PrepareResult::SubappPostBuildFailed: return RunResult::SubappPostBuildFailed;
        case PrepareResult::SubappPrepareFailed: return RunResult::SubappPrepareFailed;
        default: return RunResult::Internal;
    }
}

RunResult to_run_result(RunnerResult result) {
    switch (result) {
        case RunnerResult::Ok: return RunResult::Ok;
        case RunnerResult::InvalidArgument: return RunResult::InvalidArgument;
        case RunnerResult::InvalidState: return RunResult::InvalidState;
        case RunnerResult::SubappExtractFailed: return RunResult::SubappExtractFailed;
        case RunnerResult::SubappUpdateFailed: return RunResult::SubappUpdateFailed;
        default: return RunResult::Internal;
    }
}

RunResult to_run_result(UpdateResult result) {
    switch (result) {
        case UpdateResult::Ok: return RunResult::Ok;
        case UpdateResult::InvalidArgument: return RunResult::InvalidArgument;
        case UpdateResult::InvalidState: return RunResult::InvalidState;
        case UpdateResult::SubappExtractFailed: return RunResult::SubappExtractFailed;
        case UpdateResult::SubappUpdateFailed: return RunResult::SubappUpdateFailed;
        default: return RunResult::Internal;
    }
}

} // namespace

AddPluginResult App::validate_plugin(const Plugin& plugin) {
    if (plugin.name.empty()) {
        set_error("add_plugin: plugin name is required");
        return AddPluginResult::InvalidArgument;
    }

    if (has_plugin(plugin.name) || has_pending_plugin(plugin.name)) {
        set_error(std::format("add_plugin: plugin '{}' is already registered", plugin.name));
        return AddPluginResult::DuplicatePlugin;
    }

    return AddPluginResult::Ok;
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

bool App::plugin_dependencies_satisfied(const Plugin& plugin) const {
    // A plugin is ready to build only when every dependency has already been
    // built.  Pending dependencies are not sufficient; they merely mean the
    // dependency exists somewhere in the queue.
    for (const auto& dependency : plugin.dependencies) {
        if (!has_plugin(dependency)) {
            return false;
        }
    }
    return true;
}

bool App::plugin_dependencies_registered(const Plugin& plugin) const {
    for (const auto& dependency : plugin.dependencies) {
        if (!has_plugin(dependency) && !has_pending_plugin(dependency)) {
            return false;
        }
    }
    return true;
}

bool App::pending_plugins_satisfiable() const {
    for (const auto& entry : pending_plugins_) {
        if (!plugin_dependencies_registered(entry.plugin)) {
            return false;
        }
    }
    return true;
}

AddPluginResult App::add_plugin(Plugin plugin) {
    if (state_ != State::Created && state_ != State::Building) {
        set_error(std::format("add_plugin: plugins can only be added before post-build (current state: {})", state_name(state_)));
        return AddPluginResult::InvalidState;
    }

    AddPluginResult result = validate_plugin(plugin);
    if (result != AddPluginResult::Ok) {
        return result;
    }

    RegisteredPlugin entry;
    entry.plugin = std::move(plugin);
    pending_plugins_.push_back(std::move(entry));

    if (state_ == State::Building) {
        return AddPluginResult::Ok;
    }

    // Keep the old eager-build behavior for the normal case: as soon as the
    // pending set is satisfiable, drain it.  If a dependency has not been
    // registered yet, stay in pending so callers can register in any order.
    if (pending_plugins_satisfiable()) {
        return drain_pending_plugins();
    }

    return AddPluginResult::Ok;
}

RunResult App::run() {
    if (state_ != State::Created) {
        set_error(std::format("run: app can only be run once (current state: {})", state_name(state_)));
        return RunResult::InvalidState;
    }

    PrepareResult result = prepare();
    if (result != PrepareResult::Ok) {
        teardown();
        return to_run_result(result);
    }

    RunResult run_result = runner_fn_ ? to_run_result(runner_fn_(*this, runner_ctx_)) : default_runner();

    teardown();
    return run_result;
}

UpdateResult App::update() {
    if (state_ != State::Running) {
        set_error(std::format(
            "update: can only be called while running (current state: {})",
            state_name(state_)));
        return UpdateResult::InvalidState;
    }

    world_.progress();

    for (auto& subapp : subapps_) {
        if (subapp.extract) {
            SubappExtractResult result = subapp.extract(*this, *subapp.app, subapp.extract_ctx);
            if (result != SubappExtractResult::Ok) {
                set_error(std::format("update: subapp '{}' extract failed", subapp.name));
                return UpdateResult::SubappExtractFailed;
            }
        }

        UpdateResult result = subapp.app->update();
        if (result != UpdateResult::Ok) {
            set_error(std::format("update: subapp '{}' update() failed", subapp.name));
            return UpdateResult::SubappUpdateFailed;
        }
    }

    return UpdateResult::Ok;
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

SetRunnerResult App::set_runner(Runner runner, void* ctx) {
    if (state_ == State::Running || state_ == State::Shutdown) {
        set_error(std::format("set_runner: runner can only be set before the app starts running (current state: {})", state_name(state_)));
        return SetRunnerResult::InvalidState;
    }

    runner_fn_ = std::move(runner);
    runner_ctx_ = ctx;
    return SetRunnerResult::Ok;
}

InsertSubappResult App::try_insert_subapp(std::string_view name, std::unique_ptr<App>& subapp) {
    if (name.empty() || !subapp) {
        set_error("try_insert_subapp: subapp name and app handle are required");
        return InsertSubappResult::InvalidArgument;
    }

    if (state_ == State::Running || state_ == State::Shutdown) {
        set_error(std::format("try_insert_subapp: subapps can only be inserted before the app starts running (current state: {})", state_name(state_)));
        return InsertSubappResult::InvalidState;
    }

    for (const auto& entry : subapps_) {
        if (entry.name == name) {
            set_error(std::format("try_insert_subapp: subapp '{}' is already registered", name));
            return InsertSubappResult::DuplicateSubapp;
        }
    }

    RegisteredSubApp entry;
    entry.name = name;
    entry.app = std::move(subapp);
    subapps_.push_back(std::move(entry));
    return InsertSubappResult::Ok;
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

SetSubappExtractResult App::set_subapp_extract(std::string_view name, SubappExtract extract, void* ctx) {
    if (name.empty()) {
        set_error("set_subapp_extract: subapp name is required");
        return SetSubappExtractResult::InvalidArgument;
    }

    if (state_ == State::Running || state_ == State::Shutdown) {
        set_error(std::format("set_subapp_extract: extract can only be set before the app starts running (current state: {})", state_name(state_)));
        return SetSubappExtractResult::InvalidState;
    }

    for (auto& entry : subapps_) {
        if (entry.name == name) {
            entry.extract = std::move(extract);
            entry.extract_ctx = ctx;
            return SetSubappExtractResult::Ok;
        }
    }

    set_error(std::format("set_subapp_extract: subapp '{}' not found", std::string(name)));
    return SetSubappExtractResult::NotFound;
}

PrepareResult App::post_build() {
    state_ = State::PostBuilding;

    for (auto& entry : plugins_) {
        if (entry.plugin.post_build) {
            PluginBuildResult result = entry.plugin.post_build(*this, entry.plugin.ctx);
            if (result != PluginBuildResult::Ok) {
                set_error(std::format("post_build: plugin '{}' post-build failed", entry.plugin.name));
                return PrepareResult::PluginPostBuildFailed;
            }
        }
    }

    for (auto& subapp : subapps_) {
        PrepareResult result = subapp.app->post_build();
        if (result != PrepareResult::Ok) {
            set_error(std::format("post_build: subapp '{}' post-build failed", subapp.name));
            return PrepareResult::SubappPostBuildFailed;
        }
    }

    state_ = State::Created;

    return PrepareResult::Ok;
}

RunResult App::default_runner() {
    return to_run_result(update());
}

AddPluginResult App::drain_pending_plugins() {
    if (state_ == State::Building) {
        return AddPluginResult::Ok;
    }

    state_ = State::Building;

    while (!pending_plugins_.empty()) {
        // Collect every currently satisfiable plugin without building yet.
        // Nested add_plugin calls during build can safely append to the deque
        // because we no longer hold iterators into it at that point.
        std::vector<RegisteredPlugin> ready;
        ready.reserve(pending_plugins_.size());
        for (auto it = pending_plugins_.begin(); it != pending_plugins_.end(); ) {
            if (!plugin_dependencies_satisfied(it->plugin)) {
                ++it;
                continue;
            }

            ready.push_back(std::move(*it));
            it = pending_plugins_.erase(it);
        }

        if (ready.empty()) {
            // Nothing could be built.  Distinguish a missing dependency from a cycle.
            for (const auto& pending : pending_plugins_) {
                for (const auto& dependency : pending.plugin.dependencies) {
                    if (!has_plugin(dependency) && !has_pending_plugin(dependency)) {
                        set_error(std::format(
                            "plugin '{}' depends on missing plugin '{}'",
                            pending.plugin.name, dependency));
                        state_ = State::Created;
                        return AddPluginResult::MissingPluginDependency;
                    }
                }
            }

            const std::string cycle = find_circular_dependency();
            set_error(cycle.empty()
                ? "circular plugin dependency detected"
                : std::format("circular plugin dependency detected: {}", cycle));
            state_ = State::Created;
            return AddPluginResult::CircularPluginDependency;
        }

        for (auto& entry : ready) {
            // Insert before build so plugins added from inside this build can
            // declare a dependency on the plugin currently being built.
            plugins_.push_back(std::move(entry));
            auto& built = plugins_.back();

            PluginBuildResult result = PluginBuildResult::Ok;
            if (built.plugin.build) {
                result = built.plugin.build(*this, built.plugin.ctx);
            }

            if (result != PluginBuildResult::Ok) {
                set_error(std::format("plugin build failed: {}", built.plugin.name));
                plugins_.pop_back();
                state_ = State::Created;
                switch (result) {
                    case PluginBuildResult::InvalidArgument: return AddPluginResult::InvalidArgument;
                    case PluginBuildResult::InvalidState: return AddPluginResult::InvalidState;
                    case PluginBuildResult::DuplicatePlugin: return AddPluginResult::DuplicatePlugin;
                    case PluginBuildResult::Internal: return AddPluginResult::Internal;
                    default: return AddPluginResult::Internal;
                }
            }
        }
    }

    state_ = State::Created;
    return AddPluginResult::Ok;
}

bool App::has_pending_plugin(std::string_view name) const {
    for (const auto& entry : pending_plugins_) {
        if (entry.plugin.name == name) {
            return true;
        }
    }
    return false;
}

std::string App::find_circular_dependency() const {
    const auto index_of = [this](std::string_view name) -> int {
        int i = 0;
        for (const auto& entry : pending_plugins_) {
            if (entry.plugin.name == name) {
                return i;
            }
            ++i;
        }
        return -1;
    };

    const int count = static_cast<int>(pending_plugins_.size());
    std::vector<int> visit_state(count, 0);
    std::vector<int> parent(count, -1);
    std::string result;

    std::function<bool(int)> dfs = [&](int index) -> bool {
        visit_state[index] = 1;
        for (const auto& dependency : pending_plugins_[index].plugin.dependencies) {
            int next = index_of(dependency);
            if (next < 0) {
                continue;
            }
            if (visit_state[next] == 1) {
                result = pending_plugins_[next].plugin.name;
                int current = index;
                while (current != next) {
                    result = pending_plugins_[current].plugin.name + " -> " + result;
                    current = parent[current];
                }
                result = pending_plugins_[next].plugin.name + " -> " + result;
                return true;
            }
            if (visit_state[next] == 0) {
                parent[next] = index;
                if (dfs(next)) {
                    return true;
                }
            }
        }
        visit_state[index] = 2;
        return false;
    };

    for (int i = 0; i < count; ++i) {
        if (visit_state[i] == 0 && dfs(i)) {
            return result;
        }
    }
    return {};
}

PrepareResult App::prepare() {
    if (state_ != State::Created) {
        set_error(std::format("prepare: app can only be prepared once (current state: {})", state_name(state_)));
        return PrepareResult::InvalidState;
    }

    AddPluginResult result = drain_pending_plugins();
    if (result != AddPluginResult::Ok) {
        switch (result) {
            case AddPluginResult::InvalidArgument: return PrepareResult::InvalidArgument;
            case AddPluginResult::InvalidState: return PrepareResult::InvalidState;
            case AddPluginResult::DuplicatePlugin: return PrepareResult::PluginBuildFailed;
            case AddPluginResult::PluginBuildFailed: return PrepareResult::PluginBuildFailed;
            case AddPluginResult::MissingPluginDependency: return PrepareResult::MissingPluginDependency;
            case AddPluginResult::CircularPluginDependency: return PrepareResult::CircularPluginDependency;
            case AddPluginResult::Internal: return PrepareResult::Internal;
            default: return PrepareResult::Internal;
        }
    }

    PrepareResult prepare_result = post_build();
    if (prepare_result != PrepareResult::Ok) {
        return prepare_result;
    }

    if (enable_rest_api_) {
        world_.import<flecs::stats>();
        world_.set<flecs::Rest>({});
    }

    for (auto& subapp : subapps_) {
        PrepareResult subapp_result = subapp.app->prepare();
        if (subapp_result != PrepareResult::Ok) {
            set_error(std::format("prepare: subapp '{}' failed", subapp.name));
            teardown();
            return PrepareResult::SubappPrepareFailed;
        }
    }

    state_ = State::Running;
    return PrepareResult::Ok;
}

void App::set_error(std::string_view message) {
    last_error_ = message;
}

} // namespace pulse
