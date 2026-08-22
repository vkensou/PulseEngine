#include <flecs.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "pulse_app.h"
#include "app_internal.h"

import pulse_app;

ECS_COMPONENT_DECLARE(PulseTimer);
ECS_COMPONENT_DECLARE(pulse_app_state_resource);

namespace {

pulse::App* to_app(PulseAppId handle) {
    return reinterpret_cast<pulse::App*>(handle);
}

PulseAppId to_handle(pulse::App* app) {
    return reinterpret_cast<PulseAppId>(app);
}

pulse::RunnerResult to_runner_result(EPulseRunnerResult result) {
    switch (result) {
        case PULSE_RUNNER_RESULT_OK: return pulse::RunnerResult::Ok;
        case PULSE_RUNNER_RESULT_ERROR_INVALID_ARGUMENT: return pulse::RunnerResult::InvalidArgument;
        case PULSE_RUNNER_RESULT_ERROR_INVALID_STATE: return pulse::RunnerResult::InvalidState;
        case PULSE_RUNNER_RESULT_ERROR_SUBAPP_EXTRACT_FAILED: return pulse::RunnerResult::SubappExtractFailed;
        case PULSE_RUNNER_RESULT_ERROR_SUBAPP_UPDATE_FAILED: return pulse::RunnerResult::SubappUpdateFailed;
        case PULSE_RUNNER_RESULT_ERROR_INTERNAL: return pulse::RunnerResult::Internal;
        default: return pulse::RunnerResult::Internal;
    }
}

pulse::SubappExtractResult to_subapp_extract_result(EPulseSubappExtractResult result) {
    switch (result) {
        case PULSE_SUBAPP_EXTRACT_RESULT_OK: return pulse::SubappExtractResult::Ok;
        case PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_ARGUMENT: return pulse::SubappExtractResult::InvalidArgument;
        case PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_STATE: return pulse::SubappExtractResult::InvalidState;
        case PULSE_SUBAPP_EXTRACT_RESULT_ERROR_INTERNAL: return pulse::SubappExtractResult::Internal;
        default: return pulse::SubappExtractResult::Internal;
    }
}

pulse::PluginBuildResult to_plugin_build_result(EPulsePluginBuildResult result) {
    switch (result) {
        case PULSE_PLUGIN_BUILD_RESULT_OK: return pulse::PluginBuildResult::Ok;
        case PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_ARGUMENT: return pulse::PluginBuildResult::InvalidArgument;
        case PULSE_PLUGIN_BUILD_RESULT_ERROR_INVALID_STATE: return pulse::PluginBuildResult::InvalidState;
        case PULSE_PLUGIN_BUILD_RESULT_ERROR_DUPLICATE_PLUGIN: return pulse::PluginBuildResult::DuplicatePlugin;
        case PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL: return pulse::PluginBuildResult::Internal;
        default: return pulse::PluginBuildResult::Internal;
    }
}

EPulseAppRunResult to_run_c(pulse::RunResult result) {
    switch (result) {
        case pulse::RunResult::Ok: return PULSE_APP_RUN_RESULT_OK;
        case pulse::RunResult::InvalidArgument: return PULSE_APP_RUN_RESULT_ERROR_INVALID_ARGUMENT;
        case pulse::RunResult::InvalidState: return PULSE_APP_RUN_RESULT_ERROR_INVALID_STATE;
        case pulse::RunResult::PluginBuildFailed: return PULSE_APP_RUN_RESULT_ERROR_PLUGIN_BUILD_FAILED;
        case pulse::RunResult::MissingPluginDependency: return PULSE_APP_RUN_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY;
        case pulse::RunResult::CircularPluginDependency: return PULSE_APP_RUN_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY;
        case pulse::RunResult::PluginPostBuildFailed: return PULSE_APP_RUN_RESULT_ERROR_PLUGIN_POST_BUILD_FAILED;
        case pulse::RunResult::SubappPostBuildFailed: return PULSE_APP_RUN_RESULT_ERROR_SUBAPP_POST_BUILD_FAILED;
        case pulse::RunResult::SubappPrepareFailed: return PULSE_APP_RUN_RESULT_ERROR_SUBAPP_PREPARE_FAILED;
        case pulse::RunResult::SubappExtractFailed: return PULSE_APP_RUN_RESULT_ERROR_SUBAPP_EXTRACT_FAILED;
        case pulse::RunResult::SubappUpdateFailed: return PULSE_APP_RUN_RESULT_ERROR_SUBAPP_UPDATE_FAILED;
        default: return PULSE_APP_RUN_RESULT_ERROR_INTERNAL;
    }
}

EPulseAppPrepareResult to_prepare_c(pulse::PrepareResult result) {
    switch (result) {
        case pulse::PrepareResult::Ok: return PULSE_APP_PREPARE_RESULT_OK;
        case pulse::PrepareResult::InvalidArgument: return PULSE_APP_PREPARE_RESULT_ERROR_INVALID_ARGUMENT;
        case pulse::PrepareResult::InvalidState: return PULSE_APP_PREPARE_RESULT_ERROR_INVALID_STATE;
        case pulse::PrepareResult::PluginBuildFailed: return PULSE_APP_PREPARE_RESULT_ERROR_PLUGIN_BUILD_FAILED;
        case pulse::PrepareResult::MissingPluginDependency: return PULSE_APP_PREPARE_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY;
        case pulse::PrepareResult::CircularPluginDependency: return PULSE_APP_PREPARE_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY;
        case pulse::PrepareResult::PluginPostBuildFailed: return PULSE_APP_PREPARE_RESULT_ERROR_PLUGIN_POST_BUILD_FAILED;
        case pulse::PrepareResult::SubappPostBuildFailed: return PULSE_APP_PREPARE_RESULT_ERROR_SUBAPP_POST_BUILD_FAILED;
        case pulse::PrepareResult::SubappPrepareFailed: return PULSE_APP_PREPARE_RESULT_ERROR_SUBAPP_PREPARE_FAILED;
        default: return PULSE_APP_PREPARE_RESULT_ERROR_INTERNAL;
    }
}

EPulseAppUpdateResult to_update_c(pulse::UpdateResult result) {
    switch (result) {
        case pulse::UpdateResult::Ok: return PULSE_APP_UPDATE_RESULT_OK;
        case pulse::UpdateResult::InvalidArgument: return PULSE_APP_UPDATE_RESULT_ERROR_INVALID_ARGUMENT;
        case pulse::UpdateResult::InvalidState: return PULSE_APP_UPDATE_RESULT_ERROR_INVALID_STATE;
        case pulse::UpdateResult::SubappExtractFailed: return PULSE_APP_UPDATE_RESULT_ERROR_SUBAPP_EXTRACT_FAILED;
        case pulse::UpdateResult::SubappUpdateFailed: return PULSE_APP_UPDATE_RESULT_ERROR_SUBAPP_UPDATE_FAILED;
        default: return PULSE_APP_UPDATE_RESULT_ERROR_INTERNAL;
    }
}

EPulseAppSetRunnerResult to_set_runner_c(pulse::SetRunnerResult result) {
    switch (result) {
        case pulse::SetRunnerResult::Ok: return PULSE_APP_SET_RUNNER_RESULT_OK;
        case pulse::SetRunnerResult::InvalidArgument: return PULSE_APP_SET_RUNNER_RESULT_ERROR_INVALID_ARGUMENT;
        case pulse::SetRunnerResult::InvalidState: return PULSE_APP_SET_RUNNER_RESULT_ERROR_INVALID_STATE;
        default: return PULSE_APP_SET_RUNNER_RESULT_ERROR_INTERNAL;
    }
}

EPulseAppAddPluginResult to_add_plugin_c(pulse::AddPluginResult result) {
    switch (result) {
        case pulse::AddPluginResult::Ok: return PULSE_APP_ADD_PLUGIN_RESULT_OK;
        case pulse::AddPluginResult::InvalidArgument: return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
        case pulse::AddPluginResult::InvalidState: return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_STATE;
        case pulse::AddPluginResult::DuplicatePlugin: return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
        case pulse::AddPluginResult::PluginBuildFailed: return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_PLUGIN_BUILD_FAILED;
        case pulse::AddPluginResult::MissingPluginDependency: return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_MISSING_PLUGIN_DEPENDENCY;
        case pulse::AddPluginResult::CircularPluginDependency: return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_CIRCULAR_PLUGIN_DEPENDENCY;
        default: return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INTERNAL;
    }
}

EPulseAppInsertSubappResult to_insert_subapp_c(pulse::InsertSubappResult result) {
    switch (result) {
        case pulse::InsertSubappResult::Ok: return PULSE_APP_INSERT_SUBAPP_RESULT_OK;
        case pulse::InsertSubappResult::InvalidArgument: return PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INVALID_ARGUMENT;
        case pulse::InsertSubappResult::InvalidState: return PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INVALID_STATE;
        case pulse::InsertSubappResult::DuplicateSubapp: return PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_DUPLICATE_SUBAPP;
        default: return PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INTERNAL;
    }
}

EPulseAppSetSubappExtractResult to_set_subapp_extract_c(pulse::SetSubappExtractResult result) {
    switch (result) {
        case pulse::SetSubappExtractResult::Ok: return PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_OK;
        case pulse::SetSubappExtractResult::InvalidArgument: return PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_ARGUMENT;
        case pulse::SetSubappExtractResult::InvalidState: return PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_STATE;
        case pulse::SetSubappExtractResult::NotFound: return PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_NOT_FOUND;
        default: return PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INTERNAL;
    }
}


using PulseModuleRegisterFn = EPulseResult (*)(PulseAppId, const void*, uint32_t);

std::unordered_map<PulseAppId, std::vector<void*>>& module_library_handles() {
    static std::unordered_map<PulseAppId, std::vector<void*>> handles;
    return handles;
}
std::unordered_map<PulseAppId, std::unordered_map<std::string, PulseProcModuleRegisterFn>>& static_module_registry() {
    static std::unordered_map<PulseAppId, std::unordered_map<std::string, PulseProcModuleRegisterFn>> registry;
    return registry;
}


void close_module_library(void* handle) {
#ifdef _WIN32
    if (handle) FreeLibrary(static_cast<HMODULE>(handle));
#else
    if (handle) dlclose(handle);
#endif
}

void* open_module_library(const char* path) {
#ifdef _WIN32
    return static_cast<void*>(LoadLibraryA(path));
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void* find_module_register(void* handle) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), "pulse_module_register"));
#else
    return dlsym(handle, "pulse_module_register");
