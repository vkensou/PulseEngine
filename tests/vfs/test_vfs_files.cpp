// Verifies the C-style file API of pulse_vfs (PHYSFS backend):
//   - plugin descriptor
//   - mounting a real directory into the virtual tree
//   - path queries: exists / stat
//   - write dir management
//   - fopen-style stream I/O: open read/write/append, read/write bytes,
//     tell / seek / eof / file_length / close
//   - error code queries
#include "pulse_vfs.h"

#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

// Reads a stream to EOF into `out`. Fails the test on a read error.
static void read_all(PulseVfsFileId file, std::vector<char>& out) {
    char buf[64];
    for (;;) {
        int64_t n = pulse_vfs_read_bytes(file, buf, sizeof(buf));
        assert(n >= 0); // no read errors
        if (n == 0) {
            return; // EOF
        }
        out.insert(out.end(), buf, buf + n);
    }
}

static void test_plugin_desc(void) {
    PulseVfsPluginDesc desc = pulse_vfs_plugin_desc_default();
    assert(desc.struct_size == sizeof(PulseVfsPluginDesc));
    assert(desc.version == PULSE_VFS_PLUGIN_DESC_VERSION);
}

static void test_mount_and_queries(void) {
    // Mount failures.
    assert(!pulse_vfs_mount(nullptr, "/", false));
    assert(!pulse_vfs_mount("no_such_dir_xyz", "/", false));

    assert(pulse_vfs_mount("tests/vfs/data", "/", false));
    assert(pulse_vfs_mount("tests/vfs/data", "/", false)); // idempotent

    // exists.
    assert(pulse_vfs_exists("hello.dat"));
    assert(pulse_vfs_exists("sub"));
    assert(pulse_vfs_exists("sub/deep.dat"));
    assert(!pulse_vfs_exists("no_such_file.dat"));
    assert(pulse_vfs_exists("")); // empty path is the virtual root
    assert(!pulse_vfs_exists(nullptr));

    // stat.
    PulseVfsStat info = {};
    assert(pulse_vfs_stat("hello.dat", &info));
    assert(info.file_type == PULSE_VFS_FILE_TYPE_REGULAR);
    assert(info.file_size == 9);
    assert(pulse_vfs_stat("sub", &info));
    assert(info.file_type == PULSE_VFS_FILE_TYPE_DIRECTORY);
    assert(pulse_vfs_stat("sub/deep.dat", &info));
    assert(info.file_type == PULSE_VFS_FILE_TYPE_REGULAR);
    assert(info.file_size == 8);
    assert(!pulse_vfs_stat("no_such_file.dat", &info));
    assert(!pulse_vfs_stat("hello.dat", nullptr));
    assert(!pulse_vfs_stat(nullptr, &info));

    // A subdirectory mounted at its own mount point resolves there.
    assert(pulse_vfs_mount("tests/vfs/data/sub", "/nested", false));
    assert(pulse_vfs_exists("nested"));
    assert(pulse_vfs_exists("nested/deep.dat"));
    assert(pulse_vfs_stat("nested", &info));
    assert(info.file_type == PULSE_VFS_FILE_TYPE_DIRECTORY);
    assert(pulse_vfs_stat("nested/deep.dat", &info));
    assert(info.file_type == PULSE_VFS_FILE_TYPE_REGULAR);
    assert(info.file_size == 8);

    // Unmounting removes the mount point view but not the original path.
    assert(pulse_vfs_unmount("tests/vfs/data/sub"));
    assert(!pulse_vfs_exists("nested/deep.dat"));
    assert(pulse_vfs_exists("sub/deep.dat"));

    // Unmount failures.
    assert(!pulse_vfs_unmount("no_such_dir_xyz"));
    assert(!pulse_vfs_unmount(nullptr));
}

static void test_stream_read(void) {
    const char* expect = "hello vfs";
    const size_t expect_len = strlen(expect);

    PulseVfsFileId file = pulse_vfs_open_read("hello.dat");
    assert(file != nullptr);
    assert(pulse_vfs_tell(file) == 0);
    assert(pulse_vfs_file_length(file) == (int64_t)expect_len);
    assert(!pulse_vfs_eof(file));

    // Sequential reads from the start.
    char buf[64] = {0};
    assert(pulse_vfs_read_bytes(file, buf, 4) == 4);
    assert(strncmp(buf, "hell", 4) == 0);
    assert(pulse_vfs_tell(file) == 4);
    assert(pulse_vfs_read_bytes(file, buf, 4) == 4);
    assert(strncmp(buf, "o vf", 4) == 0);
    assert(pulse_vfs_tell(file) == 8);

    // Whole stream in one pass.
    assert(pulse_vfs_seek(file, 0));
    assert(pulse_vfs_tell(file) == 0);
    std::vector<char> all;
    read_all(file, all);
    assert(all.size() == expect_len);
    assert(memcmp(all.data(), expect, expect_len) == 0);
    assert(pulse_vfs_eof(file));

    // At EOF, reads return 0.
    assert(pulse_vfs_read_bytes(file, buf, sizeof(buf)) == 0);

    // Seek to an absolute offset and read the tail.
    assert(pulse_vfs_seek(file, 6));
    assert(pulse_vfs_tell(file) == 6);
    char tail[8] = {0};
    assert(pulse_vfs_read_bytes(file, tail, sizeof(tail)) == (int64_t)(expect_len - 6));
    assert(strcmp(tail, "vfs") == 0);

    assert(pulse_vfs_close(file));

    // Writing to a read-only handle fails.
    file = pulse_vfs_open_read("hello.dat");
    assert(file != nullptr);
    assert(pulse_vfs_write_bytes(file, "x", 1) == -1);
    assert(pulse_vfs_close(file));

    // Open failures.
    assert(pulse_vfs_open_read("no_such_file.dat") == nullptr);
    assert(pulse_vfs_open_read("sub") == nullptr); // a directory
    assert(pulse_vfs_open_read("") == nullptr);
    assert(pulse_vfs_open_read(nullptr) == nullptr);
}

