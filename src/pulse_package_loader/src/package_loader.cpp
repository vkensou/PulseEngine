#include "pulse_package_loader.h"
#include "pulse_config.h"
#include "pulse_script_register.h"
#include "pulse_vfs.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#endif

namespace {

using PulsePackageRegisterFn = EPulseResult (*)(PulseAppId, PulseConfig*);

struct ResolvedPackage {
    std::string manifest_name;
    std::string library_path;
    std::string entry_symbol = "pulse_package_register";
    std::string package_dir;
    std::string type = "native";
    std::string script_file;
    bool has_assets = false;
    std::vector<std::string> dependencies;
};

std::unordered_map<PulseAppId, std::vector<void*>>& package_library_handles() {
    static std::unordered_map<PulseAppId, std::vector<void*>> handles;
    return handles;
}

std::unordered_map<PulseAppId, std::unordered_map<std::string, PulseProcPackageRegisterFn>>& static_package_registry() {
    static std::unordered_map<PulseAppId, std::unordered_map<std::string, PulseProcPackageRegisterFn>> registry;
    return registry;
}

std::unordered_map<std::string, const PulseScriptRuntimeDesc*>& script_runtime_registry() {
    static std::unordered_map<std::string, const PulseScriptRuntimeDesc*> registry;
    return registry;
}

std::unordered_map<std::string, const PulseScriptRuntimeDesc*>& script_extension_registry() {
    static std::unordered_map<std::string, const PulseScriptRuntimeDesc*> registry;
    return registry;
}

void close_package_library(void* handle) {
#ifdef _WIN32
    if (handle) FreeLibrary(static_cast<HMODULE>(handle));
#else
    if (handle) dlclose(handle);
#endif
}

bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
#ifdef _WIN32
    for (size_t i = 0; i < a.size(); ++i) {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb - 'A' + 'a');
        if (ca != cb) return false;
    }
    return true;
#else
    return a == b;
#endif
}

std::string join_path(const std::string& a, const std::string& b) {
    if (a.empty()) return b;
    char last = a.back();
    return (last == '/' || last == '\\') ? a + b : a + "/" + b;
}

bool file_exists(const std::string& path) {
#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path.c_str());
    return attr != INVALID_FILE_ATTRIBUTES && !(attr & FILE_ATTRIBUTE_DIRECTORY);
#else
    struct stat st;
    return ::stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
#endif
}

std::string module_directory() {
#ifdef _WIN32
    HMODULE mod = nullptr;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&module_directory),
            &mod))
        return {};

    char path[MAX_PATH];
    DWORD len = GetModuleFileNameA(mod, path, MAX_PATH);
    if (len == 0 || len >= MAX_PATH) return {};

    std::string full(path, len);
    auto slash = full.find_last_of("/\\");
    return slash == std::string::npos ? std::string{} : full.substr(0, slash);
#else
    Dl_info info;
    if (dladdr(reinterpret_cast<void*>(&module_directory), &info) && info.dli_fname) {
        std::string full(info.dli_fname);
        auto slash = full.find_last_of('/');
        return slash == std::string::npos ? std::string{} : full.substr(0, slash);
    }
    return {};
#endif
}

