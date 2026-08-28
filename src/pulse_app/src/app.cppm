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

export enum class RunResult {
    Ok = PULSE_APP_RUN_RESULT_OK,
    InvalidArgument = PULSE_APP_RUN_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_APP_RUN_RESULT_ERROR_INVALID_STATE,
    PluginBuildFailed = PULSE_APP_RUN_RESULT_ERROR_PLUGIN_BUILD_FAILED,
    MissingPluginDependency = PULSE_APP_RUN_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY,
    CircularPluginDependency = PULSE_APP_RUN_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY,
    PluginPostBuildFailed = PULSE_APP_RUN_RESULT_ERROR_PLUGIN_POST_BUILD_FAILED,
    SubappPostBuildFailed = PULSE_APP_RUN_RESULT_ERROR_SUBAPP_POST_BUILD_FAILED,
    SubappPrepareFailed = PULSE_APP_RUN_RESULT_ERROR_SUBAPP_PREPARE_FAILED,
    SubappExtractFailed = PULSE_APP_RUN_RESULT_ERROR_SUBAPP_EXTRACT_FAILED,
    SubappUpdateFailed = PULSE_APP_RUN_RESULT_ERROR_SUBAPP_UPDATE_FAILED,
    Internal = PULSE_APP_RUN_RESULT_ERROR_INTERNAL,
};

export enum class PrepareResult {
    Ok = PULSE_APP_PREPARE_RESULT_OK,
    InvalidArgument = PULSE_APP_PREPARE_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_APP_PREPARE_RESULT_ERROR_INVALID_STATE,
    PluginBuildFailed = PULSE_APP_PREPARE_RESULT_ERROR_PLUGIN_BUILD_FAILED,
    MissingPluginDependency = PULSE_APP_PREPARE_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY,
    CircularPluginDependency = PULSE_APP_PREPARE_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY,
    PluginPostBuildFailed = PULSE_APP_PREPARE_RESULT_ERROR_PLUGIN_POST_BUILD_FAILED,
    SubappPostBuildFailed = PULSE_APP_PREPARE_RESULT_ERROR_SUBAPP_POST_BUILD_FAILED,
    SubappPrepareFailed = PULSE_APP_PREPARE_RESULT_ERROR_SUBAPP_PREPARE_FAILED,
    Internal = PULSE_APP_PREPARE_RESULT_ERROR_INTERNAL,
};

export enum class UpdateResult {
    Ok = PULSE_APP_UPDATE_RESULT_OK,
    InvalidArgument = PULSE_APP_UPDATE_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_APP_UPDATE_RESULT_ERROR_INVALID_STATE,
    SubappExtractFailed = PULSE_APP_UPDATE_RESULT_ERROR_SUBAPP_EXTRACT_FAILED,
    SubappUpdateFailed = PULSE_APP_UPDATE_RESULT_ERROR_SUBAPP_UPDATE_FAILED,
    Internal = PULSE_APP_UPDATE_RESULT_ERROR_INTERNAL,
};

export enum class SetRunnerResult {
    Ok = PULSE_APP_SET_RUNNER_RESULT_OK,
    InvalidArgument = PULSE_APP_SET_RUNNER_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_APP_SET_RUNNER_RESULT_ERROR_INVALID_STATE,
    Internal = PULSE_APP_SET_RUNNER_RESULT_ERROR_INTERNAL,
};

export enum class AddPluginResult {
    Ok = PULSE_APP_ADD_PLUGIN_RESULT_OK,
    InvalidArgument = PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_STATE,
    DuplicatePlugin = PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN,
    PluginBuildFailed = PULSE_APP_ADD_PLUGIN_RESULT_ERROR_PLUGIN_BUILD_FAILED,
    MissingPluginDependency = PULSE_APP_ADD_PLUGIN_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY,
    CircularPluginDependency = PULSE_APP_ADD_PLUGIN_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY,
    Internal = PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INTERNAL,
};

export enum class InsertSubappResult {
    Ok = PULSE_APP_INSERT_SUBAPP_RESULT_OK,
    InvalidArgument = PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INVALID_STATE,
    DuplicateSubapp = PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_DUPLICATE_SUBAPP,
    Internal = PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INTERNAL,
};

export enum class SetSubappExtractResult {
    Ok = PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_OK,
    InvalidArgument = PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_STATE,
    NotFound = PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_NOT_FOUND,
    Internal = PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INTERNAL,
};

export enum class RunnerResult {
    Ok = PULSE_RUNNER_RESULT_OK,
    InvalidArgument = PULSE_RUNNER_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_RUNNER_RESULT_ERROR_INVALID_STATE,
    SubappExtractFailed = PULSE_RUNNER_RESULT_ERROR_SUBAPP_EXTRACT_FAILED,
    SubappUpdateFailed = PULSE_RUNNER_RESULT_ERROR_SUBAPP_UPDATE_FAILED,
    Internal = PULSE_RUNNER_RESULT_ERROR_INTERNAL,
};

export enum class SubappExtractResult {
    Ok = PULSE_SUBAPP_EXTRACT_RESULT_OK,
    InvalidArgument = PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_STATE,
    Internal = PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INTERNAL,
};

export enum class PluginBuildResult {
    Ok = PULSE_PLUGIN_BUILD_RESULT_OK,
    InvalidArgument = PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT,
    InvalidState = PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE,
    DuplicatePlugin = PULSE_PLUGIN_BUILD_RESULT_ERROR_DUPLICATE_PLUGIN,
    Internal = PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL,
};

export enum class State {
    Created,
    Building,
    PostBuilding,
    Running,
    Shutdown,
};

export class App;

export struct AppDesc {
    std::string name;
    bool enable_rest_api = false;
};

export struct Plugin {
    std::string name;
    uint32_t plugin_version = 0;
    void* ctx = nullptr;
    std::function<PluginBuildResult(App&, void*)> build;
    std::function<PluginBuildResult(App&, void*)> post_build;
    std::function<void(App&, void*)> shutdown;
    std::vector<std::string> dependencies;
};

export using Runner = std::function<RunnerResult(App&, void*)>;
export using SubappExtract = std::function<SubappExtractResult(App& parent, App& subapp, void* ctx)>;

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
    AddPluginResult add_plugin(Plugin plugin);

    RunResult run();
    PrepareResult prepare();
    UpdateResult update();
    void teardown();
    void finish();
    bool should_quit() const;

    SetRunnerResult set_runner(Runner runner, void* ctx);

    flecs::world& world() { return world_; }
    std::string_view name() const { return name_; }
    State state() const { return state_; }
    std::string_view last_error() const { return last_error_; }

    InsertSubappResult try_insert_subapp(std::string_view name, std::unique_ptr<App>& subapp);
    App* get_subapp(std::string_view name) const;
    std::unique_ptr<App> remove_subapp(std::string_view name);
    SetSubappExtractResult set_subapp_extract(std::string_view name, SubappExtract extract, void* ctx);

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

    PrepareResult post_build();
    RunResult default_runner();
    AddPluginResult drain_pending_plugins();
    AddPluginResult validate_plugin(const Plugin& plugin);
    bool has_pending_plugin(std::string_view name) const;
    bool plugin_dependencies_satisfied(const Plugin& plugin) const;
    bool plugin_dependencies_registered(const Plugin& plugin) const;
    bool pending_plugins_satisfiable() const;
    std::string find_circular_dependency() const;
    void set_error(std::string_view message);
};

} // namespace pulse
