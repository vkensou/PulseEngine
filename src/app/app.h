#pragma once

#include <stdint.h>
#include <vector>
#include <string>
#include <unordered_map>

#define flecs_STATIC
#include <flecs.h>

extern "C" {
#include "pulse_app.h"
}

namespace pulse {

enum class AppState {
    Adding = 0,
    Ready  = 1,
    Finished = 2,
    Cleaned = 3,
};

struct RegisteredPlugin {
    pulse_plugin_desc desc;
    bool build_done   = false;
    bool ready_done   = false;
    bool finish_done  = false;
    bool cleanup_done = false;
};

class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void add_plugin(const pulse_plugin_desc& desc);
    void add_plugin_to_registry(const pulse_plugin_desc& desc);
    void run();
    void update();

    void set_runner(pulse_runner_fn runner, void* ctx);
    void set_name(const char* name);
    const char* get_name() const;

    flecs::world& world() { return world_; }
    ecs_world_t* world_c() { return world_.c_ptr(); }

    bool is_plugin_added(const char* name) const;
    AppState plugins_state() const { return state_; }

    void insert_subapp(const char* name, App* subapp);
    App* get_subapp(const char* name);
    App* remove_subapp(const char* name);

    struct InitPhase {};
    struct UpdatePhase {};
    struct PostUpdatePhase {};
    struct RenderPhase {};
    struct ImguiPhase {};

private:
    flecs::world world_;
    std::vector<RegisteredPlugin> plugins_;
    std::unordered_map<std::string, App*> subapps_;
    pulse_runner_fn runner_fn_ = nullptr;
    void* runner_ctx_ = nullptr;
    AppState state_ = AppState::Adding;
    std::string name_;

    void setup_phases();
    void poll_ready();
    void call_finish();
    void call_cleanup();
    void default_runner();
};

} // namespace pulse
