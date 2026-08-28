// Verifies directory scanning of pulse_vfs (PHYSFS backend):
//   - mount makes a real directory visible in the virtual tree
//   - enumerate walks the tree: names + stat types, no "." / ".."
//   - mount points appear as directory entries of their parent
//   - recursive walking finds nested files
//   - callback flow control (STOP / ERROR) and failure cases
#include "pulse_vfs.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

struct ScanContext {
    std::vector<std::string> entries; // names seen during one scan
};

static std::string join(const std::string& base, const char* name) {
    if (base.empty() || base.back() == '/') {
        return base + name;
    }
    return base + "/" + name;
}

// Collects names; asserts on the entry contract.
static EPulseVfsEnumerateResult collect(void* data, const char* orig_dir, const char* name) {
    auto* ctx = static_cast<ScanContext*>(data);
    assert(orig_dir != nullptr);
    assert(name != nullptr && name[0] != '\0');
    assert(strcmp(name, ".") != 0 && strcmp(name, "..") != 0);
    ctx->entries.push_back(name);
    return PULSE_VFS_ENUMERATE_RESULT_OK;
}

// Recursive walk of the virtual tree; directories are re-enumerated.
static void walk(const std::string& vfs_dir, std::set<std::string>& out) {
    ScanContext ctx;
    assert(pulse_vfs_enumerate(vfs_dir.c_str(), collect, &ctx));
    for (const std::string& name : ctx.entries) {
        std::string rel = join(vfs_dir, name.c_str());
        out.insert(rel);
        PulseVfsStat info = {};
        assert(pulse_vfs_stat(rel.c_str(), &info));
        if (info.file_type == PULSE_VFS_FILE_TYPE_DIRECTORY) {
            walk(rel, out);
        }
    }
}

static bool has_entry(const ScanContext& ctx, const char* name) {
    for (const std::string& entry : ctx.entries) {
        if (entry == name) {
            return true;
        }
    }
    return false;
}

static EPulseVfsEnumerateResult stop_first_cb(void* data, const char*, const char*) {
    auto* calls = static_cast<int*>(data);
    ++*calls;
    return PULSE_VFS_ENUMERATE_RESULT_STOP;
}

static EPulseVfsEnumerateResult error_first_cb(void* data, const char*, const char*) {
    auto* calls = static_cast<int*>(data);
    ++*calls;
    return PULSE_VFS_ENUMERATE_RESULT_ERROR;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-vfs-dirs",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // Data dir mounted at the virtual root.
    assert(pulse_vfs_mount("tests/vfs/data", "/", false));

    // Full scan of the virtual root.
    ScanContext ctx;
    assert(pulse_vfs_enumerate("/", collect, &ctx));
    assert(has_entry(ctx, "hello.dat"));
    assert(has_entry(ctx, "sub"));
    for (const std::string& name : ctx.entries) {
        PulseVfsStat info = {};
        assert(pulse_vfs_stat(join("/", name.c_str()).c_str(), &info));
        if (name == "hello.dat") {
            assert(info.file_type == PULSE_VFS_FILE_TYPE_REGULAR);
            assert(info.file_size == 9);
        } else if (name == "sub") {
            assert(info.file_type == PULSE_VFS_FILE_TYPE_DIRECTORY);
        }
    }

    // Recursive walk sees the whole tree.
    std::set<std::string> found;
    walk("/", found);
    assert(found.count("/hello.dat") == 1);
    assert(found.count("/sub") == 1);
    assert(found.count("/sub/deep.dat") == 1);
    assert(found.size() == 3); // hello.dat, sub, sub/deep.dat

    // A mount point shows up as a directory entry of its parent.
    assert(pulse_vfs_mount("tests/vfs/data/sub", "/nested", true));
    ctx = ScanContext();
    assert(pulse_vfs_enumerate("/", collect, &ctx));
    assert(has_entry(ctx, "nested"));
    PulseVfsStat info = {};
    assert(pulse_vfs_stat("nested/deep.dat", &info));
    assert(info.file_type == PULSE_VFS_FILE_TYPE_REGULAR);
    assert(info.file_size == 8);

    // Unmounting removes the mount point entry again.
    assert(pulse_vfs_unmount("tests/vfs/data/sub"));
    ctx = ScanContext();
    assert(pulse_vfs_enumerate("/", collect, &ctx));
    assert(!has_entry(ctx, "nested"));

    // Callback flow control: STOP stops early and reports success, ERROR
    // stops with failure and an APP_CALLBACK error code.
    int calls = 0;
    assert(pulse_vfs_enumerate("/", stop_first_cb, &calls));
    assert(calls == 1);
    calls = 0;
    assert(!pulse_vfs_enumerate("/", error_first_cb, &calls));
    assert(calls == 1);
    assert(pulse_vfs_get_last_error_code() == PULSE_VFS_ERROR_CODE_APP_CALLBACK);

    // Failure cases: bad arguments are rejected, a missing or non-directory
    // path simply yields no entries.
    assert(!pulse_vfs_enumerate(nullptr, collect, &ctx));
    assert(!pulse_vfs_enumerate("/", nullptr, nullptr));
    ctx = ScanContext();
    assert(pulse_vfs_enumerate("no_such_dir", collect, &ctx));
    assert(ctx.entries.empty());
    ctx = ScanContext();
    assert(pulse_vfs_enumerate("hello.dat", collect, &ctx)); // a file, not a dir
    assert(ctx.entries.empty());

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("VFS directory scan test passed!\n");
    return 0;
}