std::vector<std::string> list_subdirectories(const std::string& path) {
    std::vector<std::string> result;
#ifdef _WIN32
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((path + "\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return result;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            std::strcmp(fd.cFileName, ".") != 0 && std::strcmp(fd.cFileName, "..") != 0)
            result.emplace_back(fd.cFileName);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
#else
    DIR* dir = opendir(path.c_str());
    if (!dir) return result;
    while (struct dirent* ent = readdir(dir))
        if (ent->d_type == DT_DIR && std::strcmp(ent->d_name, ".") != 0 && std::strcmp(ent->d_name, "..") != 0)
            result.emplace_back(ent->d_name);
    closedir(dir);
#endif
    return result;
}

std::string find_in_packages_root(const std::string& packages_root, const std::string& package_name) {
    std::string direct = join_path(join_path(packages_root, package_name), "package.json");
    if (file_exists(direct)) return direct;

    for (const auto& dir : list_subdirectories(packages_root)) {
        std::string candidate = join_path(join_path(packages_root, dir), "package.json");
        if (!file_exists(candidate)) continue;

        PulseConfig* cfg = pulse_config_create_from_json_file(candidate.c_str());
        if (!cfg) continue;
        const char* manifest_name = pulse_config_get_string(cfg, "name", nullptr);
        bool match = manifest_name && manifest_name[0] && iequals(manifest_name, package_name);
        pulse_config_release(cfg);
        if (match || iequals(dir, package_name)) return candidate;
    }
    return {};
}

std::string find_package_json(const std::string& package_name,
                              const std::vector<std::string>& search_paths) {
    for (const auto& sp : search_paths) {
        if (sp.empty()) continue;
        std::string found = find_in_packages_root(sp, package_name);
        if (!found.empty()) return found;
    }
    return {};
}

bool resolve_package(const char* entry_name, ResolvedPackage& out, const std::vector<std::string>& search_paths) {
    if (!entry_name || !entry_name[0]) return false;

    std::string manifest_path = find_package_json(entry_name, search_paths);
    if (manifest_path.empty()) return false;

    PulseConfig* cfg = pulse_config_create_from_json_file(manifest_path.c_str());
    if (!cfg) return false;

    auto slash = manifest_path.find_last_of("/\\");
    std::string package_dir = slash == std::string::npos ? std::string() : manifest_path.substr(0, slash);
    out.package_dir = package_dir;

    const char* manifest_name = pulse_config_get_string(cfg, "name", nullptr);
    out.manifest_name = (manifest_name && manifest_name[0]) ? manifest_name : entry_name;

    out.has_assets = pulse_config_get_bool(cfg, "assets", false);

    const char* type = pulse_config_get_string(cfg, "type", nullptr);
    out.type = (type && type[0]) ? type : "native";

    const char* script_file = pulse_config_get_string(cfg, "script_file", nullptr);
    if (script_file && script_file[0]) out.script_file = script_file;

    if (out.type == "native") {
        // Optional DLL entry symbol; fall back to the default register function.
        const char* entry_symbol = pulse_config_get_string(cfg, "entry", nullptr);
        if (entry_symbol && entry_symbol[0]) out.entry_symbol = entry_symbol;

        const char* library = pulse_config_get_string(cfg, "library", nullptr);
        const bool library_absolute = library && library[0] && (library[0] == '/' || library[0] == '\\' || library[1] == ':');
        if (library && library[0]) {
            out.library_path = library_absolute
                                   ? library
                                   : join_path(package_dir, library);
        } else {
            std::string base = package_dir;
            auto last = base.find_last_of("/\\");
            base = (last == std::string::npos) ? base : base.substr(last + 1);
            if (base.empty()) base = entry_name;
#ifdef _WIN32
            out.library_path = join_path(package_dir, base + ".dll");
#elif defined(__APPLE__)
            out.library_path = join_path(package_dir, "lib" + base + ".dylib");
#else
            out.library_path = join_path(package_dir, "lib" + base + ".so");
#endif
        }

        if (!library_absolute && !file_exists(out.library_path)) {
            auto slash = out.library_path.find_last_of("/\\");
            if (slash != std::string::npos) {
                printf("library '%s' not found beside package manifest, falling back to name search\n", out.library_path.c_str());
                out.library_path = out.library_path.substr(slash + 1);
            }
        }
    }
    if (PulseConfigArray* dep_arr = pulse_config_get_array(cfg, "dependencies")) {
        const size_t dep_count = pulse_config_array_count(dep_arr);
        out.dependencies.reserve(dep_count);
        for (size_t i = 0; i < dep_count; ++i) {
            PulseConfig* dep = pulse_config_array_get(dep_arr, i);
            const char* dep_name = dep ? pulse_config_get_string(dep, nullptr, nullptr) : nullptr;
            if (dep_name && dep_name[0]) out.dependencies.emplace_back(dep_name);
        }
    }

    pulse_config_release(cfg);
    return true;
}

#ifdef _WIN32
std::string current_dll_directory() {
    DWORD size = GetDllDirectoryA(0, nullptr);
    if (size == 0) return {};

    std::string dir(size, '\0');
    DWORD copied = GetDllDirectoryA(size, &dir[0]);
    if (copied == 0) return {};

    auto nul = dir.find('\0');
    if (nul != std::string::npos) dir.resize(nul);
    return dir;
}
#endif

void* open_package_library(const char* path) {
#ifdef _WIN32
    std::string previous_dir = current_dll_directory();

    std::string package_path(path ? path : "");
    auto slash = package_path.find_last_of("/\\");
    std::string package_dir = (slash == std::string::npos) ? std::string() : package_path.substr(0, slash);

    if (!package_dir.empty()) {
        char full_path[MAX_PATH];
        DWORD full_len = GetFullPathNameA(package_dir.c_str(), MAX_PATH, full_path, nullptr);
        if (full_len > 0 && full_len < MAX_PATH) package_dir.assign(full_path, full_len);
    }

    SetDllDirectoryA(package_dir.empty() ? nullptr : package_dir.c_str());
    void* handle = static_cast<void*>(LoadLibraryA(path));
    SetDllDirectoryA(previous_dir.empty() ? nullptr : previous_dir.c_str());
    return handle;
#else
    return dlopen(path, RTLD_NOW | RTLD_LOCAL);
#endif
}

void* find_package_register(void* handle, const char* entry_symbol) {
#ifdef _WIN32
    return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(handle), entry_symbol));
#else
    return dlsym(handle, entry_symbol);
#endif
}

