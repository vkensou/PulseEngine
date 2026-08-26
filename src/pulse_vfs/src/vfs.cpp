// ============================================================================
// pulse_vfs - PulseEngine virtual file system (core package)
//
// Registers "content roots" (search directories) and resolves asset paths
// through them. The backend uses SDL3 file I/O directly (SDL_IOFromFile /
// SDL_ReadIO / SDL_EnumerateDirectory).
//
// Resolution order:
//   1. Content roots, most recently added first (a package's own assets
//      shadow earlier registered roots with the same relative path).
//   2. The raw path relative to the working directory (fallback, also covers
//      absolute paths).
//
// API style follows C: fopen/fread/fseek-like stream handles for files
// (pulse_vfs_open_file / read / write / seek / tell / close), opendir/readdir-
// like scans for directories (pulse_vfs_open_dir / read_dir / close_dir), and
// stat-like queries (pulse_vfs_get_path_info).
// ============================================================================

#include "pulse_vfs.h"

#include <SDL3/SDL.h>

#include <cstring>
#include <mutex>
#include <new>
#include <string>
#include <vector>

namespace pulse::vfs {

std::vector<std::string>& content_roots() {
    static std::vector<std::string> roots;
    return roots;
}

std::mutex& roots_mutex() {
    static std::mutex m;
    return m;
}

// Normalizes a path so that equivalent spellings compare and join equal:
//   - backslashes become forward slashes
//   - leading/trailing whitespace is trimmed
//   - runs of '/' collapse to one ("//server" UNC prefixes kept)
//   - "." segments are dropped
//   - ".." pops the previous segment (kept when nothing can be popped)
//   - a trailing slash is dropped
// Returns "" for empty input, "." when everything collapsed (e.g. "./"),
// and "/" for the bare root.
std::string normalize_path(const char* path) {
    std::string in(path ? path : "");

    size_t b = 0;
    size_t e = in.size();
    while (b < e && (in[b] == ' ' || in[b] == '\t')) {
        ++b;
    }
    while (e > b && (in[e - 1] == ' ' || in[e - 1] == '\t')) {
        --e;
    }
    in = in.substr(b, e - b);
    for (char& c : in) {
        if (c == '\\') {
            c = '/';
        }
    }
    if (in.empty()) {
        return "";
    }

    std::vector<std::string> segs;
    size_t i = 0;
    while (i < in.size()) {
        size_t j = in.find('/', i);
        if (j == std::string::npos) {
            j = in.size();
        }
        std::string seg = in.substr(i, j - i);
        i = j + 1;

        if (seg.empty()) {
            // Leading slash(es): keep at most two empty segments so "/x" stays
            // rooted and "//server/share" keeps its UNC prefix; interior
            // repeats collapse.
            if (segs.empty()) {
                segs.emplace_back("");
            } else if (segs.size() == 1 && segs[0].empty()) {
                segs.emplace_back("");
            }
            continue;
        }
        if (seg == ".") {
            continue;
        }
        if (seg == "..") {
            // Pop a normal segment, but never the root marker, a drive
            // letter ("C:"), or a leading "..".
            if (!segs.empty() && segs.back() != "" && segs.back() != ".." &&
                !(segs.back().size() >= 2 && segs.back()[1] == ':')) {
                segs.pop_back();
            } else {
                segs.emplace_back("..");
            }
            continue;
        }
        segs.emplace_back(std::move(seg));
    }

    std::string out;
    for (size_t k = 0; k < segs.size(); ++k) {
        if (k > 0) {
            out += '/';
        }
        out += segs[k];
    }
    if (out.empty()) {
        out = (segs.size() == 1 && segs[0].empty()) ? "/" : ".";
    }
    return out;
}

// A content root is a path too; normalize it the same way so duplicate
// registration is detected ("a/b" vs "a/./b" vs "a\\b").
std::string normalize_root(const char* root) {
    return normalize_path(root);
}

std::string join_path(const std::string& root, const std::string& rel) {
    if (root.empty()) {
        return rel;
    }
    if (root.back() == '/') {
        return root + rel;
    }
    return root + "/" + rel;
}

// Returns true when full_path exists, be it a file, a directory or anything
// else the OS knows about (SDL_GetPathInfo reports type != NONE).
bool exists_at(const std::string& full_path) {
    SDL_PathInfo info{};
    if (!SDL_GetPathInfo(full_path.c_str(), &info)) {
        return false;
    }
    return info.type != SDL_PATHTYPE_NONE;
}

// Searches the content roots (most recent first), then falls back to the raw
// path relative to the working directory. Returns an empty string when the
// path cannot be found anywhere.
std::string resolve(const std::string& rel) {
    {
        std::lock_guard<std::mutex> lock(roots_mutex());
        const std::vector<std::string>& roots = content_roots();
        for (auto it = roots.rbegin(); it != roots.rend(); ++it) {
            std::string candidate = join_path(*it, rel);
            if (exists_at(candidate)) {
                return candidate;
            }
        }
    }
    if (exists_at(rel)) {
        return rel;
    }
    return {};
}

EPulseVfsPathType to_path_type(SDL_PathType type) {
    switch (type) {
        case SDL_PATHTYPE_NONE:
            return PULSE_VFS_PATH_TYPE_NONE;
        case SDL_PATHTYPE_FILE:
            return PULSE_VFS_PATH_TYPE_FILE;
        case SDL_PATHTYPE_DIRECTORY:
            return PULSE_VFS_PATH_TYPE_DIRECTORY;
        default:
            return PULSE_VFS_PATH_TYPE_OTHER;
    }
}

} // namespace pulse::vfs

