#pragma once

#ifndef PULSE_FONT_API_HEADER_GUARD
#define PULSE_FONT_API_HEADER_GUARD
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

#include <stdbool.h>
#include <stdint.h>
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_FONT_MODULE_BUILD)
#  define PULSE_FONT_API PULSE_EXPORT
#else
#  define PULSE_FONT_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_FONT_PLUGIN_DESC_VERSION 1u

/**
 * Font handle 0 表示无效
 *
 */
#define PULSE_FONT_ID_NONE 0u

/**
 * 注册字体上限，含 fallback 兜底字体，超出即失败
 *
 */
#define PULSE_FONT_MAX_COUNT 64u










typedef struct PulseFontPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    uint32_t             atlas_width;
    uint32_t             atlas_height;
    uint32_t             max_atlas_count;
    uint32_t             sdf_padding;
    int32_t              record_priority;

} PulseFontPluginDesc;

/**
 * 垂直度量，单位为目标字号的像素
 *
 */
typedef struct PulseVerticalMetrics
{
    float                ascent;
    float                descent;
    float                line_gap;
    float                height;

} PulseVerticalMetrics;

/**
 * 字形在 SDF 图集中的位置与摆放信息，y 轴向下，相对笔尖基线原点
 *
 */
typedef struct PulseGlyph
{
    bool                 valid;
    uint32_t             page;
    float                x0;
    float                y0;
    float                x1;
    float                y1;
    float                u0;
    float                v0;
    float                u1;
    float                v1;
    float                advance;

} PulseGlyph;

/**
 * 图集运行状态
 *
 */
typedef struct PulseAtlasStats
{
    uint32_t             page_count;
    uint32_t             glyph_count;
    uint32_t             slot_count;
    uint64_t             eviction_count;
    uint64_t             rasterize_count;

} PulseAtlasStats;

/**
 * 一条 glyph 的绘制数据，page 相同的实例会被合进同一次 draw
 *
 */
typedef struct PulseGlyphInstance
{
    float                x;
    float                y;
    float                width;
    float                height;
    float                u0;
    float                v0;
    float                u1;
    float                v1;
    float                r;
    float                g;
    float                b;
    float                a;
    uint32_t             page;

} PulseGlyphInstance;

/**
 * 局部像素坐标到 NDC 的仿射变换
 *
 */
typedef struct PulseTransform
{
    float                scale_x;
    float                scale_y;
    float                translate_x;
    float                translate_y;

} PulseTransform;

/**
 * 裁剪矩形，与实例同处局部像素坐标
 *
 */
typedef struct PulseScissor
{
    float                x;
    float                y;
    float                width;
    float                height;
    bool                 enabled;

} PulseScissor;

typedef struct PulseDrawDesc
{
    Pulse_Array(const PulseGlyphInstance, instances);
    PulseTransform       transform;
    PulseScissor         scissor;

} PulseDrawDesc;






PULSE_FONT_API PulseFontPluginDesc pulse_font_plugin_desc_default(void);
PULSE_FONT_API EPulseAppAddPluginResult pulse_add_font_plugin(PulseAppId app, const PulseFontPluginDesc* desc);

/**
 * 从内存字节流注册字体，TTC 按 faceIndex 展开，返回字体 id
 *
 * @param[in] app
 * @param[in] memory
 * @param[in] faceIndex
 *
 */
PULSE_FONT_API uint32_t pulse_font_register(PulseAppId app, Pulse_Blob_Param(memory), uint32_t face_index);

/**
 * 从 VFS 路径注册字体
 *
 * @param[in] app
 * @param[in] path
 * @param[in] faceIndex
 *
 */
PULSE_FONT_API uint32_t pulse_font_register_file(PulseAppId app, const char* path, uint32_t face_index);

/**
 * 字节流里的 face 数量
 *
 * @param[in] app
 * @param[in] memory
 *
 */
PULSE_FONT_API uint32_t pulse_font_face_count(PulseAppId app, Pulse_Blob_Param(memory));
PULSE_FONT_API uint32_t pulse_font_count(PulseAppId app);
[[pulse::optional]] PULSE_FONT_API const char* pulse_font_family_name(PulseAppId app, uint32_t font);
PULSE_FONT_API uint32_t pulse_font_find_family(PulseAppId app, const char* family);

/**
 * 按顺序尝试的字体链，排版层只认链
 *
 * @param[in] app
 * @param[in] fonts
 *
 */
PULSE_FONT_API uint32_t pulse_font_create_chain(PulseAppId app, Pulse_Array_Param(const uint32_t, fonts));
PULSE_FONT_API void pulse_font_destroy_chain(PulseAppId app, uint32_t chain);

/**
 * 返回链上提供该 codepoint 的字体 id，全缺返回 0
 *
 * @param[in] app
 * @param[in] chain
 * @param[in] codepoint
 *
 */
PULSE_FONT_API uint32_t pulse_font_resolve_codepoint(PulseAppId app, uint32_t chain, uint32_t codepoint);
PULSE_FONT_API float pulse_font_advance(PulseAppId app, uint32_t chain, uint32_t codepoint, float size);
PULSE_FONT_API float pulse_font_kerning(PulseAppId app, uint32_t chain, uint32_t first, uint32_t second, float size);
PULSE_FONT_API PulseVerticalMetrics pulse_font_vertical_metrics(PulseAppId app, uint32_t chain, float size);

/**
 * 取字形并确保其已进图集
 *
 * @param[in] app
 * @param[in] chain
 * @param[in] codepoint
 * @param[in] size
 *
 */
PULSE_FONT_API PulseGlyph pulse_font_glyph(PulseAppId app, uint32_t chain, uint32_t codepoint, float size);

/**
 * 批量光栅化一段 UTF-8 文本用到的字符
 *
 * @param[in] app
 * @param[in] chain
 * @param[in] text
 * @param[in] size
 *
 */
PULSE_FONT_API void pulse_font_prewarm(PulseAppId app, uint32_t chain, const char* text, float size);
PULSE_FONT_API PulseAtlasStats pulse_font_atlas_stats(PulseAppId app);

/**
 * 调试用：读取图集页单个像素
 *
 * @param[in] app
 * @param[in] page
 * @param[in] x
 * @param[in] y
 *
 */
PULSE_FONT_API uint8_t pulse_font_atlas_sample(PulseAppId app, uint32_t page, uint32_t x, uint32_t y);

/**
 * 本帧提交一批 glyph 实例，按 page 分组各一次 draw
 *
 * @param[in] app
 * @param[in] desc
 *
 */
PULSE_FONT_API EPulseResult pulse_font_submit(PulseAppId app, const PulseDrawDesc* desc);

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
#endif // PULSE_FONT_API_HEADER_GUARD
