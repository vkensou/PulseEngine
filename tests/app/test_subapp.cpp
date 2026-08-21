#include "test_common.h"

static bool sub_build_called = false;

static EPulseResult sub_build(PulseAppId app, void* ctx) {
    (void)app;
    (void)ctx;
    sub_build_called = true;
    return PULSE_RESULT_OK;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-app-subapp",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    // ---- subapp ----
    PulseAppDesc sub_desc = {
        .name = "Sub",
    };
    PulseAppId sub = pulse_create_app(&sub_desc);
    assert(sub != nullptr);

    PulsePluginDesc sub_plugin_desc = {
        .struct_size = sizeof(PulsePluginDesc),
        .version = PULSE_PLUGIN_DESC_VERSION,
        .name = "SubPlugin",
        .build = sub_build,
    };
    assert(pulse_app_add_plugin(sub, &sub_plugin_desc) == PULSE_RESULT_OK);
    assert(sub_build_called);

    assert(pulse_app_insert_subapp(app, "Sub", sub) == PULSE_RESULT_OK);
    assert(pulse_app_get_subapp(app, "Sub") == sub);

    // ---- subapp removal is only valid before run ----
    PulseAppDesc sub2_desc = {
        .name = "Sub2",
    };
    PulseAppId sub2 = pulse_create_app(&sub2_desc);
    assert(sub2 != nullptr);
    assert(pulse_app_insert_subapp(app, "Sub2", sub2) == PULSE_RESULT_OK);
    PulseAppId removed = pulse_app_remove_subapp(app, "Sub2");
    assert(removed == sub2);
    assert(pulse_app_get_subapp(app, "Sub2") == nullptr);
    pulse_destroy_app(removed);

    pulse_destroy_app(app);

    printf("Subapp test passed!\n");
    return 0;
}