#include <assert.h>
#include <stdio.h>
#include <vector>

#include "pulse_app.h"
#include "pulse_window.h"
#include "pulse_cgpu_render.h"
#include "renderer.h"
#include "rendergraph_cpp.h"

struct test_render_state {
    std::vector<HGEGraphics::Backbuffer> backbuffers;
};

static void record_test_rendergraph(
    pulse_rendergraph_t* graph,
    uint32_t target_count,
    const pulse_cgpu_render_window_target* targets,
    void* user_data
) {
    test_render_state* state = static_cast<test_render_state*>(user_data);

    if (!graph || !target_count || !targets || !state) {
        return;
    }

    state->backbuffers.clear();
    state->backbuffers.reserve(target_count);

    for (uint32_t i = 0; i < target_count; ++i) {
        const pulse_cgpu_render_window_target& window_target = targets[i];
        state->backbuffers.emplace_back();
        HGEGraphics::Backbuffer& backbuffer = state->backbuffers.back();
        HGEGraphics::init_backbuffer(
            &backbuffer,
            window_target.swapchain,
            static_cast<int>(window_target.backbuffer_index)
        );

        HGEGraphics::texture_handle_t target =
            pulse_rendergraph_import_backbuffer(graph, &backbuffer);
        if (!HGEGraphics::rendergraph_texture_handle_valid(target)) {
            continue;
        }

        pulse_renderpass_builder_t pass =
            pulse_rendergraph_add_renderpass(graph, "TestCallbackPass");
        pulse_renderpass_add_color_attachment(
            &pass,
            target,
            CGPU_LOAD_ACTION_CLEAR,
            0xff00ffff,
            CGPU_STORE_ACTION_STORE
        );
        pulse_rendergraph_present(graph, target);
    }
}

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    auto window_plugin_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_plugin_desc) == PULSE_OK);

    auto cgpu_render_plugin_desc = pulse_cgpu_render_plugin_desc_default();
    assert(pulse_cgpu_render_add_plugin(app, &cgpu_render_plugin_desc) == PULSE_OK);

    test_render_state render_state;
    assert(
        pulse_cgpu_render_set_record_callback(
            app,
            record_test_rendergraph,
            &render_state
        ) == PULSE_OK
    );

    pulse_app_run(app);

    pulse_app_destroy(app);

    printf("All tests passed!\n");
    return 0;
}
