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
#include "pulse_graphics.h"

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
 * Asset type id range 0x4000+ belongs to pulse_font (graphics 0x1000+, prefab 0x2000, datatable 0x3000)
 *
 */
#define PULSE_TYPE_FONT UINT64_C(0x4000)

#define PULSE_TYPE_FONT_CHAIN UINT64_C(0x4001)








// ---- asset type definitions ----

PULSE_DEFINE_ASSET_TYPE(font, PULSE_TYPE_FONT, PulseFontHandle, PulseFontRequest)
PULSE_DEFINE_ASSET_TYPE(font_chain, PULSE_TYPE_FONT_CHAIN, PulseFontChainHandle, PulseFontChainRequest)



typedef struct PulseFontPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    uint32_t             atlas_width;
    uint32_t             atlas_height;
    uint32_t             max_atlas_count;
    uint32_t             sdf_padding;

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
 * 排版结果里的一条 glyph 实例，坐标相对盒子原点、y 轴向下，page 为所属图集页
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
 * 内置默认 bitmap 字体凭证，asset 系统缺失或未构建成功时返回无效 handle
 *
 * @param[in] app
 *
 */
PULSE_FONT_API PulseFontHandle pulse_font_default(PulseAppId app);

/**
 * 按顺序尝试的字体链，排版层只认链；匿名运行时链，无序列化标识；链尾自动追加默认字体（已在列表中则不重复追加）；每次调用生成独立资产并各持一份引用
 *
 * @param[in] app
 * @param[in] fonts
 *
 */
PULSE_FONT_API PulseFontChainHandle pulse_font_create_chain(PulseAppId app, Pulse_Array_Param(const PulseFontHandle, fonts));

/**
 * 发起异步字体链加载
 *
 * @param[in] app
 * @param[in] path
 *
 */
PULSE_FONT_API PulseFontChainRequest pulse_font_load_chain(PulseAppId app, const char* path);
PULSE_FONT_API void pulse_font_destroy_chain(PulseAppId app, PulseFontChainHandle chain);
PULSE_FONT_API float pulse_font_advance(PulseAppId app, PulseFontChainHandle chain, uint32_t codepoint, float size);
PULSE_FONT_API float pulse_font_kerning(PulseAppId app, PulseFontChainHandle chain, uint32_t first, uint32_t second, float size);
PULSE_FONT_API PulseVerticalMetrics pulse_font_vertical_metrics(PulseAppId app, PulseFontChainHandle chain, float size);

/**
 * 取字形并确保其已进图集
 *
 * @param[in] app
 * @param[in] chain
 * @param[in] codepoint
 * @param[in] size
 *
 */
PULSE_FONT_API PulseGlyph pulse_font_glyph(PulseAppId app, PulseFontChainHandle chain, uint32_t codepoint, float size);

/**
 * 当前图集页数
 *
 * @param[in] app
 *
 */
PULSE_FONT_API uint32_t pulse_font_page_count(PulseAppId app);

/**
 * 图集失效代数：页淘汰或字体卸载使已分发字形的图集位置作废时加一；纯新增光栅化不加。缓存了 glyph UV 的消费方（排版结果）发现代数变化即需整体重排
 *
 * @param[in] app
 *
 */
PULSE_FONT_API uint64_t pulse_font_atlas_generation(PulseAppId app);

/**
 * 取图集页的 GPU 纹理凭证；page 必须小于 pulse_font_page_count，越界返回无效凭证；渲染器不存在（headless）时返回无效凭证；页存在时首次调用即同步创建纹理对象并返回稳定句柄，句柄在插件生命周期内不变，像素内容由插件延迟到渲染记录阶段上传（允许晚一帧）；创建失败视为致命错误，不会以无效凭证表达
 *
 * @param[in] app
 * @param[in] page
 *
 */
PULSE_FONT_API PulseTextureHandle pulse_font_page_texture(PulseAppId app, uint32_t page);

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
