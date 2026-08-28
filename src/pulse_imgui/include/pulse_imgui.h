#pragma once

#ifndef PULSE_IMGUI_API_HEADER_GUARD
#define PULSE_IMGUI_API_HEADER_GUARD
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h> // bool
#include <stddef.h>  // size_t
#include <stdint.h>  // uint32_t
#include "pulse_platform.h"
#include "pulse_app.h"

#if defined(PULSE_IMGUI_MODULE_BUILD)
#  define PULSE_IMGUI_API PULSE_EXPORT
#else
#  define PULSE_IMGUI_API PULSE_IMPORT
#endif

#define PULSE_IMGUI_PLUGIN_DESC_VERSION 1u


typedef uint32_t EPulseFlags;
typedef uint64_t EPulseFlags64;
typedef struct ImGuiContext ImGuiContext;



/**
 * ImGui plugin flag bits (PulseImguiPluginFlags = EPulseFlags)
 *
 */
typedef enum EPulseImguiPluginFlagBits
{
    PULSE_IMGUI_PLUGIN_ENABLE_DOCKING = 0x00000001,        /** ( 0)  //!< Enable docking layout    */
    PULSE_IMGUI_PLUGIN_DEFAULT = PULSE_IMGUI_PLUGIN_ENABLE_DOCKING,

} EPulseImguiPluginFlagBits;
typedef EPulseFlags EPulseImguiPluginFlags;






typedef struct PulseImguiPluginDesc
{
    uint32_t             struct_size;
    uint32_t             version;
    EPulseImguiPluginFlags flags;
    const char*          ini_filename;       /** NULL -> default "imgui.ini"              */

} PulseImguiPluginDesc;

typedef struct PulseImguiContext
{
    ImGuiContext*        context;

} PulseImguiContext;


// ECS declarations
PULSE_IMGUI_API extern ECS_COMPONENT_DECLARE(PulseImguiContext);

PULSE_IMGUI_API PulseImguiPluginDesc pulse_imgui_plugin_desc_default(void);
PULSE_IMGUI_API EPulseAppAddPluginResult pulse_add_imgui_plugin(PulseAppId app, const PulseImguiPluginDesc* desc);
PULSE_IMGUI_API [[pulse::optional]] ImGuiContext* pulse_imgui_get_context(PulseAppId app);
PULSE_IMGUI_API ecs_entity_t pulse_imgui_get_phase(PulseAppId app);

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
#endif // PULSE_IMGUI_API_HEADER_GUARD
