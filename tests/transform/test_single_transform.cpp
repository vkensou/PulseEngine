#include "test_common.h"

int main() {
    PulseAppId app = make_transform_app("t-single");

    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t e = ecs_new(world);
    assert(e);

    set_local_pos(world, e, HMM_V3(3.0f, 4.0f, 5.0f));

    assert(ecs_has_id(world, e, ecs_id(PulseWorldTransform)));

    ecs_remove_id(world, e, ecs_id(PulseWorldTransform));
    assert(!ecs_has_id(world, e, ecs_id(PulseWorldTransform)));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);

    assert(ecs_has_id(world, e, ecs_id(PulseWorldTransform)));
    const PulseWorldTransform* wt = ecs_get(world, e, PulseWorldTransform);
    assert(wt);
    // Column-major: translation in column 3
    assert(fabsf(wt->value.Elements[3][0] - 3.0f) < 1e-5f);
    assert(fabsf(wt->value.Elements[3][1] - 4.0f) < 1e-5f);
    assert(fabsf(wt->value.Elements[3][2] - 5.0f) < 1e-5f);
    // Upper-left 3x3 should be identity
    assert(fabsf(wt->value.Elements[0][0] - 1.0f) < 1e-5f);
    assert(fabsf(wt->value.Elements[1][1] - 1.0f) < 1e-5f);
    assert(fabsf(wt->value.Elements[2][2] - 1.0f) < 1e-5f);

    pulse_destroy_app(app);
    return 0;
}
