#include "test_common.h"

int main() {
    PulseAppId app = make_transform_app("t-scale");

    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t e = ecs_new(world);
    assert(e);

    set_local_transform(world, e, HMM_V3(1.0f, 2.0f, 3.0f),
                        HMM_Q(0.f, 0.f, 0.f, 1.f), HMM_V3(2.0f, 3.0f, 4.0f));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);

    const PulseWorldTransform* wt = ecs_get(world, e, PulseWorldTransform);
    assert(wt);

    // Translation should be in column 3, scaled values in diagonal
    assert(fabsf(wt->value.Elements[3][0] - 1.0f) < 1e-4f);
    assert(fabsf(wt->value.Elements[3][1] - 2.0f) < 1e-4f);
    assert(fabsf(wt->value.Elements[3][2] - 3.0f) < 1e-4f);
    // Scale in diagonal
    assert(fabsf(wt->value.Elements[0][0] - 2.0f) < 1e-4f);
    assert(fabsf(wt->value.Elements[1][1] - 3.0f) < 1e-4f);
    assert(fabsf(wt->value.Elements[2][2] - 4.0f) < 1e-4f);

    pulse_destroy_app(app);
    return 0;
}
