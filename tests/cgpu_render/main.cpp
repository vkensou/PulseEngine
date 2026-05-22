#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_window.h"
#include "pulse_cgpu_render.h"
#include "rendergraph_cpp.h"

struct test_render_state {
    ecs_query_t* window_query;
};

static void record_test_rendergraph(
    pulse_app_t app,
    pulse_rendergraph_t* graph,
    void* user_data
) {
    test_render_state* state = static_cast<test_render_state*>(user_data);
    if (!graph || !state) {
        return;
    }

    if (!state->window_query) {
        return;
    }

    ecs_iter_t it = ecs_query_iter(state->window_query->world, state->window_query);
    while (ecs_query_next(&it)) {
        for (int i = 0; i < it.count; ++i) {
            ecs_entity_t entity = it.entities[i];

            pulse_texture_handle_t target_handle =
                pulse_cgpu_render_import_window_backbuffer(app, graph, entity);
            if (!pulse_rendergraph_texture_handle_valid(target_handle)) {
                continue;
            }

            pulse_renderpass_builder_t pass =
                pulse_rendergraph_add_renderpass(graph, "TestCallbackPass");
            pulse_renderpass_add_color_attachment(
                &pass,
                target_handle,
                CGPU_LOAD_ACTION_CLEAR,
                0xff00ffff,
                CGPU_STORE_ACTION_STORE
            );
            pulse_rendergraph_present(graph, target_handle);
        }
    }
}

int main(void) {
    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    auto window_plugin_desc = pulse_window_plugin_desc_default();
    assert(pulse_window_add_plugin(app, &window_plugin_desc) == PULSE_OK);

    auto cgpu_render_plugin_desc = pulse_cgpu_render_plugin_desc_default();
    assert(pulse_cgpu_render_add_plugin(app, &cgpu_render_plugin_desc) == PULSE_OK);

    test_render_state render_state{};

    ecs_query_desc_t window_query_desc{};
    window_query_desc.terms[0] = { .id = ecs_id(pulse_window) };
    window_query_desc.cache_kind = EcsQueryCacheAuto;
    render_state.window_query = ecs_query_init(pulse_app_world(app), &window_query_desc);

    assert(
        pulse_cgpu_render_set_record_callback(
            app,
            record_test_rendergraph,
            &render_state
        ) == PULSE_OK
    );

    pulse_app_run(app);

    ecs_query_fini(render_state.window_query);

    pulse_app_destroy(app);

    printf("All tests passed!\n");
    return 0;
}
