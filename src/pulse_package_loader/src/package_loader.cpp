#include "pulse_package_loader.h"

#include <string>
#include <unordered_map>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace {

using PulsePackageRegisterFn = EPulseResult (*)(PulseAppId, PulseConfig*);

std::unordered_map<PulseAppId, std::vector<void*>>& package_library_handles() {
    static std::unordered_map<PulseAppId, std::vector<void*>> handles;
    return handles;
}

std::unordered_map<PulseAppId, std::unordered_map<std::string, PulseProcPackageRegisterFn>>& static_package_registry() {
    static std::unordered_map<PulseAppId, std::unordered_map<std::string, PulseProcPackageRegisterFn>> registry;
    return registry;
}

void close_package_library(void* handle) {
#ifdef _WIN32
    if (handle) FreeLibrary(static_cast<HMODULE>(handle));
#else
    if (handle) dlclose(handle);
#endif
}

void* open_package_library(const char* path) {
#ifdef _WIN32
    return static_cast<void*>(LoadLibraryA(path));
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void* find_package_register(void* handle) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), "pulse_package_register"));
#else
    return dlsym(handle, "pulse_package_register");
#endif
}

bool package_name_loaded(PulseAppId app, const char* name, const std::vector<std::string>& loaded) {
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

EPulsePackageLoadResult load_packages_impl(PulseAppId app, const PulsePackageListEntry* entries, uint32_t count) {
    if (!app || (!entries && count != 0)) {
        return PULSE_PACKAGE_LOAD_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (count == 0) {
        return PULSE_PACKAGE_LOAD_RESULT_OK;
    }

    std::vector<const PulsePackageListEntry*> remaining;
    remaining.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (!entries[i].name || !entries[i].name[0]) {
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_INVALID_ARGUMENT;
        }
        if (pulse_app_has_plugin(app, entries[i].name)) {
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_DUPLICATE_PACKAGE;
        }
        remaining.push_back(&entries[i]);
    }

    std::vector<const PulsePackageListEntry*> order;
    std::vector<std::string> loaded;
    bool progress = true;
    while (!remaining.empty() && progress) {
        progress = false;
        for (auto it = remaining.begin(); it != remaining.end(); ) {
            bool ready = true;
            for (uint32_t d = 0; d < (*it)->dependency_count; ++d) {
                const char* dep = (*it)->dependencies ? (*it)->dependencies[d] : nullptr;
                if (!dep || !package_name_loaded(app, dep, loaded)) {
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
                if (!dep || !package_name_loaded(app, dep, loaded)) {
                    return PULSE_PACKAGE_LOAD_RESULT_ERROR_MISSING_DEPENDENCY;
                }
            }
        }
        return PULSE_PACKAGE_LOAD_RESULT_ERROR_CIRCULAR_DEPENDENCY;
    }

    auto& handles = package_library_handles();
    auto& static_packages = static_package_registry()[app];
    for (const auto* entry : order) {
        PulsePackageRegisterFn fn = nullptr;
        void* lib = nullptr;

        if (entry->library && entry->library[0]) {
            lib = open_package_library(entry->library);
            if (!lib) {
                return PULSE_PACKAGE_LOAD_RESULT_ERROR_LIBRARY_NOT_FOUND;
            }
            void* symbol = find_package_register(lib);
            if (!symbol) {
                close_package_library(lib);
                return PULSE_PACKAGE_LOAD_RESULT_ERROR_ENTRY_NOT_FOUND;
            }
            fn = reinterpret_cast<PulsePackageRegisterFn>(symbol);
        } else {
            auto it = static_packages.find(entry->name ? entry->name : "");
            if (it == static_packages.end()) {
                return PULSE_PACKAGE_LOAD_RESULT_ERROR_ENTRY_NOT_FOUND;
            }
            fn = reinterpret_cast<PulsePackageRegisterFn>(it->second);
        }

        EPulseResult result = fn(app, entry->config);
        if (result != PULSE_RESULT_OK) {
            if (lib) {
                close_package_library(lib);
            }
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED;
        }
        if (lib) {
            handles[app].push_back(lib);
        }
    }

    return PULSE_PACKAGE_LOAD_RESULT_OK;
}

} // namespace

extern "C" {

PULSE_PACKAGE_LOADER_API EPulsePackageLoadResult pulse_package_loader_load_packages(
    PulseAppId app,
    const PulsePackageListEntry* entries,
    uint32_t count) {
    return load_packages_impl(app, entries, count);
}

PULSE_PACKAGE_LOADER_API void pulse_package_loader_register_static_package(
    PulseAppId app,
    const char* name,
    PulseProcPackageRegisterFn register_fn) {
    if (!app || !name || !name[0] || !register_fn) {
        return;
    }
    static_package_registry()[app][name] = register_fn;
}

PULSE_PACKAGE_LOADER_API void pulse_package_loader_cleanup(PulseAppId app) {
    if (!app) {
        return;
    }

    auto& handles = package_library_handles();
    auto it = handles.find(app);
    if (it != handles.end()) {
        for (void* handle : it->second) {
            close_package_library(handle);
        }
        handles.erase(it);
    }

    auto& static_packages = static_package_registry();
    auto sm_it = static_packages.find(app);
    if (sm_it != static_packages.end()) {
        static_packages.erase(sm_it);
    }
}

} // extern "C"