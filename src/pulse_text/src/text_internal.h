#pragma once

#include "pulse_text.h"

#include <cstdint>
#include <vector>

namespace pulse_text_internal {

struct text_block_slot {
    PulseTextBlockDesc desc{};
    bool alive = false;
};

struct pulse_text_plugin_state {
    PulseAppId app = nullptr;
    std::vector<text_block_slot> blocks;
    std::vector<uint32_t> free_blocks;
};

struct pulse_text_state_resource {
    pulse_text_plugin_state* state;
};

extern ECS_COMPONENT_DECLARE(pulse_text_state_resource);

pulse_text_plugin_state* state_from_app(PulseAppId app);

EPulsePluginBuildResult pulse_text_plugin_build(PulseAppId app, void* ctx);
void pulse_text_plugin_shutdown(PulseAppId app, void* ctx);

const PulseTextBlockDesc* require_block(pulse_text_plugin_state* state, uint32_t block);

PulseTextMeasure text_measure(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width);
PulseTextLayout* text_layout(PulseAppId app, const PulseTextBlockDesc* desc, const char* text, float box_width, float box_height);

}
