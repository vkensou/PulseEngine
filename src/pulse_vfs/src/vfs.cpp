// ============================================================================
// pulse_vfs - PulseEngine virtual file system (core package)
//
// Registers "content roots" (search directories) and resolves/reads asset
// paths through them. The backend uses SDL3 file I/O directly
// (SDL_IOFromFile / SDL_ReadIO), same as the asset package used to.
//
// Resolution order:
//   1. Content roots, most recently added first (a package's own assets
//      shadow earlier registered roots with the same relative path).
//   2. The raw path relative to the working directory (fallback, also covers
//      absolute paths).
// ============================================================================

#include "pulse_vfs.h"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <cstring>
#include <limits>
#include <mutex>
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

// Backslashes to forward slashes; strips trailing slashes.
std::string normalize_root(const char* root) {
    std::string out(root ? root : "");
    for (char& c : out) {
        if (c == '\\') {
            c = '/';
        }
    }
    while (out.size() > 1 && out.back() == '/') {
        out.pop_back();
    }
    return out;
}

std::string normalize_path(const char* path) {
    std::string out(path ? path : "");
    for (char& c : out) {
        if (c == '\\') {
            c = '/';
        }
    }
    return out;
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

bool file_exists_at(const std::string& full_path) {
    SDL_IOStream* stream = SDL_IOFromFile(full_path.c_str(), "rb");
    if (!stream) {
        return false;
    }
    SDL_CloseIO(stream);
    return true;
}

// Searches the content roots (most recent first), then falls back to the raw
// path relative to the working directory. Returns an empty string when the
// file cannot be found anywhere.
std::string resolve(const std::string& rel) {
    {
        std::lock_guard<std::mutex> lock(roots_mutex());
        const std::vector<std::string>& roots = content_roots();
        for (auto it = roots.rbegin(); it != roots.rend(); ++it) {
            std::string candidate = join_path(*it, rel);
            if (file_exists_at(candidate)) {
                return candidate;
            }
        }
    }
    if (file_exists_at(rel)) {
        return rel;
    }
    return {};
}

} // namespace pulse::vfs

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

PULSE_VFS_API bool pulse_vfs_resolve_path(const char* path, char* out_path, size_t out_path_size) {
    if (out_path && out_path_size > 0) {
        out_path[0] = '\0';
    }
    if (!path || !path[0] || !out_path || out_path_size == 0) {
        return false;
    }

    std::string resolved = pulse::vfs::resolve(pulse::vfs::normalize_path(path));
    if (resolved.empty()) {
        return false;
    }
    // resolved.size() + 1 includes the trailing NUL.
    if (resolved.size() + 1 > out_path_size) {
        return false;
    }
    std::memcpy(out_path, resolved.c_str(), resolved.size() + 1);
    return true;
}

PULSE_VFS_API bool pulse_vfs_file_exists(const char* path) {
    if (!path || !path[0]) {
        return false;
    }
    return !pulse::vfs::resolve(pulse::vfs::normalize_path(path)).empty();
}

PULSE_VFS_API bool pulse_vfs_read_file(const char* path, void** out_data, uint64_t* out_size) {
    if (out_data) {
        *out_data = nullptr;
    }
    if (out_size) {
        *out_size = 0;
    }
    if (!path || !path[0] || !out_data) {
        return false;
    }

    std::string resolved = pulse::vfs::resolve(pulse::vfs::normalize_path(path));
    if (resolved.empty()) {
        return false;
    }

    SDL_IOStream* stream = SDL_IOFromFile(resolved.c_str(), "rb");
    if (!stream) {
        return false;
    }

    Sint64 file_size = SDL_GetIOSize(stream);
    if (file_size < 0 ||
        static_cast<uint64_t>(file_size) > static_cast<uint64_t>(std::numeric_limits<size_t>::max())) {
        SDL_CloseIO(stream);
        return false;
    }

    const size_t size = static_cast<size_t>(file_size);
    void* data = std::malloc(size == 0 ? 1 : size);
    if (!data) {
        SDL_CloseIO(stream);
        return false;
    }

    size_t read = 0;
    while (read < size) {
        size_t chunk = SDL_ReadIO(stream, static_cast<uint8_t*>(data) + read, size - read);
        if (chunk == 0) {
            std::free(data);
            SDL_CloseIO(stream);
            return false;
        }
        read += chunk;
    }

    SDL_CloseIO(stream);
    *out_data = data;
    if (out_size) {
        *out_size = size;
    }
    return true;
}

PULSE_VFS_API void pulse_vfs_free_buffer(void* data) {
    std::free(data);
}

} // extern "C"