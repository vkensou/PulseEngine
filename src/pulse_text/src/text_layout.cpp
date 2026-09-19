#include "pulse_text.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace pulse_text_internal {

namespace {

constexpr uint32_t kNoBreak = 0xFFFFFFFFu;
constexpr float kEpsilon = 1e-3f;

struct text_item {
    uint32_t cp = 0;
    PulseTextColor color{};
};

struct text_line {
    uint32_t first = 0;
    uint32_t count = 0;
    float width = 0.0f;
};

struct text_runs {
    std::vector<text_item> items;
    std::vector<float> advances;
    std::vector<float> kerns;
};

bool is_space(uint32_t cp) {
    return cp == ' ' || cp == 0x3000;
}

bool is_cjk(uint32_t cp) {
    return (cp >= 0x1100 && cp <= 0x115F) ||
        (cp >= 0x2E80 && cp <= 0x303E) ||
        (cp >= 0x3041 && cp <= 0x33FF) ||
        (cp >= 0x3400 && cp <= 0x4DBF) ||
        (cp >= 0x4E00 && cp <= 0x9FFF) ||
        (cp >= 0xA000 && cp <= 0xA4CF) ||
        (cp >= 0xAC00 && cp <= 0xD7A3) ||
        (cp >= 0xF900 && cp <= 0xFAFF) ||
        (cp >= 0xFE30 && cp <= 0xFE4F) ||
        (cp >= 0xFF00 && cp <= 0xFF60) ||
        (cp >= 0xFFE0 && cp <= 0xFFE6) ||
        cp > 0xFFFF;
}

uint32_t hex_digit(uint8_t c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return 0xFF;
}

uint32_t decode_cp(const uint8_t* s, uint32_t& width) {
    uint32_t code = s[0];
    uint32_t extra = 0;
    if (code >= 0xF0) {
        code &= 0x07;
        extra = 3;
    } else if (code >= 0xE0) {
        code &= 0x0F;
        extra = 2;
    } else if (code >= 0xC0) {
        code &= 0x1F;
        extra = 1;
    }
    width = 1;
    for (uint32_t i = 0; i < extra && (s[width] & 0xC0) == 0x80; ++i) {
        code = (code << 6) | (s[width] & 0x3F);
        ++width;
    }
    return code;
}

void parse_text(const char* text, const PulseTextColor& def, std::vector<text_item>& out) {
    out.clear();
    if (!text) {
        return;
    }
    PulseTextColor cur = def;
    const uint8_t* cursor = reinterpret_cast<const uint8_t*>(text);
    while (*cursor) {
        if (*cursor == '[') {
            const uint8_t* close = cursor + 1;
            while (*close && *close != ']' && *close != '[') {
                ++close;
            }
            const uint32_t body_len = (uint32_t)(close - cursor - 1);
            if (*close == ']' && body_len == 1 && cursor[1] == '/') {
                cur = def;
                cursor = close + 1;
                continue;
            }
            if (*close == ']' && body_len == 6) {
                uint32_t digits[6];
                bool matched = true;
                for (uint32_t i = 0; i < 6; ++i) {
                    digits[i] = hex_digit(cursor[1 + i]);
                    matched = matched && digits[i] != 0xFF;
                }
                if (matched) {
                    const float to_unit = 1.0f / 255.0f;
                    cur.r = ((float)digits[0] * 16.0f + (float)digits[1]) * to_unit;
                    cur.g = ((float)digits[2] * 16.0f + (float)digits[3]) * to_unit;
                    cur.b = ((float)digits[4] * 16.0f + (float)digits[5]) * to_unit;
                    cur.a = def.a;
                    cursor = close + 1;
                    continue;
                }
            }
            text_item item{};
            item.cp = '[';
            item.color = cur;
            out.push_back(item);
            ++cursor;
            continue;
        }
        uint32_t cp_width = 1;
        const uint32_t cp = decode_cp(cursor, cp_width);
        cursor += cp_width;
        if (cp == '\r' || (cp != '\n' && cp < 0x20)) {
            continue;
        }
        text_item item{};
        item.cp = cp;
        item.color = cur;
        out.push_back(item);
    }
}

void build_runs(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, text_runs& runs) {
    parse_text(text, desc->color, runs.items);
    const uint32_t count = (uint32_t)runs.items.size();
    runs.advances.resize(count);
    runs.kerns.assign(count, 0.0f);
    for (uint32_t i = 0; i < count; ++i) {
        runs.advances[i] = pulse_font_advance(app, desc->chain, runs.items[i].cp, desc->size);
    }
    for (uint32_t i = 1; i < count; ++i) {
        runs.kerns[i] = pulse_font_kerning(app, desc->chain, runs.items[i - 1].cp, runs.items[i].cp, desc->size);
    }
}

void break_lines(const text_runs& runs, float box_width, std::vector<text_line>& lines) {
    lines.clear();
    const uint32_t n = (uint32_t)runs.items.size();
    const bool wrap = box_width > 0.0f;
    uint32_t start = 0;
    uint32_t i = 0;
    float pen = 0.0f;
    float visible = 0.0f;
    uint32_t brk = kNoBreak;
    float brk_width = 0.0f;
    while (i < n) {
        if (runs.items[i].cp == '\n') {
            text_line line{};
            line.first = start;
            line.count = i - start;
            line.width = visible;
            lines.push_back(line);
            start = i + 1;
            i = start;
            pen = 0.0f;
            visible = 0.0f;
            brk = kNoBreak;
            continue;
        }
        const float add = runs.advances[i] + (i > start ? runs.kerns[i] : 0.0f);
        if (wrap && i > start && pen + add > box_width) {
            text_line line{};
            line.first = start;
            if (brk != kNoBreak && brk > start) {
                line.count = brk - start;
                line.width = brk_width;
                i = brk;
            } else {
                line.count = i - start;
                line.width = visible;
            }
            lines.push_back(line);
            start = i;
            pen = 0.0f;
            visible = 0.0f;
            brk = kNoBreak;
            continue;
        }
        pen += add;
        if (!is_space(runs.items[i].cp)) {
            visible = pen;
        }
        if (i + 1 < n && runs.items[i + 1].cp != '\n') {
            if (is_cjk(runs.items[i].cp)) {
                brk = i + 1;
                brk_width = pen;
            } else if (is_space(runs.items[i].cp) && !is_space(runs.items[i + 1].cp)) {
                brk = i + 1;
                brk_width = visible;
            }
        }
        ++i;
    }
    if (start < n) {
        text_line line{};
        line.first = start;
        line.count = n - start;
        line.width = visible;
        lines.push_back(line);
    } else if (n > 0 && runs.items[n - 1].cp == '\n') {
        text_line line{};
        line.first = n;
        line.count = 0;
        line.width = 0.0f;
        lines.push_back(line);
    }
}

float line_advance_for(const PulseTextBlockDesc* desc, const PulseVerticalMetrics& metrics) {
    const float natural = metrics.ascent - metrics.descent + metrics.line_gap;
    return desc->line_height > 0.0f ? natural * desc->line_height : natural;
}

float max_line_width(const std::vector<text_line>& lines) {
    float width = 0.0f;
    for (const text_line& line : lines) {
        width = std::max(width, line.width);
    }
    return width;
}

PulseTextLayout* make_layout(std::vector<PulseGlyphInstance>& instances, float width, float height, uint32_t line_count, bool out_of_box) {
    auto* layout = new PulseTextLayout();
    layout->width = width;
    layout->height = height;
    layout->line_count = line_count;
    layout->out_of_box = out_of_box;
    if (instances.empty()) {
        layout->p_instances = nullptr;
        layout->instances_count = 0;
        return layout;
    }
    auto* data = new PulseGlyphInstance[instances.size()];
    std::memcpy(data, instances.data(), instances.size() * sizeof(PulseGlyphInstance));
    layout->p_instances = data;
    layout->instances_count = instances.size();
    return layout;
}

}

