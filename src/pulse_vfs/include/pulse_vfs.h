#pragma once

#ifndef PULSE_VFS_API_HEADER_GUARD
#define PULSE_VFS_API_HEADER_GUARD
#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunknown-attributes"
#elif defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wattributes"
#elif defined(_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:5030)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint64_t
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_VFS_MODULE_BUILD)
#  define PULSE_VFS_API PULSE_EXPORT
#else
#  define PULSE_VFS_API PULSE_IMPORT
#endif

#define PULSE_VFS_PLUGIN_DESC_VERSION 1u


/**
 * Pulse VFS error codes, aligned with PHYSFS_ErrorCode.
 *
 */
typedef enum EPulseVfsErrorCode
{
    PULSE_VFS_ERROR_CODE_OK,                  /** ( 0)                                */
    PULSE_VFS_ERROR_CODE_OTHER_ERROR,         /** ( 1)                                */
    PULSE_VFS_ERROR_CODE_OUT_OF_MEMORY,       /** ( 2)                                */
    PULSE_VFS_ERROR_CODE_NOT_INITIALIZED,     /** ( 3)                                */
    PULSE_VFS_ERROR_CODE_IS_INITIALIZED,      /** ( 4)                                */
    PULSE_VFS_ERROR_CODE_ARGV0_IS_NULL,       /** ( 5)                                */
    PULSE_VFS_ERROR_CODE_UNSUPPORTED,         /** ( 6)                                */
    PULSE_VFS_ERROR_CODE_PAST_EOF,            /** ( 7)                                */
    PULSE_VFS_ERROR_CODE_FILES_STILL_OPEN,    /** ( 8)                                */
    PULSE_VFS_ERROR_CODE_INVALID_ARGUMENT,    /** ( 9)                                */
    PULSE_VFS_ERROR_CODE_NOT_MOUNTED,         /** (10)                                */
    PULSE_VFS_ERROR_CODE_NOT_FOUND,           /** (11)                                */
    PULSE_VFS_ERROR_CODE_SYMLINK_FORBIDDEN,   /** (12)                                */
    PULSE_VFS_ERROR_CODE_NO_WRITE_DIR,        /** (13)                                */
    PULSE_VFS_ERROR_CODE_OPEN_FOR_READING,    /** (14)                                */
    PULSE_VFS_ERROR_CODE_OPEN_FOR_WRITING,    /** (15)                                */
    PULSE_VFS_ERROR_CODE_NOT_AFILE,           /** (16)                                */
    PULSE_VFS_ERROR_CODE_READ_ONLY,           /** (17)                                */
    PULSE_VFS_ERROR_CODE_CORRUPT,             /** (18)                                */
    PULSE_VFS_ERROR_CODE_SYMLINK_LOOP,        /** (19)                                */
    PULSE_VFS_ERROR_CODE_IO,                  /** (20)                                */
    PULSE_VFS_ERROR_CODE_PERMISSION,          /** (21)                                */
    PULSE_VFS_ERROR_CODE_NO_SPACE,            /** (22)                                */
    PULSE_VFS_ERROR_CODE_BAD_FILENAME,        /** (23)                                */
    PULSE_VFS_ERROR_CODE_BUSY,                /** (24)                                */
    PULSE_VFS_ERROR_CODE_DIR_NOT_EMPTY,       /** (25)                                */
    PULSE_VFS_ERROR_CODE_OSERROR,             /** (26)                                */
    PULSE_VFS_ERROR_CODE_DUPLICATE,           /** (27)                                */
    PULSE_VFS_ERROR_CODE_BAD_PASSWORD,        /** (28)                                */
    PULSE_VFS_ERROR_CODE_APP_CALLBACK,        /** (29)                                */

    PULSE_VFS_ERROR_CODE_COUNT

} EPulseVfsErrorCode;

/**
 * Pulse VFS file type, aligned with PHYSFS_FileType.
 *
 */
typedef enum EPulseVfsFileType
{
    PULSE_VFS_FILE_TYPE_REGULAR,              /** ( 0) A normal file                  */
    PULSE_VFS_FILE_TYPE_DIRECTORY,            /** ( 1) A directory                    */
    PULSE_VFS_FILE_TYPE_SYMLINK,              /** ( 2) A symbolic link                */
    PULSE_VFS_FILE_TYPE_OTHER,                /** ( 3) Something completely different like a device */

    PULSE_VFS_FILE_TYPE_COUNT

} EPulseVfsFileType;

