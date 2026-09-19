#pragma once

#ifndef PULSE_TEXT_API_HEADER_GUARD
#define PULSE_TEXT_API_HEADER_GUARD
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

#include "pulse_font.h"

#if defined(PULSE_TEXT_MODULE_BUILD)
#  define PULSE_TEXT_API PULSE_EXPORT
#else
#  define PULSE_TEXT_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define PULSE_TEXT_PLUGIN_DESC_VERSION 1u


/**
 * 水平对齐
 *
 */
typedef enum EPulseTextAlignH
{
    PULSE_TEXT_ALIGN_H_LEFT,                  /** ( 0)                                */
    PULSE_TEXT_ALIGN_H_CENTER,                /** ( 1)                                */
    PULSE_TEXT_ALIGN_H_RIGHT,                 /** ( 2)                                */

    PULSE_TEXT_ALIGN_H_COUNT

} EPulseTextAlignH;

/**
 * 垂直对齐
 *
 */
typedef enum EPulseTextAlignV
{
    PULSE_TEXT_ALIGN_V_TOP,                   /** ( 0)                                */
    PULSE_TEXT_ALIGN_V_MIDDLE,                /** ( 1)                                */
    PULSE_TEXT_ALIGN_V_BOTTOM,                /** ( 2)                                */

    PULSE_TEXT_ALIGN_V_COUNT

} EPulseTextAlignV;








/**
 * RGBA 颜色，分量 0..1
 *
 */
typedef struct PulseTextColor
{
    float                r;
    float                g;
    float                b;
    float                a;

} PulseTextColor;

/**
 * 排版参数；按值传入排版接口，chain 为字体链 id，lineHeight 为行高系数（<=0 用字体自然行高），color 为默认文字颜色
 *
 */
typedef struct PulseTextBlockDesc
{
    uint32_t             chain;
    float                size;
    PulseTextColor       color;
    EPulseTextAlignH     align_h;
    EPulseTextAlignV     align_v;
    float                line_height;

} PulseTextBlockDesc;

/**
 * 排版结果；instances 矩形相对盒子原点、y 轴向下，调用方叠加 transform 后交给 pulse_font_submit
 *
 */
typedef struct PulseTextLayout
{
    Pulse_Array(const PulseGlyphInstance, instances);
    float                width;
    float                height;
    uint32_t             line_count;
    bool                 out_of_box;

} PulseTextLayout;

/**
 * 排版尺寸结果
 *
 */
typedef struct PulseTextMeasure
{
    float                width;
    float                height;
    uint32_t             line_count;

} PulseTextMeasure;






PULSE_TEXT_API EPulseAppAddPluginResult pulse_add_text_plugin(PulseAppId app);

/**
 * 在 boxWidth × boxHeight 的盒子里排版 utf8 文本，产出 glyph 实例流；boxWidth <= 0 不按宽度断行，boxHeight <= 0 不按高度截断；结果归调用方所有，用完调 pulse_text_layout_free 释放
 *
 * @param[in] app
 * @param[in] desc
 * @param[in] text
 * @param[in] boxWidth
 * @param[in] boxHeight
 *
 */
[[pulse::optional]] [[pulse::owner]] PULSE_TEXT_API PulseTextLayout* pulse_text_layout(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width, float box_height);

/**
 * 只算尺寸不产实例，与 layout 共用同一套断行实现；boxWidth <= 0 为自然尺寸
 *
 * @param[in] app
 * @param[in] desc
 * @param[in] text
 * @param[in] boxWidth
 *
 */
PULSE_TEXT_API PulseTextMeasure pulse_text_measure(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width);

/**
 * 释放 pulse_text_layout 的结果
 *
 * @param[in] app
 * @param[in] layout
 *
 */
PULSE_TEXT_API void pulse_text_layout_free(PulseAppId app, [[pulse::owner]] PulseTextLayout* layout);

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
#endif // PULSE_TEXT_API_HEADER_GUARD