std::vector<std::string> parse_extensions(const char* extensions) {
    std::vector<std::string> result;
    if (!extensions || !extensions[0]) return result;
    const char* it = extensions;
    while (*it) {
        while (*it == ',' || *it == ' ' || *it == '.') ++it;
        const char* start = it;
        while (*it && *it != ',') ++it;
        if (it > start) result.emplace_back(start, it);
    }
    return result;
}

void register_script_runtimes(void* lib) {
    void* symbol = find_package_register(lib, PULSE_PACKAGE_GET_RUNTIMES_SYMBOL);
    if (!symbol) return;

    auto get_runtimes = reinterpret_cast<PulseProcPackageGetRuntimesFn>(symbol);
    const PulseScriptRuntimeDesc* runtimes = nullptr;
    const uint32_t count = get_runtimes(&runtimes);
    if (!runtimes) return;

    auto& registry = script_runtime_registry();
    auto& ext_registry = script_extension_registry();
    for (uint32_t i = 0; i < count; ++i) {
        const PulseScriptRuntimeDesc& desc = runtimes[i];
        if (!desc.type || !desc.type[0] || !desc.load || !desc.load_package) continue;

        // type 分发：后注册覆盖（G5.3）。
        auto [it, inserted] = registry.insert_or_assign(desc.type, &desc);
        if (!inserted)
            printf("script runtime '%s' re-registered, overriding the previous one\n", desc.type);

        // 扩展名路由：first-claim-lock（G6.2），防劫持。
        for (const auto& ext : parse_extensions(desc.extensions)) {
            if (ext.empty()) continue;
            if (!ext_registry.emplace(ext, &desc).second)
                printf("warning: extension '%s' already claimed by a runtime, '%s' keeps it\n",
                    ext.c_str(), ext_registry[ext]->type);
            else
                printf("script runtime registered: %s (.%s)\n", desc.type, ext.c_str());
        }
    }
}

bool package_name_loaded(PulseAppId app, const std::string& name,
                         const std::vector<std::string>& loaded,
                         const std::unordered_map<std::string, std::string>& manifest_names) {
    if (pulse_app_has_plugin(app, name.c_str())) return true;
    auto it = manifest_names.find(name);
    if (it != manifest_names.end() && !it->second.empty() && pulse_app_has_plugin(app, it->second.c_str())) return true;
    for (const auto& item : loaded)
        if (iequals(item, name)) return true;
    return false;
}

struct ScriptPackageEntry {
    std::string name;
    std::string package_dir;
    std::string script_file;
    PulseConfig* config = nullptr;
    const PulseScriptRuntimeDesc* runtime = nullptr;
};

struct EnumerateCtx {
    std::unordered_set<std::string> files;
};

