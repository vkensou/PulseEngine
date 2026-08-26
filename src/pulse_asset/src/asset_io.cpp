#include "asset_internal.h"

#include "pulse_vfs.h"
#include <algorithm>

namespace pulse::asset {

PooledBlock::~PooledBlock() {
    reset();
}

PooledBlock::PooledBlock(PooledBlock&& other) noexcept
    : data(other.data),
      size(other.size),
      align(other.align),
      resource(other.resource) {
    other.data = nullptr;
    other.size = 0;
    other.align = 0;
    other.resource = nullptr;
}

PooledBlock& PooledBlock::operator=(PooledBlock&& other) noexcept {
    if (this != &other) {
        reset();
        data = other.data;
        size = other.size;
        align = other.align;
        resource = other.resource;
        other.data = nullptr;
        other.size = 0;
        other.align = 0;
        other.resource = nullptr;
    }
    return *this;
}

void PooledBlock::reset() {
    if (data && resource) {
        resource->deallocate(data, size, align);
    }
    data = nullptr;
    size = 0;
    align = 0;
    resource = nullptr;
}

bool PooledBlock::allocate(std::pmr::memory_resource* allocator, uint32_t byte_size, uint32_t alignment, bool zero_memory) {
    reset();
    if (byte_size == 0) {
        return true;
    }
    if (!allocator || alignment == 0) {
        return false;
    }

    data = allocator->allocate(byte_size, alignment);
    if (!data) {
        return false;
    }
    size = byte_size;
    align = alignment;
    resource = allocator;
    if (zero_memory) {
        std::memset(data, 0, byte_size);
    }
    return true;
}

bool PooledBlock::copy(std::pmr::memory_resource* allocator, const void* bytes, uint32_t byte_size, uint32_t alignment) {
    if (!bytes || byte_size == 0) {
        reset();
        return true;
    }
    if (!allocate(allocator, byte_size, alignment, false)) {
        return false;
    }
    std::memcpy(data, bytes, byte_size);
    return true;
}

std::pmr::string AssetIo::normalize_path(const char* path, std::pmr::memory_resource* resource) {
    std::pmr::string out(path ? path : "", resource);
    for (char& c : out) {
        if (c == '\\') {
            c = '/';
        }
    }

    size_t first_relative_char = out.find_first_not_of('/');
    if (first_relative_char == std::pmr::string::npos) {
        out.clear();
    } else if (first_relative_char > 0) {
        out.erase(0, first_relative_char);
    }
    return out;
}

std::pmr::string AssetIo::normalize_extension(const char* extension, std::pmr::memory_resource* resource) {
    std::pmr::string out(extension ? extension : "", resource);
    if (!out.empty() && out[0] == '.') {
        out.erase(out.begin());
    }
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return out;
}

std::pmr::vector<std::pmr::string> AssetIo::parse_extensions(const char* extensions, std::pmr::memory_resource* resource) {
    std::pmr::vector<std::pmr::string> out(resource);
    if (!extensions) {
        return out;
    }

    std::pmr::string current(resource);
    for (const char* c = extensions; ; ++c) {
        if (*c == ',' || *c == '\0') {
            std::pmr::string normalized = normalize_extension(current.c_str(), resource);
            if (!normalized.empty()) {
                out.push_back(std::move(normalized));
            }
            current.clear();
            if (*c == '\0') {
                break;
            }
        } else if (*c != ' ' && *c != '\t') {
            current.push_back(*c);
        }
    }
    return out;
}

std::pmr::string AssetIo::extension_from_path(const std::pmr::string& path, std::pmr::memory_resource* resource) {
    size_t slash = path.find_last_of("/");
    size_t dot = path.find_last_of('.');
    if (dot == std::pmr::string::npos || (slash != std::pmr::string::npos && dot < slash)) {
        return std::pmr::string(resource);
    }
    return normalize_extension(path.c_str() + dot + 1, resource);
}

std::optional<std::pmr::vector<uint8_t>> AssetIo::read_file(const char* filename, std::pmr::memory_resource* resource) {
    PulseVfsFileId file = pulse_vfs_open_read(filename);
    if (!file) {
        return std::optional<std::pmr::vector<uint8_t>>{};
    }

    std::pmr::vector<uint8_t> buffer(resource);
    uint8_t chunk[8192];
    for (;;) {
        int64_t n = pulse_vfs_read_bytes(file, chunk, sizeof(chunk));
        if (n < 0) {
            pulse_vfs_close(file);
            return std::optional<std::pmr::vector<uint8_t>>{};
        }
        if (n == 0) {
            break; // EOF
        }
        buffer.insert(buffer.end(), chunk, chunk + n);
    }
    pulse_vfs_close(file);
    return std::move(buffer);
}

} // namespace pulse::asset
