// ============================================================================
// pulse_vfs - PulseEngine virtual file system
//
// Thin C API wrapper around PhysicsFS (PhysFS). The public surface is aligned
// with PHYSFS's core file/mount/stat/enumerate operations.
// ============================================================================

#include "pulse_vfs.h"

#include "physfs.h"

#include <new>

struct PulseVfsFile {
    PHYSFS_File* handle = nullptr;
};

namespace pulse::vfs {

constexpr const char* kPluginName = "pulse_vfs";

EPulsePluginBuildResult vfs_plugin_build_callback(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;

    if (PHYSFS_isInit()) {
        return PULSE_PLUGIN_BUILD_RESULT_OK;
    }
    return PHYSFS_init(nullptr) != 0
        ? PULSE_PLUGIN_BUILD_RESULT_OK
        : PULSE_PLUGIN_BUILD_RESULT_ERROR_INTERNAL;
}

void vfs_plugin_shutdown_callback(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;

    if (PHYSFS_isInit()) {
        PHYSFS_deinit();
    }
}

EPulseVfsErrorCode to_pulse_error(PHYSFS_ErrorCode code) {
    switch (code) {
        case PHYSFS_ERR_OK: return PULSE_VFS_ERROR_CODE_OK;
        case PHYSFS_ERR_OTHER_ERROR: return PULSE_VFS_ERROR_CODE_OTHER_ERROR;
        case PHYSFS_ERR_OUT_OF_MEMORY: return PULSE_VFS_ERROR_CODE_OUT_OF_MEMORY;
        case PHYSFS_ERR_NOT_INITIALIZED: return PULSE_VFS_ERROR_CODE_NOT_INITIALIZED;
        case PHYSFS_ERR_IS_INITIALIZED: return PULSE_VFS_ERROR_CODE_IS_INITIALIZED;
        case PHYSFS_ERR_ARGV0_IS_NULL: return PULSE_VFS_ERROR_CODE_ARGV0_IS_NULL;
        case PHYSFS_ERR_UNSUPPORTED: return PULSE_VFS_ERROR_CODE_UNSUPPORTED;
        case PHYSFS_ERR_PAST_EOF: return PULSE_VFS_ERROR_CODE_PAST_EOF;
        case PHYSFS_ERR_FILES_STILL_OPEN: return PULSE_VFS_ERROR_CODE_FILES_STILL_OPEN;
        case PHYSFS_ERR_INVALID_ARGUMENT: return PULSE_VFS_ERROR_CODE_INVALID_ARGUMENT;
        case PHYSFS_ERR_NOT_MOUNTED: return PULSE_VFS_ERROR_CODE_NOT_MOUNTED;
        case PHYSFS_ERR_NOT_FOUND: return PULSE_VFS_ERROR_CODE_NOT_FOUND;
        case PHYSFS_ERR_SYMLINK_FORBIDDEN: return PULSE_VFS_ERROR_CODE_SYMLINK_FORBIDDEN;
        case PHYSFS_ERR_NO_WRITE_DIR: return PULSE_VFS_ERROR_CODE_NO_WRITE_DIR;
        case PHYSFS_ERR_OPEN_FOR_READING: return PULSE_VFS_ERROR_CODE_OPEN_FOR_READING;
        case PHYSFS_ERR_OPEN_FOR_WRITING: return PULSE_VFS_ERROR_CODE_OPEN_FOR_WRITING;
        case PHYSFS_ERR_NOT_A_FILE: return PULSE_VFS_ERROR_CODE_NOT_AFILE;
        case PHYSFS_ERR_READ_ONLY: return PULSE_VFS_ERROR_CODE_READ_ONLY;
        case PHYSFS_ERR_CORRUPT: return PULSE_VFS_ERROR_CODE_CORRUPT;
        case PHYSFS_ERR_SYMLINK_LOOP: return PULSE_VFS_ERROR_CODE_SYMLINK_LOOP;
        case PHYSFS_ERR_IO: return PULSE_VFS_ERROR_CODE_IO;
        case PHYSFS_ERR_PERMISSION: return PULSE_VFS_ERROR_CODE_PERMISSION;
        case PHYSFS_ERR_NO_SPACE: return PULSE_VFS_ERROR_CODE_NO_SPACE;
        case PHYSFS_ERR_BAD_FILENAME: return PULSE_VFS_ERROR_CODE_BAD_FILENAME;
        case PHYSFS_ERR_BUSY: return PULSE_VFS_ERROR_CODE_BUSY;
        case PHYSFS_ERR_DIR_NOT_EMPTY: return PULSE_VFS_ERROR_CODE_DIR_NOT_EMPTY;
        case PHYSFS_ERR_OS_ERROR: return PULSE_VFS_ERROR_CODE_OSERROR;
        case PHYSFS_ERR_DUPLICATE: return PULSE_VFS_ERROR_CODE_DUPLICATE;
        case PHYSFS_ERR_BAD_PASSWORD: return PULSE_VFS_ERROR_CODE_BAD_PASSWORD;
        case PHYSFS_ERR_APP_CALLBACK: return PULSE_VFS_ERROR_CODE_APP_CALLBACK;
    }
    return PULSE_VFS_ERROR_CODE_OTHER_ERROR;
}

PHYSFS_ErrorCode to_physfs_error(EPulseVfsErrorCode code) {
    switch (code) {
        case PULSE_VFS_ERROR_CODE_OK: return PHYSFS_ERR_OK;
        case PULSE_VFS_ERROR_CODE_OTHER_ERROR: return PHYSFS_ERR_OTHER_ERROR;
        case PULSE_VFS_ERROR_CODE_OUT_OF_MEMORY: return PHYSFS_ERR_OUT_OF_MEMORY;
        case PULSE_VFS_ERROR_CODE_NOT_INITIALIZED: return PHYSFS_ERR_NOT_INITIALIZED;
        case PULSE_VFS_ERROR_CODE_IS_INITIALIZED: return PHYSFS_ERR_IS_INITIALIZED;
        case PULSE_VFS_ERROR_CODE_ARGV0_IS_NULL: return PHYSFS_ERR_ARGV0_IS_NULL;
        case PULSE_VFS_ERROR_CODE_UNSUPPORTED: return PHYSFS_ERR_UNSUPPORTED;
        case PULSE_VFS_ERROR_CODE_PAST_EOF: return PHYSFS_ERR_PAST_EOF;
        case PULSE_VFS_ERROR_CODE_FILES_STILL_OPEN: return PHYSFS_ERR_FILES_STILL_OPEN;
        case PULSE_VFS_ERROR_CODE_INVALID_ARGUMENT: return PHYSFS_ERR_INVALID_ARGUMENT;
        case PULSE_VFS_ERROR_CODE_NOT_MOUNTED: return PHYSFS_ERR_NOT_MOUNTED;
        case PULSE_VFS_ERROR_CODE_NOT_FOUND: return PHYSFS_ERR_NOT_FOUND;
        case PULSE_VFS_ERROR_CODE_SYMLINK_FORBIDDEN: return PHYSFS_ERR_SYMLINK_FORBIDDEN;
        case PULSE_VFS_ERROR_CODE_NO_WRITE_DIR: return PHYSFS_ERR_NO_WRITE_DIR;
        case PULSE_VFS_ERROR_CODE_OPEN_FOR_READING: return PHYSFS_ERR_OPEN_FOR_READING;
        case PULSE_VFS_ERROR_CODE_OPEN_FOR_WRITING: return PHYSFS_ERR_OPEN_FOR_WRITING;
        case PULSE_VFS_ERROR_CODE_NOT_AFILE: return PHYSFS_ERR_NOT_A_FILE;
        case PULSE_VFS_ERROR_CODE_READ_ONLY: return PHYSFS_ERR_READ_ONLY;
        case PULSE_VFS_ERROR_CODE_CORRUPT: return PHYSFS_ERR_CORRUPT;
        case PULSE_VFS_ERROR_CODE_SYMLINK_LOOP: return PHYSFS_ERR_SYMLINK_LOOP;
        case PULSE_VFS_ERROR_CODE_IO: return PHYSFS_ERR_IO;
        case PULSE_VFS_ERROR_CODE_PERMISSION: return PHYSFS_ERR_PERMISSION;
        case PULSE_VFS_ERROR_CODE_NO_SPACE: return PHYSFS_ERR_NO_SPACE;
        case PULSE_VFS_ERROR_CODE_BAD_FILENAME: return PHYSFS_ERR_BAD_FILENAME;
        case PULSE_VFS_ERROR_CODE_BUSY: return PHYSFS_ERR_BUSY;
        case PULSE_VFS_ERROR_CODE_DIR_NOT_EMPTY: return PHYSFS_ERR_DIR_NOT_EMPTY;
        case PULSE_VFS_ERROR_CODE_OSERROR: return PHYSFS_ERR_OS_ERROR;
        case PULSE_VFS_ERROR_CODE_DUPLICATE: return PHYSFS_ERR_DUPLICATE;
        case PULSE_VFS_ERROR_CODE_BAD_PASSWORD: return PHYSFS_ERR_BAD_PASSWORD;
        case PULSE_VFS_ERROR_CODE_APP_CALLBACK: return PHYSFS_ERR_APP_CALLBACK;
        case PULSE_VFS_ERROR_CODE_COUNT: break;
    }
    return PHYSFS_ERR_OTHER_ERROR;
}

EPulseVfsFileType to_pulse_file_type(PHYSFS_FileType type) {
    switch (type) {
        case PHYSFS_FILETYPE_REGULAR: return PULSE_VFS_FILE_TYPE_REGULAR;
        case PHYSFS_FILETYPE_DIRECTORY: return PULSE_VFS_FILE_TYPE_DIRECTORY;
        case PHYSFS_FILETYPE_SYMLINK: return PULSE_VFS_FILE_TYPE_SYMLINK;
        case PHYSFS_FILETYPE_OTHER: return PULSE_VFS_FILE_TYPE_OTHER;
    }
    return PULSE_VFS_FILE_TYPE_OTHER;
}

struct EnumerateContext {
    PulseProcVfsEnumerateCallback callback = nullptr;
    void* user_data = nullptr;
};

PHYSFS_EnumerateCallbackResult enumerate_adapter(void* data, const char* origdir, const char* fname) {
    auto* ctx = static_cast<EnumerateContext*>(data);
    return static_cast<PHYSFS_EnumerateCallbackResult>(
        ctx->callback(ctx->user_data, origdir, fname));
}

} // namespace pulse::vfs