std::string normalize_vfs_path(std::string path) {
    while (!path.empty() && (path.front() == '/' || path.front() == '\\'))
        path.erase(path.begin());
    return path;
}

EPulseVfsEnumerateResult collect_files_cb(void* data, const char* orig_dir, const char* fname) {
    auto* ctx = static_cast<EnumerateCtx*>(data);
    if (!fname || !fname[0] || std::strcmp(fname, ".") == 0 || std::strcmp(fname, "..") == 0)
        return PULSE_VFS_ENUMERATE_RESULT_OK;

    std::string full = normalize_vfs_path(join_path(orig_dir ? orig_dir : "", fname));
    if (full.empty()) return PULSE_VFS_ENUMERATE_RESULT_OK;

    PulseVfsStat st{};
    if (!pulse_vfs_stat(full.c_str(), &st))
        return PULSE_VFS_ENUMERATE_RESULT_OK;

    if (st.file_type == PULSE_VFS_FILE_TYPE_DIRECTORY) {
        pulse_vfs_enumerate(full.c_str(), collect_files_cb, ctx);
        return PULSE_VFS_ENUMERATE_RESULT_OK;
    }
    if (st.file_type == PULSE_VFS_FILE_TYPE_REGULAR)
        ctx->files.insert(std::move(full));
    return PULSE_VFS_ENUMERATE_RESULT_OK;
}

std::unordered_set<std::string> collect_union_files() {
    EnumerateCtx ctx;
    pulse_vfs_enumerate("/", collect_files_cb, &ctx);
    return std::move(ctx.files);
}

std::string extension_of(const std::string& path) {
    auto dot = path.find_last_of('.');
    auto slash = path.find_last_of("/\\");
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash))
        return {};
    return path.substr(dot + 1);
}

bool read_vfs_file(const std::string& path, std::string& out) {
    PulseVfsFileId file = pulse_vfs_open_read(path.c_str());
    if (!file) return false;
    int64_t length = pulse_vfs_file_length(file);
    if (length < 0) {
        pulse_vfs_close(file);
        return false;
    }
    out.resize(static_cast<size_t>(length));
    if (length > 0) {
        int64_t got = pulse_vfs_read_bytes(file, out.data(), static_cast<uint64_t>(length));
        if (got != length) {
            pulse_vfs_close(file);
            return false;
        }
    }
    pulse_vfs_close(file);
    return true;
}

EPulsePackageLoadResult inject_script_snapshot(PulseAppId app, const std::vector<ScriptPackageEntry>& script_packages, const std::vector<std::string>& script_only_mounts) {
    if (script_packages.empty()) return PULSE_PACKAGE_LOAD_RESULT_OK;

    std::unordered_set<std::string> files = collect_union_files();
    const auto& ext_registry = script_extension_registry();
    for (const std::string& path : files) {
        std::string ext = extension_of(path);
        if (ext.empty()) continue;
        auto rt = ext_registry.find(ext);
        if (rt == ext_registry.end()) continue;

        std::string text;
        if (!read_vfs_file(path, text)) {
            fprintf(stderr, "package loader: failed to read script '%s' via vfs\n", path.c_str());
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED;
        }
        EPulseResult r = rt->second->load(app, path.c_str(), text.data(), static_cast<uint64_t>(text.size()));
        if (r != PULSE_RESULT_OK) {
            fprintf(stderr, "package loader: script runtime failed to inject '%s'\n", path.c_str());
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED;
        }
    }

    for (const auto& pkg : script_packages) {
        if (pkg.runtime->load_package(app, pkg.script_file.c_str()) != PULSE_RESULT_OK) {
            fprintf(stderr, "package loader: script runtime failed to register entry for package '%s' ('%s')\n",
                pkg.name.c_str(), pkg.script_file.c_str());
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED;
        }
    }

    for (const std::string& dir : script_only_mounts)
        pulse_vfs_unmount(dir.c_str());

    return PULSE_PACKAGE_LOAD_RESULT_OK;
}

