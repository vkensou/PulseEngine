#pragma once

#ifndef PULSE_TEXT_RENDER_API_HEADER_GUARD
#define PULSE_TEXT_RENDER_API_HEADER_GUARD
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

#include <stdint.h>
#include "pulse_platform.h"
#include "pulse_app.h"
#include "pulse_text.h"

#if defined(PULSE_TEXT_RENDER_MODULE_BUILD)
#  define PULSE_TEXT_RENDER_API PULSE_EXPORT
#else
#  define PULSE_TEXT_RENDER_API PULSE_IMPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Constants
 *
 */
#define PULSE_TEXT_RENDER_PLUGIN_DESC_VERSION 1u

/**
 * 单个实体内联文本缓冲上限，含结尾 '\0'
 *
 */
#define PULSE_TEXT_MAX_CHARS 256u












/**
 * 文字渲染组件；block 为 pulse_text 排版块 id，text 为内联 utf8（meta/预制数据里按 utf8 字符串读写）
 * scissor 为相对文字自身位置的裁剪矩形，与字形实例同处局部像素坐标（y 向下），
 * width/height 均大于 0 时生效，渲染后自动重置为全屏
 *
 */
typedef struct PulseText
{
    PulseTextBlockDesc   block;
    char                 text[PULSE_TEXT_MAX_CHARS];
    float                box_width;
    float                box_height;
    float                scissor_x;
    float                scissor_y;
    float                scissor_width;
    float                scissor_height;
    int32_t              sorting_order;
    uint32_t             revision;

} PulseText;
PULSE_TEXT_RENDER_API extern ECS_COMPONENT_DECLARE(PulseText);


PULSE_TEXT_RENDER_API EPulseAppAddPluginResult pulse_add_text_render_plugin(PulseAppId app);

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
#endif // PULSE_TEXT_RENDER_API_HEADER_GUARD