// Opaque handle storage. Stay out of the pulse::vfs namespace on purpose: the
// public header declares them as struct PulseVfsFile / PulseVfsDir via the
// DEFINE_PULSE_OBJECT macro.
struct PulseVfsFile {
    SDL_IOStream* stream;
};

struct PulseVfsDir {
    std::vector<std::string> names;         // owns the entry name strings
    std::vector<PulseVfsDirEntry> entries;  // name pointers point into `names`
    size_t cursor = 0;
};

extern "C" {

PULSE_VFS_API EPulseVfsResult pulse_vfs_add_content_root(const char* root_path) {
    if (!root_path || !root_path[0]) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    std::string normalized = pulse::vfs::normalize_root(root_path);
    if (normalized.empty()) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }

    std::lock_guard<std::mutex> lock(pulse::vfs::roots_mutex());
    std::vector<std::string>& roots = pulse::vfs::content_roots();
    for (const std::string& root : roots) {
        if (root == normalized) {
            return PULSE_VFS_RESULT_OK; // already registered
        }
    }
    roots.push_back(std::move(normalized));
    return PULSE_VFS_RESULT_OK;
}

PULSE_VFS_API bool pulse_vfs_path_exists(const char* path) {
    if (!path || !path[0]) {
        return false;
    }
    return !pulse::vfs::resolve(pulse::vfs::normalize_path(path)).empty();
}

PULSE_VFS_API EPulseVfsResult pulse_vfs_get_path_info(const char* path, PulseVfsPathInfo* out_info) {
    if (out_info) {
        out_info->type = PULSE_VFS_PATH_TYPE_NONE;
        out_info->size = 0;
    }
    if (!path || !path[0] || !out_info) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }

    std::string resolved = pulse::vfs::resolve(pulse::vfs::normalize_path(path));
    if (resolved.empty()) {
        return PULSE_VFS_RESULT_ERROR_NOT_FOUND;
    }

    SDL_PathInfo raw{};
    if (!SDL_GetPathInfo(resolved.c_str(), &raw)) {
        return PULSE_VFS_RESULT_ERROR_IO;
    }
    out_info->type = pulse::vfs::to_path_type(raw.type);
    out_info->size = static_cast<uint64_t>(raw.size);
    return PULSE_VFS_RESULT_OK;
}

PULSE_VFS_API PulseVfsFileId pulse_vfs_open_file(const char* path, const char* mode) {
    if (!path || !path[0] || !mode || !mode[0]) {
        return nullptr;
    }

    std::string resolved = pulse::vfs::resolve(pulse::vfs::normalize_path(path));
    if (resolved.empty()) {
        // Read modes fail when nothing resolves. Write/append modes may also
        // target a file that does not exist yet ("wb"/"a" create it), so they
        // fall back to the raw path, like fopen() would.
        if (std::strchr(mode, 'r')) {
            return nullptr;
        }
        resolved = pulse::vfs::normalize_path(path);
    }

    SDL_IOStream* stream = SDL_IOFromFile(resolved.c_str(), mode);
    if (!stream) {
        return nullptr;
    }

    PulseVfsFile* file = new (std::nothrow) PulseVfsFile{stream};
    if (!file) {
        SDL_CloseIO(stream);
        return nullptr;
    }
    return file;
}

PULSE_VFS_API EPulseVfsResult pulse_vfs_read_file(PulseVfsFileId file, void* buffer, size_t max_bytes, size_t* out_read) {
    if (out_read) {
        *out_read = 0;
    }
    if (!file || !file->stream) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (max_bytes == 0) {
        return PULSE_VFS_RESULT_OK; // nothing requested, nothing to fail on
    }
    if (!buffer) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    size_t reads = SDL_ReadIO(file->stream, buffer, max_bytes);
    if (out_read) {
        *out_read = reads;
    }
    // (size_t)-1 is SDL's only real error signal. A 0 result means EOF (or a
    // zero-size request, handled above), which is normal flow, not an error:
    // callers loop while out_read > 0.
    if (reads == static_cast<size_t>(-1)) {
        return PULSE_VFS_RESULT_ERROR_IO;
    }
    return PULSE_VFS_RESULT_OK;
}

