#include "test_common.h"

int main() {
    PulseAppId app = make_transform_app("t-hier");

    ecs_world_t* world = pulse_app_world(app);

    ecs_entity_t parent = ecs_new(world);
    ecs_entity_t child = ecs_new(world);
    ecs_entity_t grandchild = ecs_new(world);
    assert(parent && child && grandchild);

    set_local_pos(world, parent, HMM_V3(10, 0, 0));
    set_local_pos(world, child, HMM_V3(0, 5, 0));
    set_local_pos(world, grandchild, HMM_V3(0, 0, 3));

    // Build hierarchy via thin API (wraps EcsChildOf)
    pulse_set_parent(app, child, parent);
    pulse_set_parent(app, grandchild, child);

    // Single update is sufficient: EcsCascade guarantees parent-before-child order
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);

    const PulseWorldTransform* pw = ecs_get(world, parent, PulseWorldTransform);
    assert(pw);
    assert(fabsf(pw->value.Elements[3][0] - 10.0f) < 1e-4f);
    assert(fabsf(pw->value.Elements[3][1] - 0.0f) < 1e-4f);
    assert(fabsf(pw->value.Elements[3][2] - 0.0f) < 1e-4f);

    const PulseWorldTransform* cw = ecs_get(world, child, PulseWorldTransform);
    assert(cw);
    assert(fabsf(cw->value.Elements[3][0] - 10.0f) < 1e-4f);
    assert(fabsf(cw->value.Elements[3][1] - 5.0f) < 1e-4f);
    assert(fabsf(cw->value.Elements[3][2] - 0.0f) < 1e-4f);

    const PulseWorldTransform* gw = ecs_get(world, grandchild, PulseWorldTransform);
    assert(gw);
    assert(fabsf(gw->value.Elements[3][0] - 10.0f) < 1e-4f);
    assert(fabsf(gw->value.Elements[3][1] - 5.0f) < 1e-4f);
    assert(fabsf(gw->value.Elements[3][2] - 3.0f) < 1e-4f);

    // Test get_parent
    assert(pulse_get_parent(app, child) == parent);
    assert(pulse_get_parent(app, grandchild) == child);
    assert(pulse_get_parent(app, parent) == 0);

    // Test remove_parent
    pulse_remove_parent(app, grandchild);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    // After removal, grandchild should be a root (world == local)
    assert(pulse_get_parent(app, grandchild) == 0);
    const PulseWorldTransform* gw2 = ecs_get(world, grandchild, PulseWorldTransform);
    assert(gw2);
    assert(fabsf(gw2->value.Elements[3][0] - 0.0f) < 1e-4f);  // just local pos
    assert(fabsf(gw2->value.Elements[3][1] - 0.0f) < 1e-4f);
    assert(fabsf(gw2->value.Elements[3][2] - 3.0f) < 1e-4f);

    pulse_destroy_app(app);
    return 0;
}
