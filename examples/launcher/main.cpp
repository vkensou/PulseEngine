// ============================================================
// PulseEngine Launcher (plugin package loader)
//
// 这个 launcher 不链接任何具体的 pulse_* 插件，只链接：
//   - pulse_app            创建/运行 app
//   - pulse_package_loader 动态加载 package
//   - pulse_config         读取 packages.json 并传递配置树
//
// 所有插件（包括 example-snake 本身）都通过 packages.json 在运行时
// 加载；每个包的 config 子对象以 PulseConfig* 借用形式传给插件。
// ============================================================

#include <assert.h>
#include <stdio.h>
#include <stdint.h>

#include <vector>

#include "pulse_app.h"
#include "pulse_config.h"
#include "pulse_package_loader.h"

int main(void)
{
    PulseAppDesc app_desc = {
        .name = "pulse-launcher",
        .enable_restapi = true,
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseConfig* root = pulse_config_create_from_json_file("packages.json");
    if (!root) {
        fprintf(stderr, "bad packages.json: %s\n", pulse_config_last_error());
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        return 1;
    }

    PulseConfigArray* packages = pulse_config_get_array(root, "packages");
    if (!packages) {
        fprintf(stderr, "bad packages.json: missing 'packages' array\n");
        pulse_config_release(root);
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        return 1;
    }

    const size_t count = pulse_config_array_count(packages);
    std::vector<PulsePackageListEntry> entries;
    std::vector<std::vector<const char*>> dependencies;
    entries.reserve(count);
    dependencies.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        PulseConfig* pkg = pulse_config_array_get(packages, i);
        if (!pkg) {
            fprintf(stderr, "bad packages.json: null package at index %zu\n", i);
            pulse_config_release(root);
            pulse_destroy_app(app);
            pulse_package_loader_cleanup(app);
            return 1;
        }

        PulsePackageListEntry entry = {};
        entry.name    = pulse_config_get_string(pkg, "name", nullptr);
        entry.library = pulse_config_get_string(pkg, "library", nullptr);
        entry.config  = pulse_config_get_obj(pkg, "config");

        PulseConfigArray* dep_arr = pulse_config_get_array(pkg, "dependencies");
        dependencies.emplace_back();
        if (dep_arr) {
            const size_t dep_count = pulse_config_array_count(dep_arr);
            dependencies.back().reserve(dep_count);
            for (size_t d = 0; d < dep_count; ++d) {
                PulseConfig* dep = pulse_config_array_get(dep_arr, d);
                dependencies.back().push_back(
                    dep ? pulse_config_get_string(dep, nullptr, nullptr) : nullptr);
            }
            entry.dependency_count = (uint32_t)dependencies.back().size();
            entry.dependencies = dependencies.back().data();
        }

        entries.push_back(entry);
    }

    EPulsePackageLoadResult load_result = pulse_package_loader_load_packages(app, entries.data(), (uint32_t)entries.size());

    if (load_result != PULSE_PACKAGE_LOAD_RESULT_OK) {
        fprintf(stderr, "failed to load packages, result=%d\n", (int)load_result);
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        pulse_config_release(root);
        return 1;
    }

    pulse_app_run(app);

    pulse_destroy_app(app);
    pulse_package_loader_cleanup(app);
    pulse_config_release(root);
    printf("Pulse launcher exited.\n");
    return 0;
}