PULSE_VFS_API EPulseVfsResult pulse_vfs_write_file(PulseVfsFileId file, const void* buffer, size_t size) {
    if (!file || !file->stream) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    if (size == 0) {
        return PULSE_VFS_RESULT_OK;
    }
    if (!buffer) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    size_t writes = SDL_WriteIO(file->stream, buffer, size);
    return writes < size ? PULSE_VFS_RESULT_ERROR_IO : PULSE_VFS_RESULT_OK;
}

PULSE_VFS_API EPulseVfsResult pulse_vfs_seek_file(PulseVfsFileId file, int64_t offset, EPulseVfsSeekOrigin origin) {
    if (!file || !file->stream) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    SDL_IOWhence whence;
    switch (origin) {
        case PULSE_VFS_SEEK_ORIGIN_SET:
            whence = SDL_IO_SEEK_SET;
            break;
        case PULSE_VFS_SEEK_ORIGIN_CURRENT:
            whence = SDL_IO_SEEK_CUR;
            break;
        case PULSE_VFS_SEEK_ORIGIN_END:
            whence = SDL_IO_SEEK_END;
            break;
        default:
            return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    auto result = SDL_SeekIO(file->stream, offset, whence);
    return result >= 0 ? PULSE_VFS_RESULT_OK : PULSE_VFS_RESULT_ERROR_IO;
}

PULSE_VFS_API int64_t pulse_vfs_tell_file(PulseVfsFileId file) {
    if (!file || !file->stream) {
        return -1;
    }
    return SDL_TellIO(file->stream); // -1 on error
}

PULSE_VFS_API EPulseVfsResult pulse_vfs_get_file_size(PulseVfsFileId file, uint64_t* out_size) {
    if (!file || !file->stream || !out_size) {
        return PULSE_VFS_RESULT_ERROR_INVALID_ARGUMENT;
    }
    Sint64 size = SDL_GetIOSize(file->stream);
    if (size < 0) {
        return PULSE_VFS_RESULT_ERROR_IO;
    }
    *out_size = static_cast<uint64_t>(size);
    return PULSE_VFS_RESULT_OK;
}

PULSE_VFS_API void pulse_vfs_close_file(PulseVfsFileId file) {
    if (!file) {
        return;
    }
    if (file->stream) {
        SDL_CloseIO(file->stream);
    }
    file->stream = nullptr;
    delete file;
}

PULSE_VFS_API PulseVfsDirId pulse_vfs_open_dir(const char* path) {
    if (!path || !path[0]) {
        return nullptr;
    }

    std::string resolved = pulse::vfs::resolve(pulse::vfs::normalize_path(path));
    if (resolved.empty()) {
        return nullptr;
    }

    // Collect the raw names first. Entry name pointers must stay valid until
    // close_dir, so the entries table is built only after the enumeration
    // finished and `names` will never move again.
    std::vector<std::string> names;
    const bool ok = SDL_EnumerateDirectory(
        resolved.c_str(),
        [](void* userdata, const char* dirname, const char* fname) -> SDL_EnumerationResult {
            auto* n = static_cast<std::vector<std::string>*>(userdata);
            (void)dirname;
            n->emplace_back(fname);
            return SDL_ENUM_CONTINUE;
        },
        &names);

    if (!ok) {
        return nullptr;
    }

    PulseVfsDir* dir = new (std::nothrow) PulseVfsDir;
    if (!dir) {
        return nullptr;
    }

    dir->names = std::move(names);
    dir->entries.reserve(dir->names.size());
    for (const std::string& name : dir->names) {
        PulseVfsDirEntry entry{};
        entry.name = name.c_str();
        SDL_PathInfo info{};
        if (SDL_GetPathInfo(pulse::vfs::join_path(resolved, name).c_str(), &info)) {
            entry.type = pulse::vfs::to_path_type(info.type);
        } else {
            entry.type = PULSE_VFS_PATH_TYPE_NONE;
        }
        dir->entries.push_back(entry);
    }
    return dir;
}

PULSE_VFS_API PulseVfsDirEntry* pulse_vfs_read_dir(PulseVfsDirId dir) {
    if (!dir) {
        return nullptr;
    }
    if (dir->cursor >= dir->entries.size()) {
        return nullptr;
    }
    return &dir->entries[dir->cursor++];
}

PULSE_VFS_API void pulse_vfs_close_dir(PulseVfsDirId dir) {
    delete dir;
}

} // extern "C"