static void test_stream_write(void) {
    // No write dir by default, and a nonexistent one cannot be set.
    assert(pulse_vfs_get_write_dir() == nullptr);
    assert(!pulse_vfs_set_write_dir("no_such_dir_xyz"));
    assert(pulse_vfs_get_write_dir() == nullptr);

    // Writes fail while no write dir is configured.
    assert(pulse_vfs_open_write("x.dat") == nullptr);

    // The build dir exists when the project has been built; the write dir
    // is returned verbatim and is NOT part of the search path.
    assert(pulse_vfs_set_write_dir("build"));
    const char* write_dir = pulse_vfs_get_write_dir();
    assert(write_dir != nullptr);
    assert(strcmp(write_dir, "build") == 0);

    // Writes land in the write dir; nothing is visible until it is mounted.
    const char* payload = "written through vfs\n";
    const size_t payload_len = strlen(payload);
    PulseVfsFileId file = pulse_vfs_open_write("vfs_tmp.dat");
    assert(file != nullptr);
    assert(pulse_vfs_write_bytes(file, payload, payload_len) == (int64_t)payload_len);
    assert(pulse_vfs_tell(file) == (int64_t)payload_len);
    // A write handle never reports EOF.
    assert(!pulse_vfs_eof(file));
    assert(pulse_vfs_close(file));

    assert(pulse_vfs_open_read("vfs_tmp.dat") == nullptr);
    assert(pulse_vfs_mount(write_dir, "/wd", true));

    // Read the file back through the VFS.
    file = pulse_vfs_open_read("wd/vfs_tmp.dat");
    assert(file != nullptr);
    assert(pulse_vfs_file_length(file) == (int64_t)payload_len);
    std::vector<char> all;
    read_all(file, all);
    assert(pulse_vfs_eof(file));
    assert(pulse_vfs_close(file));
    assert(all.size() == payload_len);
    assert(memcmp(all.data(), payload, payload_len) == 0);

    // Append mode adds at the end.
    file = pulse_vfs_open_append("vfs_tmp.dat");
    assert(file != nullptr);
    assert(pulse_vfs_write_bytes(file, "more", 4) == 4);
    assert(pulse_vfs_close(file));

    file = pulse_vfs_open_read("wd/vfs_tmp.dat");
    assert(file != nullptr);
    all.clear();
    read_all(file, all);
    assert(pulse_vfs_close(file));
    assert(all.size() == payload_len + 4);
    assert(memcmp(all.data(), payload, payload_len) == 0);
    assert(memcmp(all.data() + payload_len, "more", 4) == 0);

    // A mount with an open file cannot be unmounted.
    file = pulse_vfs_open_read("wd/vfs_tmp.dat");
    assert(file != nullptr);
    assert(!pulse_vfs_unmount(write_dir));
    assert(pulse_vfs_close(file));
    assert(pulse_vfs_unmount(write_dir));
    assert(pulse_vfs_open_read("wd/vfs_tmp.dat") == nullptr);
}

static void test_null_handles(void) {
    char buf[16] = {0};
    assert(pulse_vfs_read_bytes(nullptr, buf, sizeof(buf)) == -1);
    assert(pulse_vfs_write_bytes(nullptr, buf, sizeof(buf)) == -1);
    assert(!pulse_vfs_eof(nullptr));
    assert(pulse_vfs_tell(nullptr) == -1);
    assert(!pulse_vfs_seek(nullptr, 0));
    assert(pulse_vfs_file_length(nullptr) == -1);
    assert(!pulse_vfs_close(nullptr));
}

static void test_error_codes(void) {
    assert(pulse_vfs_get_error_by_code(PULSE_VFS_ERROR_CODE_OK) != nullptr);
    assert(pulse_vfs_get_error_by_code(PULSE_VFS_ERROR_CODE_NOT_FOUND) != nullptr);

    // A failed open leaves a queryable error code.
    assert(pulse_vfs_open_read("no_such_file.dat") == nullptr);
    assert(pulse_vfs_get_last_error_code() == PULSE_VFS_ERROR_CODE_NOT_FOUND);
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-vfs-files",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    test_plugin_desc();
    test_mount_and_queries();
    test_stream_read();
    test_stream_write();
    test_null_handles();
    test_error_codes();

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("VFS file API test passed!\n");
    return 0;
}