PulseTextMeasure text_measure(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width) {
    PulseTextMeasure out{};
    text_runs runs{};
    build_runs(app, desc, text, runs);
    std::vector<text_line> lines;
    break_lines(runs, box_width, lines);
    if (lines.empty()) {
        return out;
    }
    const PulseVerticalMetrics metrics = pulse_font_vertical_metrics(app, desc->chain, desc->size);
    out.width = max_line_width(lines);
    out.height = line_advance_for(desc, metrics) * (float)lines.size();
    out.line_count = (uint32_t)lines.size();
    return out;
}

PulseTextLayout* text_layout(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width, float box_height) {
    text_runs runs{};
    build_runs(app, desc, text, runs);
    std::vector<text_line> lines;
    break_lines(runs, box_width, lines);
    std::vector<PulseGlyphInstance> instances;
    if (lines.empty()) {
        return make_layout(instances, 0.0f, 0.0f, 0, false);
    }
    const PulseVerticalMetrics metrics = pulse_font_vertical_metrics(app, desc->chain, desc->size);
    const float line_advance = line_advance_for(desc, metrics);
    const float total_height = line_advance * (float)lines.size();
    const bool out_of_box = box_height > 0.0f && line_advance > 0.0f && total_height > box_height + kEpsilon;
    uint32_t visible_lines = (uint32_t)lines.size();
    if (out_of_box) {
        visible_lines = (uint32_t)std::floor(box_height / line_advance + 1e-4f);
    }
    const float content_height = line_advance * (float)visible_lines;
    float y_top = 0.0f;
    if (box_height > 0.0f) {
        if (desc->align_v == PULSE_TEXT_ALIGN_V_MIDDLE) {
            y_top = (box_height - content_height) * 0.5f;
        } else if (desc->align_v == PULSE_TEXT_ALIGN_V_BOTTOM) {
            y_top = box_height - content_height;
        }
    }
    const float effective_width = box_width > 0.0f ? box_width : max_line_width(lines);
    for (uint32_t k = 0; k < visible_lines; ++k) {
        const text_line& line = lines[k];
        float x_off = 0.0f;
        if (desc->align_h == PULSE_TEXT_ALIGN_H_CENTER) {
            x_off = (effective_width - line.width) * 0.5f;
        } else if (desc->align_h == PULSE_TEXT_ALIGN_H_RIGHT) {
            x_off = effective_width - line.width;
        }
        const float baseline = y_top + metrics.ascent + (float)k * line_advance;
        float pen = 0.0f;
        const uint32_t end = line.first + line.count;
        for (uint32_t i = line.first; i < end; ++i) {
            const text_item& item = runs.items[i];
            if (!is_space(item.cp)) {
                const PulseGlyph glyph = pulse_font_glyph(app, desc->chain, item.cp, desc->size);
                if (glyph.valid) {
                    PulseGlyphInstance inst{};
                    inst.x = x_off + pen + glyph.x0;
                    inst.y = baseline + glyph.y0;
                    inst.width = glyph.x1 - glyph.x0;
                    inst.height = glyph.y1 - glyph.y0;
                    inst.u0 = glyph.u0;
                    inst.v0 = glyph.v0;
                    inst.u1 = glyph.u1;
                    inst.v1 = glyph.v1;
                    inst.r = item.color.r;
                    inst.g = item.color.g;
                    inst.b = item.color.b;
                    inst.a = item.color.a;
                    inst.page = glyph.page;
                    instances.push_back(inst);
                }
            }
            pen += runs.advances[i] + (i > line.first ? runs.kerns[i] : 0.0f);
        }
    }
    float width = 0.0f;
    for (uint32_t k = 0; k < visible_lines; ++k) {
        width = std::max(width, lines[k].width);
    }
    return make_layout(instances, width, content_height, visible_lines, out_of_box);
}

}