#endif
}

bool module_name_loaded(PulseAppId app, const char* name, const std::vector<std::string>& loaded) {
    if (pulse_app_has_plugin(app, name)) {
        return true;
    }
    for (const auto& item : loaded) {
        if (item == name) {
            return true;
        }
    }
    return false;
}

EPulseModuleLoadResult load_modules_impl(PulseAppId app, const PulseModuleListEntry* entries, uint32_t count) {
    if (!app || (!entries && count != 0)) {
        return PULSE_MODULE_LOAD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (count == 0) {
        return PULSE_MODULE_LOAD_RESULT_OK;
    }

    std::vector<const PulseModuleListEntry*> remaining;
    remaining.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (!entries[i].name || !entries[i].name[0]) {
            return PULSE_MODULE_LOAD_RESULT_ERROR_INVALID_ARGUMENT;
        }
        if (pulse_app_has_plugin(app, entries[i].name)) {
            return PULSE_MODULE_LOAD_RESULT_ERROR_DUPLICATE_MODULE;
        }
        remaining.push_back(&entries[i]);
    }

    std::vector<const PulseModuleListEntry*> order;
    std::vector<std::string> loaded;
    bool progress = true;
    while (!remaining.empty() && progress) {
        progress = false;
        for (auto it = remaining.begin(); it != remaining.end(); ) {
            bool ready = true;
            for (uint32_t d = 0; d < (*it)->dependency_count; ++d) {
                const char* dep = (*it)->dependencies ? (*it)->dependencies[d] : nullptr;
                if (!dep || !module_name_loaded(app, dep, loaded)) {
                    ready = false;
                    break;
                }
            }
            if (!ready) {
                ++it;
                continue;
            }
            order.push_back(*it);
            loaded.emplace_back((*it)->name);
            it = remaining.erase(it);
            progress = true;
        }
    }

    if (!remaining.empty()) {
        for (const auto* entry : remaining) {
            for (uint32_t d = 0; d < entry->dependency_count; ++d) {
                const char* dep = entry->dependencies ? entry->dependencies[d] : nullptr;
                if (!dep || !module_name_loaded(app, dep, loaded)) {
                    return PULSE_MODULE_LOAD_RESULT_ERROR_MISSING_DEPENDENCY;
                }
            }
        }
        return PULSE_MODULE_LOAD_RESULT_ERROR_CIRCULAR_DEPENDENCY;
    }

    auto& handles = module_library_handles();
    auto& static_modules = static_module_registry()[app];
    for (const auto* entry : order) {
        PulseModuleRegisterFn fn = nullptr;
        void* lib = nullptr;

        if (entry->library && entry->library[0]) {
            lib = open_module_library(entry->library);
            if (!lib) {
                return PULSE_MODULE_LOAD_RESULT_ERROR_LIBRARY_NOT_FOUND;
            }
            void* symbol = find_module_register(lib);
            if (!symbol) {
                close_module_library(lib);
                return PULSE_MODULE_LOAD_RESULT_ERROR_ENTRY_NOT_FOUND;
            }
            fn = reinterpret_cast<PulseModuleRegisterFn>(symbol);
        } else {
            auto it = static_modules.find(entry->name ? entry->name : "");
            if (it == static_modules.end()) {
                return PULSE_MODULE_LOAD_RESULT_ERROR_ENTRY_NOT_FOUND;
            }
            fn = reinterpret_cast<PulseModuleRegisterFn>(it->second);
        }

        EPulseResult result = fn(app, entry->config, entry->config_size);
        if (result != PULSE_RESULT_OK) {
            if (lib) {
                close_module_library(lib);
            }
            return PULSE_MODULE_LOAD_RESULT_ERROR_REGISTER_FAILED;
        }
        if (lib) {
            handles[app].push_back(lib);
        }
    }

    return PULSE_MODULE_LOAD_RESULT_OK;
}

