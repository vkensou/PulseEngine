// ============================================================
// PulseEngine Launcher (plugin package loader)
//
// 这个 launcher 不链接任何具体的 pulse_* 插件，只链接：
//   - pulse_app            创建/运行 app
//   - pulse_package_loader 动态加载 package
//   - pulse_config         读取 launcher.manifest 并传递配置树
//
// launcher 只负责获取需要加载的包列表及其 config；
// 每个包自己的 package.json（library、dependencies 等）由
// pulse_package_loader 内部读取和解析。
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

    PulseConfig* root = pulse_config_create_from_json_file("launcher.manifest");
    if (!root) {
        fprintf(stderr, "bad launcher.manifest: %s\n", pulse_config_last_error());
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        return 1;
    }

    PulseConfigArray* packages = pulse_config_get_array(root, "packages");
    if (!packages) {
        fprintf(stderr, "bad launcher.manifest: missing 'packages' array\n");
        pulse_config_release(root);
        pulse_destroy_app(app);
        pulse_package_loader_cleanup(app);
        return 1;
    }

    const size_t count = pulse_config_array_count(packages);
    std::vector<PulsePackageListEntry> entries;
    entries.reserve(count);

    for (size_t i = 0; i < count; ++i) {
        PulseConfig* pkg = pulse_config_array_get(packages, i);
        if (!pkg) {
            fprintf(stderr, "bad launcher.manifest: null package at index %zu\n", i);
            pulse_config_release(root);
            pulse_destroy_app(app);
            pulse_package_loader_cleanup(app);
            return 1;
        }

        PulsePackageListEntry entry = {};
        entry.name = pulse_config_get_string(pkg, "name", nullptr);
        if (!entry.name || !entry.name[0]) {
            fprintf(stderr, "bad launcher.manifest: missing 'name' at index %zu\n", i);
            pulse_config_release(root);
            pulse_destroy_app(app);
            pulse_package_loader_cleanup(app);
            return 1;
        }
        entry.config = pulse_config_get_obj(pkg, "config");
        entries.push_back(entry);
    }

    const char* search_paths[] = { "packages" };
    EPulsePackageLoadResult load_result = pulse_package_loader_load_packages(app, (uint32_t)(sizeof(search_paths) / sizeof(search_paths[0])), search_paths, (uint32_t)entries.size(), entries.data());

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
