#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_window.h"
#include "pulse_cgpu_render.h"

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    auto window_plugin_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_plugin_desc) == PULSE_OK);

    auto cgpu_render_plugin_desc = pulse_cgpu_render_plugin_desc_default();
    cgpu_render_plugin_desc.clear_color[0] = 1;
    cgpu_render_plugin_desc.clear_color[1] = 1;
    cgpu_render_plugin_desc.clear_color[2] = 0;
    cgpu_render_plugin_desc.clear_color[3] = 1;
    assert(pulse_cgpu_render_add_plugin(app, &cgpu_render_plugin_desc) == PULSE_OK);

    pulse_app_run(app);

    pulse_app_destroy(app);

    printf("All tests passed!\n");
    return 0;
}