pulse::AppDesc to_desc(const PulseAppDesc* desc) {
    pulse::AppDesc out;
    out.name = desc->name ? desc->name : "";
    out.enable_rest_api = desc->enable_restapi;
    return out;
}

} // namespace

extern "C" {

PulseAppId pulse_create_app(const PulseAppDesc* desc) {
    if (!desc) {
        return nullptr;
    }
    return to_handle(new pulse::App(to_desc(desc)));
}

void pulse_destroy_app(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return;
    }
    delete impl;

    auto& handles = module_library_handles();
    auto it = handles.find(app);
    if (it != handles.end()) {
        for (void* handle : it->second) {
            close_module_library(handle);
        }
        handles.erase(it);
    }

    auto& static_modules = static_module_registry();
    auto sm_it = static_modules.find(app);
    if (sm_it != static_modules.end()) {
        static_modules.erase(sm_it);
    }
}

EPulseAppRunResult pulse_app_run(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_APP_RUN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return to_run_c(impl->run());
}

EPulseAppPrepareResult pulse_app_prepare(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_APP_PREPARE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return to_prepare_c(impl->prepare());
}

EPulseAppUpdateResult pulse_app_update(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_APP_UPDATE_RESULT_ERROR_INVALID_ARGUMENT;
    }
    return to_update_c(impl->update());
}

