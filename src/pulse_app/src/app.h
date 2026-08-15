#pragma once

#include <deque>
#include <string>
#include <vector>

#include <flecs.h>

extern "C" {
#include "pulse_app.h"
}

namespace pulse {

enum class AppState {
    Created,
    Building,
    ReadyToRun,
    Running,
    Finished,
};

struct RegisteredPlugin {
    std::string name;
    void* ctx = nullptr;
    EPulseResult (*build)(PulseAppId app, void* ctx) = nullptr;
    EPulseResult (*post_build)(PulseAppId app, void* ctx) = nullptr;
    void (*shutdown)(PulseAppId app, void* ctx) = nullptr;
    bool build_done = false;
    bool post_build_done = false;
    bool shutdown_done = false;
};

struct RegisteredSubApp {
    std::string name;
    PulseAppId app = nullptr;
    PulseProcSubappExtractFn extract = nullptr;
    void* extract_ctx = nullptr;
};

class App {
public:
    explicit App(PulseAppId handle, PulseAppDesc* desc);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    EPulseResult add_plugin(const PulsePluginDesc& desc);
    EPulseResult post_build();
    void shutdown();

    EPulseResult run();
    EPulseResult update();

    EPulseResult set_runner(PulseProcRunnerFn runner, void* ctx);
    void set_name(const char* name);
    const char* get_name() const;

    flecs::world& world() { return world_; }
    ecs_world_t* world_c() { return world_.c_ptr(); }

    bool has_plugin(const char* name) const;

    EPulseResult insert_subapp(const char* name, PulseAppId subapp);
    PulseAppId get_subapp(const char* name) const;
    PulseAppId remove_subapp(const char* name);
    EPulseResult set_subapp_extract(const char* name, PulseProcSubappExtractFn extract, void* ctx);
    EPulseResult extract_subapps();

    AppState state() const { return state_; }
    const char* last_error() const;

private:
    flecs::world world_;
    std::vector<RegisteredPlugin> plugins_;
    std::deque<RegisteredPlugin> pending_plugins_;
    std::vector<RegisteredSubApp> subapps_;
    PulseAppId handle_ = nullptr;
    PulseProcRunnerFn runner_fn_ = nullptr;
    void* runner_ctx_ = nullptr;
    AppState state_ = AppState::Created;
    bool draining_plugins_ = false;
    bool post_build_done_ = false;
    bool shutdown_done_ = false;
    std::string name_;
    std::string last_error_;
    bool enableRESTApi;

    EPulseResult default_runner();
    EPulseResult drain_pending_plugins();
    EPulseResult validate_plugin_desc(const PulsePluginDesc& desc);
    bool has_pending_plugin(const char* name) const;
    void set_error(const char* message);
};

} // namespace pulse
