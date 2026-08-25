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



/**
 * Operation result codes for VFS calls
 *
 */
typedef enum EPulseVfsResult
{
    PULSE_VFS_RESULT_OK,                      /** ( 0)                                */
    PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT,  /** ( 1)                                */
    PULSE_VFS_RESULT_ERROR_NOT_FOUND,         /** ( 2)                                */
    PULSE_VFS_RESULT_ERROR_INTERNAL,          /** ( 3)                                */

    PULSE_VFS_RESULT_COUNT

} EPulseVfsResult;










PULSE_VFS_API EPulseVfsResult pulse_vfs_add_content_root(const char* root_path);
PULSE_VFS_API bool pulse_vfs_resolve_path(const char* path, char* out_path, size_t out_path_size);
PULSE_VFS_API bool pulse_vfs_file_exists(const char* path);
PULSE_VFS_API bool pulse_vfs_read_file(const char* path, void** out_data, uint64_t* out_size);
PULSE_VFS_API void pulse_vfs_free_buffer(void* data);

#ifdef __cplusplus
}
#endif

#endif // PULSE_VFS_API_HEADER_GUARD