void pulse_app_teardown(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return;
    }
    impl->teardown();
}

void pulse_app_finish(PulseAppId app) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return;
    }
    impl->finish();
}

bool pulse_app_should_quit(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->should_quit() : true;
}

EPulseAppSetRunnerResult pulse_app_set_runner(PulseAppId app, PulseProcRunnerFn runner, void* ctx) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_APP_SET_RUNNER_RESULT_ERROR_INVALID_ARGUMENT;
    }

    pulse::Runner wrapped;
    if (runner) {
        wrapped = [app, runner](pulse::App&, void* c) {
            return to_runner_result(runner(app, c));
        };
    }
    return to_set_runner_c(impl->set_runner(std::move(wrapped), ctx));
}

EPulseAppAddPluginResult pulse_app_add_plugin(PulseAppId app, const PulsePluginDesc* desc) {
    pulse::App* impl = to_app(app);
    if (!impl || !desc) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    if (desc->struct_size != sizeof(PulsePluginDesc)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (desc->version != PULSE_PLUGIN_DESC_VERSION) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    PulseProcPluginBuildFn build = desc->build;
    PulseProcPluginBuildFn post_build = desc->post_build;
    PulseProcPluginShutdownFn shutdown = desc->shutdown;

    if (desc->dependency_count > 0 && !desc->dependencies) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }

    pulse::Plugin plugin;
    plugin.name = desc->name ? desc->name : "";
    plugin.plugin_version = desc->plugin_version;
    plugin.ctx = desc->ctx;
    plugin.dependencies.reserve(desc->dependency_count);
    for (uint32_t i = 0; i < desc->dependency_count; ++i) {
        if (!desc->dependencies[i]) {
            return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
        }
        plugin.dependencies.emplace_back(desc->dependencies[i]);
    }
    if (build) {
        plugin.build = [app, build](pulse::App&, void* c) {
            return to_plugin_build_result(build(app, c));
        };
    }
    if (post_build) {
        plugin.post_build = [app, post_build](pulse::App&, void* c) {
            return to_plugin_build_result(post_build(app, c));
        };
    }
    if (shutdown) {
        plugin.shutdown = [app, shutdown](pulse::App&, void* c) {
            shutdown(app, c);
        };
    }

    return to_add_plugin_c(impl->add_plugin(std::move(plugin)));
}

