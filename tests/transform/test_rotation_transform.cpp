#include "test_common.h"

int main() {
    PulseAppId app = make_transform_app("t-rot");

    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t e = ecs_new(world);
    assert(e);

    HMM_Quat rot = HMM_QFromAxisAngle_RH(HMM_V3(0, 0, 1), HMM_AngleDeg(90));
    set_local_transform(world, e, HMM_V3(1.0f, 2.0f, 3.0f), rot, HMM_V3(1.f, 1.f, 1.f));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);

    const PulseWorldTransform* wt = ecs_get(world, e, PulseWorldTransform);
    assert(wt);

    // Translation in column 3
    assert(fabsf(wt->value.Elements[3][0] - 1.0f) < 1e-4f);
    assert(fabsf(wt->value.Elements[3][1] - 2.0f) < 1e-4f);
    assert(fabsf(wt->value.Elements[3][2] - 3.0f) < 1e-4f);

    // 90-deg Z-rotation: X basis -> (0,1,0) in column 0; Y basis -> (-1,0,0) in column 1
    assert(fabsf(wt->value.Elements[0][0] - 0.0f) < 1e-4f);
    assert(fabsf(wt->value.Elements[0][1] - 1.0f) < 1e-4f);
    assert(fabsf(wt->value.Elements[1][0] - (-1.0f)) < 1e-4f);
    assert(fabsf(wt->value.Elements[1][1] - 0.0f) < 1e-4f);

    pulse_destroy_app(app);
    return 0;
}