extern "C" {

PULSE_VFS_API PulseVfsPluginDesc pulse_vfs_plugin_desc_default(void) {
    PulseVfsPluginDesc desc{};
    desc.struct_size = sizeof(PulseVfsPluginDesc);
    desc.version = PULSE_VFS_PLUGIN_DESC_VERSION;
    return desc;
}

PULSE_VFS_API EPulseAppAddPluginResult pulse_add_vfs_plugin(
    PulseAppId app,
    const PulseVfsPluginDesc* desc
) {
    if (!app) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (desc &&
        (desc->struct_size != sizeof(PulseVfsPluginDesc) ||
         desc->version != PULSE_VFS_PLUGIN_DESC_VERSION)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (pulse_app_has_plugin(app, pulse::vfs::kPluginName)) {
        return PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN;
    }

    PulsePluginDesc plugin_desc{};
    plugin_desc.struct_size = sizeof(PulsePluginDesc);
    plugin_desc.version = PULSE_PLUGIN_DESC_VERSION;
    plugin_desc.plugin_version = PULSE_VFS_PLUGIN_DESC_VERSION;
    plugin_desc.name = pulse::vfs::kPluginName;
    plugin_desc.ctx = nullptr;
    plugin_desc.build = pulse::vfs::vfs_plugin_build_callback;
    plugin_desc.post_build = nullptr;
    plugin_desc.shutdown = pulse::vfs::vfs_plugin_shutdown_callback;
    plugin_desc.dependency_count = 0;
    plugin_desc.dependencies = nullptr;

    return pulse_app_add_plugin(app, &plugin_desc);
}

PULSE_VFS_API bool pulse_vfs_mount(const char* new_dir, const char* mount_point, bool append_to_path) {
    return PHYSFS_mount(new_dir, mount_point, append_to_path ? 1 : 0) != 0;
}

PULSE_VFS_API bool pulse_vfs_unmount(const char* old_dir) {
    return PHYSFS_unmount(old_dir) != 0;
}

PULSE_VFS_API const char* pulse_vfs_get_write_dir(void) {
    return PHYSFS_getWriteDir();
}

PULSE_VFS_API bool pulse_vfs_set_write_dir(const char* new_dir) {
    return PHYSFS_setWriteDir(new_dir) != 0;
}

PULSE_VFS_API bool pulse_vfs_exists(const char* fname) {
    if (!fname) {
        return false;
    }
    return PHYSFS_exists(fname) != 0;
}

PULSE_VFS_API bool pulse_vfs_stat(const char* fname, PulseVfsStat* stat) {
    if (!fname || !stat) {
        return false;
    }

    PHYSFS_Stat raw{};
    if (PHYSFS_stat(fname, &raw) == 0) {
        return false;
    }

    stat->file_size = raw.filesize;
    stat->mod_time = raw.modtime;
    stat->create_time = raw.createtime;
    stat->access_time = raw.accesstime;
    stat->file_type = pulse::vfs::to_pulse_file_type(raw.filetype);
    stat->read_only = raw.readonly != 0;
    return true;
}

PULSE_VFS_API PulseVfsFileId pulse_vfs_open_read(const char* filename) {
    PHYSFS_File* handle = PHYSFS_openRead(filename);
    if (!handle) {
        return nullptr;
    }

    PulseVfsFile* file = new (std::nothrow) PulseVfsFile;
    if (!file) {
        PHYSFS_close(handle);
        return nullptr;
    }
    file->handle = handle;
    return file;
}

PULSE_VFS_API PulseVfsFileId pulse_vfs_open_write(const char* filename) {
    PHYSFS_File* handle = PHYSFS_openWrite(filename);
    if (!handle) {
        return nullptr;
    }

    PulseVfsFile* file = new (std::nothrow) PulseVfsFile;
    if (!file) {
        PHYSFS_close(handle);
        return nullptr;
    }
    file->handle = handle;
    return file;
}

PULSE_VFS_API PulseVfsFileId pulse_vfs_open_append(const char* filename) {
    PHYSFS_File* handle = PHYSFS_openAppend(filename);
    if (!handle) {
        return nullptr;
    }

    PulseVfsFile* file = new (std::nothrow) PulseVfsFile;
    if (!file) {
        PHYSFS_close(handle);
        return nullptr;
    }
    file->handle = handle;
    return file;
}

PULSE_VFS_API bool pulse_vfs_close(PulseVfsFileId file) {
    if (!file || !file->handle) {
        return false;
    }
    if (PHYSFS_close(file->handle) == 0) {
        return false;
    }
    file->handle = nullptr;
    delete file;
    return true;
}

PULSE_VFS_API int64_t pulse_vfs_read_bytes(PulseVfsFileId file, void* buffer, uint64_t len) {
    if (!file || !file->handle) {
        return -1;
    }
    return PHYSFS_readBytes(file->handle, buffer, len);
}

PULSE_VFS_API int64_t pulse_vfs_write_bytes(PulseVfsFileId file, const void* buffer, uint64_t len) {
    if (!file || !file->handle) {
        return -1;
    }
    return PHYSFS_writeBytes(file->handle, buffer, len);
}

PULSE_VFS_API bool pulse_vfs_eof(PulseVfsFileId file) {
    if (!file || !file->handle) {
        return false;
    }
    return PHYSFS_eof(file->handle) != 0;
}

PULSE_VFS_API int64_t pulse_vfs_tell(PulseVfsFileId file) {
    if (!file || !file->handle) {
        return -1;
    }
    return PHYSFS_tell(file->handle);
}

PULSE_VFS_API bool pulse_vfs_seek(PulseVfsFileId file, uint64_t pos) {
    if (!file || !file->handle) {
        return false;
    }
    return PHYSFS_seek(file->handle, pos) != 0;
}

PULSE_VFS_API int64_t pulse_vfs_file_length(PulseVfsFileId file) {
    if (!file || !file->handle) {
        return -1;
    }
    return PHYSFS_fileLength(file->handle);
}

PULSE_VFS_API bool pulse_vfs_enumerate(const char* dir, PulseProcVfsEnumerateCallback callback, void* user_data) {
    if (!dir || !callback) {
        return false;
    }

    pulse::vfs::EnumerateContext ctx;
    ctx.callback = callback;
    ctx.user_data = user_data;
    return PHYSFS_enumerate(dir, pulse::vfs::enumerate_adapter, &ctx) != 0;
}

PULSE_VFS_API EPulseVfsErrorCode pulse_vfs_get_last_error_code(void) {
    return pulse::vfs::to_pulse_error(PHYSFS_getLastErrorCode());
}

PULSE_VFS_API const char* pulse_vfs_get_error_by_code(EPulseVfsErrorCode code) {
    return PHYSFS_getErrorByCode(pulse::vfs::to_physfs_error(code));
}

} // extern "C"