EPulseModuleLoadResult pulse_app_load_modules(PulseAppId _this, const PulseModuleListEntry* entries, uint32_t count) {
    return load_modules_impl(_this, entries, count);
}

void pulse_app_register_static_module(PulseAppId _this, const char* name, PulseProcModuleRegisterFn register_fn) {
    if (!_this || !name || !name[0] || !register_fn) {
        return;
    }
    static_module_registry()[_this][name] = register_fn;
}

bool pulse_app_has_plugin(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    return impl ? impl->has_plugin(name ? name : "") : false;
}

ecs_world_t* pulse_app_world(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->world() : nullptr;
}

const char* pulse_app_last_error(PulseAppId app) {
    pulse::App* impl = to_app(app);
    return impl ? impl->last_error().data() : "invalid app";
}

EPulseAppInsertSubappResult pulse_app_insert_subapp(PulseAppId app, const char* name, PulseAppId subapp) {
    if (app == subapp) {
        return PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INVALID_ARGUMENT;
    }
    pulse::App* impl = to_app(app);
    if (!impl || !subapp) {
        return PULSE_APP_INSERT_SUBAPP_RESULT_ERROR_INVALID_ARGUMENT;
    }

    auto subapp_cpp = std::unique_ptr<pulse::App>(reinterpret_cast<pulse::App*>(subapp));
    auto result = impl->try_insert_subapp(name ? name : "", subapp_cpp);
    if (result != pulse::InsertSubappResult::Ok) subapp_cpp.release();
    return to_insert_subapp_c(result);
}

PulseAppId pulse_app_get_subapp(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return nullptr;
    }
    return to_handle(impl->get_subapp(name ? name : ""));
}

PulseAppId pulse_app_remove_subapp(PulseAppId app, const char* name) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return nullptr;
    }

    std::unique_ptr<pulse::App> removed = impl->remove_subapp(name ? name : "");
    return to_handle(removed.release());
}

EPulseAppSetSubappExtractResult pulse_app_set_subapp_extract(
    PulseAppId app,
    const char* name,
    PulseProcSubappExtractFn extract,
    void* ctx
) {
    pulse::App* impl = to_app(app);
    if (!impl) {
        return PULSE_APP_SET_SUBAPP_EXTRACT_RESULT_ERROR_INVALID_ARGUMENT;
    }

    pulse::SubappExtract wrapped;
    if (extract) {
        wrapped = [app, extract](pulse::App&, pulse::App& subapp, void* c) {
            return to_subapp_extract_result(extract(app, to_handle(&subapp), c));
        };
    }

    return to_set_subapp_extract_c(impl->set_subapp_extract(name ? name : "", std::move(wrapped), ctx));
}

PulseAppId pulse_get_app_from_world(ecs_world_t* world) {
    if (!world) {
        return nullptr;
    }

    const pulse_app_state_resource* resource = ecs_singleton_get(world, pulse_app_state_resource);
    return resource ? to_handle(reinterpret_cast<pulse::App*>(resource->app)) : nullptr;
}

} // extern "C"
