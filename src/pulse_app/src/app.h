#pragma once

#include <deque>
#include <string>
#include <vector>

#define flecs_STATIC
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
    pulse_result_t (*build)(pulse_app_t app, void* ctx) = nullptr;
    pulse_result_t (*post_build)(pulse_app_t app, void* ctx) = nullptr;
    void (*shutdown)(pulse_app_t app, void* ctx) = nullptr;
    bool build_done = false;
    bool post_build_done = false;
    bool shutdown_done = false;
};

struct RegisteredSubApp {
    std::string name;
    pulse_app_t app = nullptr;
    pulse_subapp_extract_fn extract = nullptr;
    void* extract_ctx = nullptr;
};

class App {
public:
    explicit App(pulse_app_t handle);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    pulse_result_t add_plugin(const pulse_plugin_desc& desc);
    pulse_result_t post_build();
    void shutdown();

    pulse_result_t run();
    pulse_result_t update();

    pulse_result_t set_runner(pulse_runner_fn runner, void* ctx);
    void set_name(const char* name);
    const char* get_name() const;

    flecs::world& world() { return world_; }
    ecs_world_t* world_c() { return world_.c_ptr(); }

    bool has_plugin(const char* name) const;

    pulse_result_t insert_subapp(const char* name, pulse_app_t subapp);
    pulse_app_t get_subapp(const char* name) const;
    pulse_app_t remove_subapp(const char* name);
    pulse_result_t set_subapp_extract(const char* name, pulse_subapp_extract_fn extract, void* ctx);
    pulse_result_t extract_subapps();

    AppState state() const { return state_; }
    const char* last_error() const;

private:
    flecs::world world_;
    std::vector<RegisteredPlugin> plugins_;
    std::deque<RegisteredPlugin> pending_plugins_;
    std::vector<RegisteredSubApp> subapps_;
    pulse_app_t handle_ = nullptr;
    pulse_runner_fn runner_fn_ = nullptr;
    void* runner_ctx_ = nullptr;
    AppState state_ = AppState::Created;
    bool draining_plugins_ = false;
    bool post_build_done_ = false;
    bool shutdown_done_ = false;
    std::string name_;
    std::string last_error_;

    pulse_result_t default_runner();
    pulse_result_t drain_pending_plugins();
    pulse_result_t validate_plugin_desc(const pulse_plugin_desc& desc);
    bool has_pending_plugin(const char* name) const;
    void set_error(const char* message);
};

} // namespace pulse
