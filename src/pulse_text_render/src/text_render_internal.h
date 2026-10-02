#pragma once

#include "pulse_text_render.h"
#include "pulse_renderer.h"
#include "pulse_graphics.h"
#include "pulse_asset.h"
#include "pulse_transform.h"
#include "pulse_text.h"
#include "pulse_font.h"

#include <cstdint>
#include <cstring>
#include <vector>

namespace pulse_text_render_internal {

constexpr const char* kTextFeatureName = "Text";

struct text_draw_data {
    uint32_t page;
    uint32_t payload_offset;
    uint32_t payload_count;
    float scissor[4];
};

struct text_feature_userdata {
    PulseAppId app = nullptr;
    ecs_query_t* query = nullptr;
    PulseShaderHandle shader;
    PulseMaterialHandle material;
    PulseMeshHandle mesh;
    PulseSamplerHandle sampler;
    std::vector<uint32_t> used_pages;
    HMM_Mat4 view_proj = {};
    uint32_t frame_width = 0;
    uint32_t frame_height = 0;
};

struct pulse_text_render_state {
    PulseAppId app = nullptr;
};

void install_text_feature(PulseAppId app, ecs_world_t* world);

} // namespace pulse_text_render_internal