EPulsePackageLoadResult load_packages_impl(PulseAppId app, uint32_t search_path_count, const char** search_paths, const PulsePackageListEntry* entries, uint32_t count) {
    if (!app || (!entries && count != 0)) return PULSE_PACKAGE_LOAD_RESULT_ERROR_INVALID_ARGUMENT;
    if (search_paths == nullptr && search_path_count != 0) return PULSE_PACKAGE_LOAD_RESULT_ERROR_INVALID_ARGUMENT;
    if (count == 0) return PULSE_PACKAGE_LOAD_RESULT_OK;

    struct PendingEntry {
        const PulsePackageListEntry* entry;
        ResolvedPackage resolved;
    };

    std::vector<std::string> search_path_roots;
    search_path_roots.reserve(search_path_count);
    for (uint32_t i = 0; i < search_path_count; ++i)
        if (search_paths[i]) search_path_roots.emplace_back(search_paths[i]);

    auto& static_packages = static_package_registry()[app];
    std::vector<PendingEntry> pending;
    pending.reserve(count);
    for (uint32_t i = 0; i < count; ++i) {
        if (!entries[i].name || !entries[i].name[0]) return PULSE_PACKAGE_LOAD_RESULT_ERROR_INVALID_ARGUMENT;

        PendingEntry pe{&entries[i], {}};
        if (!resolve_package(entries[i].name, pe.resolved, search_path_roots)) {
            // No manifest found: allowed only for static packages.
            if (static_packages.find(entries[i].name) == static_packages.end()) {
                fprintf(stderr, "package '%s': no manifest found in any search path (and not registered as a static package)\n",
                        entries[i].name);
                return PULSE_PACKAGE_LOAD_RESULT_ERROR_LIBRARY_NOT_FOUND;
            }
        }
        if (pe.resolved.type != "native" && pe.resolved.script_file.empty()) {
            fprintf(stderr, "package '%s': type '%s' requires 'script_file' in package.json\n",
                    entries[i].name, pe.resolved.type.c_str());
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_INVALID_ARGUMENT;
        }
        if (pulse_app_has_plugin(app, entries[i].name) ||
            (!pe.resolved.manifest_name.empty() && pulse_app_has_plugin(app, pe.resolved.manifest_name.c_str())))
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_DUPLICATE_PACKAGE;
        pending.push_back(std::move(pe));
    }

    std::vector<bool> loaded_flag(count, false);
    std::vector<size_t> order;
    std::vector<std::string> loaded;
    std::unordered_map<std::string, std::string> manifest_names;
    for (const auto& pe : pending)
        if (!pe.resolved.manifest_name.empty())
            manifest_names.emplace(pe.entry->name, pe.resolved.manifest_name);

    size_t remaining = count;
    while (remaining) {
        bool progress = false;
        for (size_t i = 0; i < count; ++i) {
            if (loaded_flag[i]) continue;
            bool ready = true;
            for (const auto& dep : pending[i].resolved.dependencies)
                if (!package_name_loaded(app, dep, loaded, manifest_names)) {
                    ready = false;
                    break;
                }
            if (!ready) continue;

            loaded_flag[i] = true;
            order.push_back(i);
            loaded.emplace_back(pending[i].entry->name);
            if (!pending[i].resolved.manifest_name.empty() && pending[i].resolved.manifest_name != pending[i].entry->name)
                loaded.emplace_back(pending[i].resolved.manifest_name);
            --remaining;
            progress = true;
        }
        if (!progress) break;
    }

    if (remaining) {
        for (size_t i = 0; i < count; ++i) {
            if (loaded_flag[i]) continue;
            for (const auto& dep : pending[i].resolved.dependencies) {
                if (package_name_loaded(app, dep, loaded, manifest_names)) continue;
                bool in_pending = false;
                for (size_t j = 0; j < count; ++j)
                    if (!loaded_flag[j] && iequals(pending[j].entry->name, dep)) {
                        in_pending = true;
                        break;
                    }
                if (!in_pending) return PULSE_PACKAGE_LOAD_RESULT_ERROR_MISSING_DEPENDENCY;
            }
        }
        return PULSE_PACKAGE_LOAD_RESULT_ERROR_CIRCULAR_DEPENDENCY;
    }

    auto& handles = package_library_handles();
    std::vector<ScriptPackageEntry> script_packages;
    std::vector<std::string> script_only_mounts;
    for (size_t idx : order) {
        const auto& pe = pending[idx];

        if ((pe.resolved.has_assets || pe.resolved.type != "native") && !pe.resolved.package_dir.empty()) {
            pulse_vfs_mount(pe.resolved.package_dir.c_str(), "/", false);
        }

        if (pe.resolved.type != "native") {
            auto& registry = script_runtime_registry();
            auto runtime = registry.find(pe.resolved.type);
            if (runtime == registry.end()) {
                fprintf(stderr, "package '%s': no script runtime registered for type '%s'\n",
                        pe.entry->name, pe.resolved.type.c_str());
                return PULSE_PACKAGE_LOAD_RESULT_ERROR_UNKNOWN_RUNTIME;
            }

            ScriptPackageEntry entry = {
                pe.entry->name ? pe.entry->name : "",
                pe.resolved.package_dir,
                pe.resolved.script_file,
                pe.entry->config,
                runtime->second,
            };
            script_packages.push_back(std::move(entry));
            if (!pe.resolved.has_assets && !pe.resolved.package_dir.empty())
                script_only_mounts.push_back(pe.resolved.package_dir);
            continue;
        }

        PulsePackageRegisterFn fn = nullptr;
        void* lib = nullptr;

        if (!pe.resolved.library_path.empty()) {
            lib = open_package_library(pe.resolved.library_path.c_str());
            if (!lib) {
#ifdef _WIN32
                printf("Load library %s failed: %d\n", pe.resolved.library_path.c_str(), GetLastError());
#endif
                return PULSE_PACKAGE_LOAD_RESULT_ERROR_LIBRARY_NOT_FOUND;
            }
            void* symbol = find_package_register(lib, pe.resolved.entry_symbol.c_str());
            if (!symbol) {
                close_package_library(lib);
                return PULSE_PACKAGE_LOAD_RESULT_ERROR_ENTRY_NOT_FOUND;
            }
            fn = reinterpret_cast<PulsePackageRegisterFn>(symbol);
        } else {
            auto it = static_packages.find(pe.entry->name ? pe.entry->name : "");
            if (it == static_packages.end()) return PULSE_PACKAGE_LOAD_RESULT_ERROR_LIBRARY_NOT_FOUND;
            fn = reinterpret_cast<PulsePackageRegisterFn>(it->second);
        }

        if (fn(app, pe.entry->config) != PULSE_RESULT_OK) {
            if (lib) close_package_library(lib);
            return PULSE_PACKAGE_LOAD_RESULT_ERROR_REGISTER_FAILED;
        }
        if (lib) {
            register_script_runtimes(lib);
            handles[app].push_back(lib);
        }
    }

    EPulsePackageLoadResult inject_result = inject_script_snapshot(app, script_packages, script_only_mounts);
    if (inject_result != PULSE_PACKAGE_LOAD_RESULT_OK)
        return inject_result;

    return PULSE_PACKAGE_LOAD_RESULT_OK;
}

} // namespace

extern "C" {

PULSE_PACKAGE_LOADER_API EPulsePackageLoadResult pulse_package_loader_load_packages(
    PulseAppId app, uint32_t search_path_count, const char** search_paths,
    uint32_t count, const PulsePackageListEntry* entries) {
    return load_packages_impl(app, search_path_count, search_paths, entries, count);
}

PULSE_PACKAGE_LOADER_API void pulse_package_loader_register_static_package(
    PulseAppId app, const char* name, PulseProcPackageRegisterFn register_fn) {
    if (app && name && name[0] && register_fn) static_package_registry()[app][name] = register_fn;
}

PULSE_PACKAGE_LOADER_API void pulse_package_loader_cleanup(PulseAppId app) {
    if (!app) return;
    auto& handles = package_library_handles();
    auto it = handles.find(app);
    if (it != handles.end()) {
        for (void* handle : it->second) close_package_library(handle);
        handles.erase(it);
    }
    auto& static_packages = static_package_registry();
    auto sm_it = static_packages.find(app);
    if (sm_it != static_packages.end()) static_packages.erase(sm_it);

    script_runtime_registry().clear();
    script_extension_registry().clear();
}

} // extern "C"
