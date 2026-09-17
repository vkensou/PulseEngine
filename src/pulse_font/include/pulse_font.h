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

#include "pulse_asset.h"

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

/**
 * Asset type id range 0x4000+ belongs to pulse_font (graphics 0x1000+, prefab 0x2000, datatable 0x3000)
 *
 */
#define PULSE_TYPE_FONT UINT64_C(0x4000)










/**
 * 字体资产凭证，index/generation 即 asset handle 坐标，{0,0} 表示无效
 *
 */
typedef struct PulseFontHandle
{
    uint32_t             index;
    uint32_t             generation;

} PulseFontHandle;

/**
 * 字体加载请求
 *
 */
typedef struct PulseFontRequest
{
    uint32_t             index;
    uint32_t             generation;

} PulseFontRequest;

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
 * 图集页 CPU 像素只读视图，R8，长度 = plugin desc 的 atlas_width × atlas_height
 *
 */
typedef struct PulseFontPagePixels
{
    Pulse_Blob(pixels);

} PulseFontPagePixels;

/**
 * ===== 渲染出口区块 =====
 * 本区块的类型与函数整体属于文字渲染出口，将来从 pulse_font 整体迁出到独立渲染包；
 * GPU 侧跨包消费图集走上方 FontPageCount / FontPagePixels / FontPageVersion。
 * FontPluginDesc 的 recordPriority 亦属本区块，受结构体约束留在原处。
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






// ---- inline helpers for asset handle types ----

PULSE_DEFINE_ASSET_CONVERSIONS(font, PULSE_TYPE_FONT, PulseFontHandle, PulseFontRequest)

PULSE_FONT_API PulseFontPluginDesc pulse_font_plugin_desc_default(void);
PULSE_FONT_API EPulseAppAddPluginResult pulse_add_font_plugin(PulseAppId app, const PulseFontPluginDesc* desc);

/**
 * 发起异步字体加载（内部走 pulse_asset），支持 ttf/otf/ttc 与 BMFont 文本 .fnt（单页，PNG 与 .fnt 同目录）；is_ready 后用 get_handle 取字体凭证
 *
 * @param[in] app
 * @param[in] path
 * @param[in] faceIndex
 *
 */
PULSE_FONT_API PulseFontRequest pulse_font_load(PulseAppId app, const char* path, uint32_t face_index);

/**
 * 从内存字节流发起异步字体加载，name 用于扩展名判定（如 "latin.ttf"、"ascii.fnt"，fnt 的页面 PNG 仍从 VFS 同目录读取）
 *
 * @param[in] app
 * @param[in] name
 * @param[in] memory
 * @param[in] faceIndex
 *
 */
PULSE_FONT_API PulseFontRequest pulse_font_load_from_memory(PulseAppId app, const char* name, Pulse_Blob_Param(memory), uint32_t face_index);

/**
 * 加载是否完成（asset 状态为 LOADED）
 *
 * @param[in] app
 * @param[in] request
 *
 */
PULSE_FONT_API bool pulse_font_is_ready(PulseAppId app, PulseFontRequest request);

/**
 * 加载是否仍在进行或资产仍存活
 *
 * @param[in] app
 * @param[in] request
 *
 */
PULSE_FONT_API bool pulse_font_is_alive(PulseAppId app, PulseFontRequest request);

/**
 * 加载失败原因，无错误返回 null
 *
 * @param[in] app
 * @param[in] request
 *
 */
[[pulse::optional]] PULSE_FONT_API const char* pulse_font_get_error(PulseAppId app, PulseFontRequest request);

/**
 * ready 后取字体凭证，同时登记进字体注册表；未 ready 返回无效 handle；同一资产重复调用幂等
 *
 * @param[in] app
 * @param[in] request
 *
 */
PULSE_FONT_API PulseFontHandle pulse_font_get_handle(PulseAppId app, PulseFontRequest request);
PULSE_FONT_API uint32_t pulse_font_count(PulseAppId app);
[[pulse::optional]] PULSE_FONT_API const char* pulse_font_family_name(PulseAppId app, PulseFontHandle font);
PULSE_FONT_API PulseFontHandle pulse_font_find_family(PulseAppId app, const char* family);

/**
 * 内置默认 bitmap 字体凭证，asset 系统缺失或未构建成功时返回无效 handle
 *
 * @param[in] app
 *
 */
PULSE_FONT_API PulseFontHandle pulse_font_default(PulseAppId app);

/**
 * 按顺序尝试的字体链，排版层只认链；链尾自动追加默认字体（已在列表中则不重复追加）
 *
 * @param[in] app
 * @param[in] fonts
 *
 */
PULSE_FONT_API uint32_t pulse_font_create_chain(PulseAppId app, Pulse_Array_Param(const PulseFontHandle, fonts));
PULSE_FONT_API void pulse_font_destroy_chain(PulseAppId app, uint32_t chain);

/**
 * 返回链上提供该 codepoint 的字体凭证，全缺返回无效 handle
 *
 * @param[in] app
 * @param[in] chain
 * @param[in] codepoint
 *
 */
PULSE_FONT_API PulseFontHandle pulse_font_resolve_codepoint(PulseAppId app, uint32_t chain, uint32_t codepoint);
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
 * 当前图集页数
 *
 * @param[in] app
 *
 */
PULSE_FONT_API uint32_t pulse_font_page_count(PulseAppId app);

/**
 * 图集页 CPU 像素，借用插件内存不拷贝，页不存在返回空视图
 *
 * @param[in] app
 * @param[in] page
 *
 */
PULSE_FONT_API PulseFontPagePixels pulse_font_page_pixels(PulseAppId app, uint32_t page);

/**
 * 页内容版本号，页创建与该页每次像素写入（光栅化、淘汰重置）各加一
 *
 * @param[in] app
 * @param[in] page
 *
 */
PULSE_FONT_API uint64_t pulse_font_page_version(PulseAppId app, uint32_t page);

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
