#include "font_internal.h"

#include "pulse_vfs.h"

#define STBI_NO_JPEG
#define STBI_NO_BMP
#define STBI_NO_PSD
#define STBI_NO_GIF
#define STBI_NO_PIC
#define STBI_NO_PNM
#define STBI_NO_TGA
#define STBI_NO_HDR
#define STBI_NO_LINEAR
#define STBI_NO_STDIO
#define STBI_NO_FAILURE_STRINGS
#define STBI_NO_THREAD_LOCALS
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace pulse_font_internal {

namespace {

struct attr_pair {
    std::string key;
    std::string value;
};

bool read_word(const std::string& line, size_t& cursor, std::string& out) {
    while (cursor < line.size() && std::isspace((unsigned char)line[cursor])) {
        ++cursor;
    }
    if (cursor >= line.size()) {
        return false;
    }
    const size_t start = cursor;
    while (cursor < line.size() && line[cursor] != '=' && !std::isspace((unsigned char)line[cursor])) {
        ++cursor;
    }
    out = line.substr(start, cursor - start);
    return !out.empty();
}

bool read_value(const std::string& line, size_t& cursor, std::string& out) {
    while (cursor < line.size() && std::isspace((unsigned char)line[cursor])) {
        ++cursor;
    }
    if (cursor >= line.size() || line[cursor] != '=') {
        return false;
    }
    ++cursor;
    while (cursor < line.size() && std::isspace((unsigned char)line[cursor])) {
        ++cursor;
    }
    if (cursor >= line.size()) {
        return false;
    }
    if (line[cursor] == '"') {
        ++cursor;
        const size_t start = cursor;
        while (cursor < line.size() && line[cursor] != '"') {
            ++cursor;
        }
        out = line.substr(start, cursor - start);
        if (cursor < line.size()) {
            ++cursor;
        }
        return true;
    }
    const size_t start = cursor;
    while (cursor < line.size() && !std::isspace((unsigned char)line[cursor])) {
        ++cursor;
    }
    out = line.substr(start, cursor - start);
    return true;
}

std::vector<attr_pair> parse_attrs(const std::string& line) {
    std::vector<attr_pair> attrs;
    size_t cursor = 0;
    std::string word;
    while (read_word(line, cursor, word)) {
        std::string value;
        if (read_value(line, cursor, value)) {
            attrs.push_back(attr_pair{ std::move(word), std::move(value) });
        }
    }
    return attrs;
}

const std::string* find_attr(const std::vector<attr_pair>& attrs, const char* key) {
    for (const attr_pair& attr : attrs) {
        if (attr.key == key) {
            return &attr.value;
        }
    }
    return nullptr;
}

bool attr_int(const std::vector<attr_pair>& attrs, const char* key, int32_t& out) {
    const std::string* value = find_attr(attrs, key);
    if (!value || value->empty()) {
        return false;
    }
    char* end = nullptr;
    const long long parsed = std::strtoll(value->c_str(), &end, 10);
    if (!end || *end != '\0' || parsed < INT32_MIN || parsed > INT32_MAX) {
        return false;
    }
    out = (int32_t)parsed;
    return true;
}

bool line_keyword(const std::string& line, const char* keyword) {
    size_t cursor = 0;
    std::string word;
    return read_word(line, cursor, word) && word == keyword;
}

std::string next_line(const char*& cursor, const char* end) {
    const char* start = cursor;
    while (cursor < end && *cursor != '\n') {
        ++cursor;
    }
    std::string line(start, (size_t)(cursor - start));
    if (cursor < end) {
        ++cursor;
    }
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    return line;
}

uint64_t kerning_pair_key(uint32_t first, uint32_t second) {
    return ((uint64_t)first << 32) | (uint64_t)second;
}

}

const bitmap_glyph* bitmap_font_find_glyph(const bitmap_font_data& data, uint32_t codepoint) {
    auto it = std::lower_bound(data.codepoints.begin(), data.codepoints.end(), codepoint);
    if (it == data.codepoints.end() || *it != codepoint) {
        return nullptr;
    }
    return &data.glyphs[(size_t)(it - data.codepoints.begin())];
}

int32_t bitmap_font_kerning(const bitmap_font_data& data, uint32_t first, uint32_t second) {
    const uint64_t key = kerning_pair_key(first, second);
    auto it = std::lower_bound(data.kernings.begin(), data.kernings.end(), key, [](const bitmap_kerning& item, uint64_t value) {
        return item.pair < value;
    });
    if (it == data.kernings.end() || it->pair != key) {
        return 0;
    }
    return it->amount;
}

bool bitmap_font_parse(const uint8_t* fnt, size_t fnt_size, bitmap_font_parse_result& out, const char** out_error) {
    *out_error = nullptr;
    if (!fnt || fnt_size < 4) {
        *out_error = "bitmap font: empty or truncated .fnt";
        return false;
    }
    if (fnt[0] == 'B' && fnt[1] == 'M') {
        *out_error = "bitmap font: binary .fnt is not supported";
        return false;
    }
    const char* cursor = (const char*)fnt;
    const char* end = cursor + fnt_size;
    bitmap_font_data& data = out.data;
    bool has_common = false;
    bool has_page = false;
    bool has_chars = false;
    int32_t pages = 0;
    while (cursor < end) {
        const std::string line = next_line(cursor, end);
        if (line.empty()) {
            continue;
        }
        const std::vector<attr_pair> attrs = parse_attrs(line);
        if (line_keyword(line, "info")) {
            const std::string* face = find_attr(attrs, "face");
            if (!face) {
                *out_error = "bitmap font: info line misses face";
                return false;
            }
            data.family = *face;
            continue;
        }
        if (line_keyword(line, "common")) {
            int32_t line_height = 0;
            int32_t base = 0;
            int32_t scale_w = 0;
            int32_t scale_h = 0;
            int32_t packed = 0;
            if (!attr_int(attrs, "lineHeight", line_height) || !attr_int(attrs, "base", base) || !attr_int(attrs, "scaleW", scale_w) || !attr_int(attrs, "scaleH", scale_h) || !attr_int(attrs, "pages", pages) || !attr_int(attrs, "packed", packed)) {
                *out_error = "bitmap font: malformed common line";
                return false;
            }
            if (line_height <= 0 || base < 0 || scale_w <= 0 || scale_h <= 0) {
                *out_error = "bitmap font: invalid common metrics";
                return false;
            }
            if (pages != 1 || packed != 0) {
                *out_error = "bitmap font: only single-page unpacked .fnt is supported";
                return false;
            }
            data.line_height = (uint32_t)line_height;
            data.base = base;
            data.scale_width = (uint32_t)scale_w;
            data.scale_height = (uint32_t)scale_h;
            has_common = true;
            continue;
        }
        if (line_keyword(line, "page")) {
            int32_t id = 0;
            const std::string* file = find_attr(attrs, "file");
            if (!attr_int(attrs, "id", id) || !file) {
                *out_error = "bitmap font: malformed page line";
                return false;
            }
            if (has_page) {
                *out_error = "bitmap font: multiple page lines are not supported";
                return false;
            }
            if (id != 0) {
                *out_error = "bitmap font: page id must be 0";
                return false;
            }
            out.page_file = *file;
            has_page = true;
            continue;
        }
        if (line_keyword(line, "chars")) {
            int32_t count = 0;
            if (!attr_int(attrs, "count", count) || count <= 0) {
                *out_error = "bitmap font: malformed chars line";
                return false;
            }
            data.codepoints.reserve((size_t)count);
            data.glyphs.reserve((size_t)count);
            has_chars = true;
            continue;
        }
        if (line_keyword(line, "char")) {
            if (!has_chars) {
                *out_error = "bitmap font: char line without chars section";
                return false;
            }
            bitmap_glyph glyph{};
            int32_t codepoint = 0;
            int32_t page = 0;
            int32_t channel = 0;
            if (!attr_int(attrs, "id", codepoint) || !attr_int(attrs, "x", glyph.x) || !attr_int(attrs, "y", glyph.y) || !attr_int(attrs, "width", glyph.width) || !attr_int(attrs, "height", glyph.height) || !attr_int(attrs, "xoffset", glyph.xoffset) || !attr_int(attrs, "yoffset", glyph.yoffset) || !attr_int(attrs, "xadvance", glyph.xadvance) || !attr_int(attrs, "page", page) || !attr_int(attrs, "chnl", channel)) {
                *out_error = "bitmap font: malformed char line";
                return false;
            }
            if (codepoint < 0 || page != 0) {
                *out_error = "bitmap font: char references unsupported page";
                return false;
            }
            if (channel != 15 && channel != 1) {
                *out_error = "bitmap font: unsupported char channel mask";
                return false;
            }
            const uint32_t cp = (uint32_t)codepoint;
            if (bitmap_font_find_glyph(data, cp) != nullptr) {
                *out_error = "bitmap font: duplicated char id";
                return false;
            }
            data.codepoints.push_back(cp);
            data.glyphs.push_back(glyph);
            continue;
        }
        if (line_keyword(line, "kernings")) {
            int32_t count = 0;
            if (!attr_int(attrs, "count", count) || count < 0) {
                *out_error = "bitmap font: malformed kernings line";
                return false;
            }
            data.kernings.reserve((size_t)count);
            continue;
        }
        if (line_keyword(line, "kerning")) {
            int32_t first = 0;
            int32_t second = 0;
            int32_t amount = 0;
            if (!attr_int(attrs, "first", first) || !attr_int(attrs, "second", second) || !attr_int(attrs, "amount", amount)) {
                *out_error = "bitmap font: malformed kerning line";
                return false;
            }
            data.kernings.push_back(bitmap_kerning{ kerning_pair_key((uint32_t)first, (uint32_t)second), amount });
            continue;
        }
    }
    if (!has_common || !has_page || !has_chars || out.page_file.empty()) {
        *out_error = "bitmap font: missing required .fnt sections";
        return false;
    }
    std::vector<uint32_t> order(data.codepoints.size());
    for (uint32_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::sort(order.begin(), order.end(), [&](uint32_t a, uint32_t b) {
        return data.codepoints[a] < data.codepoints[b];
    });
    std::vector<uint32_t> sorted_codepoints(data.codepoints.size());
    std::vector<bitmap_glyph> sorted_glyphs(data.glyphs.size());
    for (uint32_t i = 0; i < order.size(); ++i) {
        sorted_codepoints[i] = data.codepoints[order[i]];
        sorted_glyphs[i] = data.glyphs[order[i]];
    }
    data.codepoints = std::move(sorted_codepoints);
    data.glyphs = std::move(sorted_glyphs);
    std::sort(data.kernings.begin(), data.kernings.end(), [](const bitmap_kerning& a, const bitmap_kerning& b) {
        return a.pair < b.pair;
    });
    return true;
}

bool bitmap_font_attach_page(bitmap_font_data& data, const uint8_t* png, size_t png_size, const char** out_error) {
    *out_error = nullptr;
    if (!png || png_size == 0) {
        *out_error = "bitmap font: missing page texture";
        return false;
    }
    int32_t width = 0;
    int32_t height = 0;
    int32_t channels = 0;
    uint8_t* decoded = stbi_load_from_memory(png, (int)png_size, &width, &height, &channels, 0);
    if (!decoded) {
        *out_error = "bitmap font: failed to decode page texture";
        return false;
    }
    if ((uint32_t)width != data.scale_width || (uint32_t)height != data.scale_height) {
        stbi_image_free(decoded);
        *out_error = "bitmap font: page texture size mismatch";
        return false;
    }
    const size_t pixel_count = (size_t)width * (size_t)height;
    data.pixels.resize(pixel_count);
    if (channels == 1) {
        std::memcpy(data.pixels.data(), decoded, pixel_count);
    } else if (channels == 4) {
        for (size_t i = 0; i < pixel_count; ++i) {
            data.pixels[i] = decoded[i * 4 + 3];
        }
    } else {
        stbi_image_free(decoded);
        *out_error = "bitmap font: page texture must be grayscale or RGBA";
        return false;
    }
    stbi_image_free(decoded);
    return true;
}

bool bitmap_font_read_file(const char* path, std::vector<uint8_t>& out) {
    PulseVfsFileId file = pulse_vfs_open_read(path);
    if (!file) {
        return false;
    }
    const int64_t length = pulse_vfs_file_length(file);
    if (length <= 0) {
        pulse_vfs_close(file);
        return false;
    }
    out.resize((size_t)length);
    int64_t read = 0;
    while (read < length) {
        const int64_t n = pulse_vfs_read_bytes(file, out.data() + read, (uint64_t)(length - read));
        if (n <= 0) {
            break;
        }
        read += n;
    }
    pulse_vfs_close(file);
    if (read != length) {
        out.clear();
        return false;
    }
    return true;
}

std::string bitmap_font_page_path(const char* fnt_path, const std::string& page_file) {
    std::string result;
    if (fnt_path) {
        const char* slash = std::strrchr(fnt_path, '/');
        if (!slash) {
            slash = std::strrchr(fnt_path, '\\');
        }
        if (slash) {
            result.assign(fnt_path, (size_t)(slash - fnt_path + 1));
        }
    }
    size_t start = 0;
    while (page_file.compare(start, 2, "./") == 0) {
        start += 2;
    }
    result.append(page_file, start, std::string::npos);
    return result;
}

}