#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_window.h"
#include "pulse_cgpu_render.h"

static void record_test_rendergraph(
    HGEGraphics::rendergraph_t* graph,
    HGEGraphics::texture_handle_t target,
    void* user_data
) {
    (void)user_data;

    if (!graph || !HGEGraphics::rendergraph_texture_handle_valid(target)) {
        return;
    }

    HGEGraphics::renderpass_builder_t pass =
        HGEGraphics::rendergraph_add_renderpass(graph, "TestCallbackPass");
    HGEGraphics::renderpass_add_color_attachment(
        &pass,
        target,
        CGPU_LOAD_ACTION_CLEAR,
        0xff00ffff,
        CGPU_STORE_ACTION_STORE
    );
    HGEGraphics::rendergraph_present(graph, target);
}

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    auto window_plugin_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_plugin_desc) == PULSE_OK);

    auto cgpu_render_plugin_desc = pulse_cgpu_render_plugin_desc_default();
    assert(pulse_cgpu_render_add_plugin(app, &cgpu_render_plugin_desc) == PULSE_OK);
    assert(
        pulse_cgpu_render_set_record_callback(
            app,
            record_test_rendergraph,
            nullptr
        ) == PULSE_OK
    );

    pulse_app_run(app);

    pulse_app_destroy(app);

    printf("All tests passed!\n");
    return 0;
}
