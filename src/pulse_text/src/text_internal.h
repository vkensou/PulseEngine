#pragma once

#include "pulse_text.h"

namespace pulse_text_internal {

struct pulse_text_plugin_state {
    PulseAppId app = nullptr;
};

struct pulse_text_state_resource {
    pulse_text_plugin_state* state;
};

extern ECS_COMPONENT_DECLARE(pulse_text_state_resource);

EPulsePluginBuildResult pulse_text_plugin_build(PulseAppId app, void* ctx);
void pulse_text_plugin_shutdown(PulseAppId app, void* ctx);

PulseTextMeasure text_measure(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width);
PulseTextLayout* text_layout(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width, float box_height);

}
