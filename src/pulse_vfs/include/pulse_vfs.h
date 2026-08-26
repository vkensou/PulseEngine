#pragma once

#ifndef PULSE_VFS_API_HEADER_GUARD
#define PULSE_VFS_API_HEADER_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint64_t
#include "pulse_platform.h"

#if defined(PULSE_VFS_MODULE_BUILD)
#  define PULSE_VFS_API PULSE_EXPORT
#else
#  define PULSE_VFS_API PULSE_IMPORT
#endif



typedef enum EPulseVfsResult
{
    PULSE_VFS_RESULT_OK,                      /** ( 0)                                */
    PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT,  /** ( 1)                                */
    PULSE_VFS_RESULT_ERROR_NOT_FOUND,         /** ( 2)                                */
    PULSE_VFS_RESULT_ERROR_BUFFER_TOO_SMALL,  /** ( 3)                                */
    PULSE_VFS_RESULT_ERROR_IO,                /** ( 4)                                */
    PULSE_VFS_RESULT_ERROR_INTERNAL,          /** ( 5)                                */

    PULSE_VFS_RESULT_COUNT

} EPulseVfsResult;

typedef enum EPulseVfsSeekOrigin
{
    PULSE_VFS_SEEK_ORIGIN_SET,                /** ( 0) Relative to the beginning of the stream */
    PULSE_VFS_SEEK_ORIGIN_CURRENT,            /** ( 1) Relative to the current read/write position */
    PULSE_VFS_SEEK_ORIGIN_END,                /** ( 2) Relative to the end of the stream */

    PULSE_VFS_SEEK_ORIGIN_COUNT

} EPulseVfsSeekOrigin;

typedef enum EPulseVfsPathType
{
    PULSE_VFS_PATH_TYPE_NONE,                 /** ( 0) Path does not exist            */
    PULSE_VFS_PATH_TYPE_FILE,                 /** ( 1) A regular file                 */
    PULSE_VFS_PATH_TYPE_DIRECTORY,            /** ( 2) A directory                    */
    PULSE_VFS_PATH_TYPE_OTHER,                /** ( 3) Something else (device, socket, ...) */

    PULSE_VFS_PATH_TYPE_COUNT

} EPulseVfsPathType;




DEFINE_PULSE_OBJECT(PulseVfsFile)
DEFINE_PULSE_OBJECT(PulseVfsDir)



typedef struct PulseVfsPathInfo
{
    EPulseVfsPathType    type;
    uint64_t             size;

} PulseVfsPathInfo;

typedef struct PulseVfsDirEntry
{
    const char*          name;
    EPulseVfsPathType    type;

} PulseVfsDirEntry;


PULSE_VFS_API EPulseVfsResult pulse_vfs_add_content_root(const char* root_path);
PULSE_VFS_API bool pulse_vfs_path_exists(const char* path);
PULSE_VFS_API EPulseVfsResult pulse_vfs_get_path_info(const char* path, PulseVfsPathInfo* out_info);
PULSE_VFS_API PulseVfsFileId pulse_vfs_open_file(const char* path, const char* mode);
PULSE_VFS_API EPulseVfsResult pulse_vfs_read_file(PulseVfsFileId file, void* buffer, size_t max_bytes, size_t* out_read);
PULSE_VFS_API EPulseVfsResult pulse_vfs_write_file(PulseVfsFileId file, const void* buffer, size_t size);
PULSE_VFS_API EPulseVfsResult pulse_vfs_seek_file(PulseVfsFileId file, int64_t offset, EPulseVfsSeekOrigin origin);
PULSE_VFS_API int64_t pulse_vfs_tell_file(PulseVfsFileId file);
PULSE_VFS_API EPulseVfsResult pulse_vfs_get_file_size(PulseVfsFileId file, uint64_t* out_size);
PULSE_VFS_API void pulse_vfs_close_file(PulseVfsFileId file);
PULSE_VFS_API PulseVfsDirId pulse_vfs_open_dir(const char* path);
PULSE_VFS_API PulseVfsDirEntry* pulse_vfs_read_dir(PulseVfsDirId dir);
PULSE_VFS_API void pulse_vfs_close_dir(PulseVfsDirId dir);

#ifdef __cplusplus
}
#endif

#endif // PULSE_VFS_API_HEADER_GUARD