/**
 * Result returned by VfsEnumerateCallback, aligned with PHYSFS_EnumerateCallbackResult.
 *
 */
typedef enum EPulseVfsEnumerateResult
{
    PULSE_VFS_ENUMERATE_RESULT_ERROR = -1,    /** Stop enumerating, report error to app */
    PULSE_VFS_ENUMERATE_RESULT_STOP = 0,      /** Stop enumerating, report success to app */
    PULSE_VFS_ENUMERATE_RESULT_OK = 1,        /** Keep enumerating, no problems */
} EPulseVfsEnumerateResult;




DEFINE_PULSE_OBJECT(PulseVfsFile)

/**
 * Callback for enumeration, aligned with PHYSFS_EnumerateCallback.
 *
 * @param[in] data
 * @param[in] origDir
 * @param[in] fname
 *
 */
typedef EPulseVfsEnumerateResult (*PulseProcVfsEnumerateCallback)([[pulse::optional]] void* data, const char* orig_dir, const char* fname);

/**
 * Metadata for a file or directory, aligned with PHYSFS_Stat.
 *
 */
typedef struct PulseVfsStat
{
    int64_t              file_size;
    int64_t              mod_time;
    int64_t              create_time;
    int64_t              access_time;
    EPulseVfsFileType    file_type;
    bool                 read_only;

} PulseVfsStat;

/**
 * Plugin descriptor
 *
 */
typedef struct PulseVfsPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;

} PulseVfsPluginDesc;


PULSE_VFS_API PulseVfsPluginDesc pulse_vfs_plugin_desc_default(void);
PULSE_VFS_API EPulseAppAddPluginResult pulse_add_vfs_plugin(PulseAppId app, const PulseVfsPluginDesc* desc);
PULSE_VFS_API bool pulse_vfs_mount(const char* new_dir, [[pulse::optional]] const char* mount_point, bool append_to_path);
PULSE_VFS_API bool pulse_vfs_unmount(const char* old_dir);
PULSE_VFS_API [[pulse::optional]] const char* pulse_vfs_get_write_dir(void);
PULSE_VFS_API bool pulse_vfs_set_write_dir([[pulse::optional]] const char* new_dir);
PULSE_VFS_API bool pulse_vfs_exists(const char* fname);
PULSE_VFS_API bool pulse_vfs_stat(const char* fname, PulseVfsStat* stat);
PULSE_VFS_API [[pulse::optional]] PulseVfsFileId pulse_vfs_open_read(const char* filename);
PULSE_VFS_API [[pulse::optional]] PulseVfsFileId pulse_vfs_open_write(const char* filename);
PULSE_VFS_API [[pulse::optional]] PulseVfsFileId pulse_vfs_open_append(const char* filename);
PULSE_VFS_API bool pulse_vfs_close(PulseVfsFileId file);
PULSE_VFS_API int64_t pulse_vfs_read_bytes(PulseVfsFileId file, void* buffer, uint64_t len);
PULSE_VFS_API int64_t pulse_vfs_write_bytes(PulseVfsFileId file, const void* buffer, uint64_t len);
PULSE_VFS_API bool pulse_vfs_eof(PulseVfsFileId file);
PULSE_VFS_API int64_t pulse_vfs_tell(PulseVfsFileId file);
PULSE_VFS_API bool pulse_vfs_seek(PulseVfsFileId file, uint64_t pos);
PULSE_VFS_API int64_t pulse_vfs_file_length(PulseVfsFileId file);
PULSE_VFS_API bool pulse_vfs_enumerate(const char* dir, PulseProcVfsEnumerateCallback callback, [[pulse::optional]] void* user_data);
PULSE_VFS_API EPulseVfsErrorCode pulse_vfs_get_last_error_code(void);
PULSE_VFS_API [[pulse::optional]] const char* pulse_vfs_get_error_by_code(EPulseVfsErrorCode code);

#ifdef __cplusplus
}
#endif

#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#  pragma warning(pop)
#endif
#endif // PULSE_VFS_API_HEADER_GUARD
