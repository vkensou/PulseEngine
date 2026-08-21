module;

#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include <flecs.h>
#include "pulse_app.h"

export module pulse_app;

namespace pulse {

export enum class Result {
    Ok = PULSE_RESULT_OK,
    InvalidArgument = PULSE_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_RESULT_ERROR_INVALID_STATE,
    DuplicatePlugin = PULSE_RESULT_ERROR_DUPLICATE_PLUGIN,
    DuplicateSubapp = PULSE_RESULT_ERROR_DUPLICATE_SUBAPP,
    NotFound = PULSE_RESULT_ERROR_NOT_FOUND,
    Internal = PULSE_RESULT_ERROR_INTERNAL,
};

export enum class State {
    Created,
    Building,
    PostBuilding,
    Running,
    Shutdown,
};

class App;

export struct AppDesc {
    std::string name;
    bool enable_rest_api = false;
};

export struct Plugin {
    std::string name;
    void* ctx = nullptr;
    std::function<Result(App&, void*)> build;
    std::function<Result(App&, void*)> post_build;
    std::function<void(App&, void*)> shutdown;
};

export using Runner = std::function<Result(App&, void*)>;
export using SubappExtract = std::function<Result(App& parent, App& subapp, void* ctx)>;

struct RegisteredPlugin {
    Plugin plugin;
    bool shutdown_done = false;
};

struct RegisteredSubApp {
    std::string name;
    std::unique_ptr<App> app;
    SubappExtract extract;
    void* extract_ctx = nullptr;
};

export class App {
public:
    explicit App(const AppDesc& desc);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) = delete;
    App& operator=(App&&) = delete;

    bool has_plugin(std::string_view name) const;
    Result add_plugin(Plugin plugin);

    Result run();
    Result update();
    void finish();
    bool should_quit() const;

    Result set_runner(Runner runner, void* ctx);

    flecs::world& world() { return world_; }
    std::string_view name() const { return name_; }
    State state() const { return state_; }
    std::string_view last_error() const { return last_error_; }

    Result try_insert_subapp(std::string_view name, std::unique_ptr<App>& subapp);
    App* get_subapp(std::string_view name) const;
    std::unique_ptr<App> remove_subapp(std::string_view name);
    Result set_subapp_extract(std::string_view name, SubappExtract extract, void* ctx);
    Result extract_subapps();

private:
    flecs::world world_;
    std::vector<RegisteredPlugin> plugins_;
    std::deque<RegisteredPlugin> pending_plugins_;
    std::vector<RegisteredSubApp> subapps_;
    Runner runner_fn_;
    void* runner_ctx_ = nullptr;
    State state_ = State::Created;
    bool request_finish_ = false;
    std::string name_;
    std::string last_error_;
    bool enable_rest_api_ = false;

    Result post_build();
    Result default_runner();
    Result drain_pending_plugins();
    Result prepare();
    Result validate_plugin(const Plugin& plugin);
    bool has_pending_plugin(std::string_view name) const;
    void set_error(std::string_view message);
    void teardown();
};

} // namespace pulse
