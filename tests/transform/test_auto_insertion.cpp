#include "test_common.h"

int main() {
    PulseAppId app = make_transform_app("t-auto");

    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t e = ecs_new(world);
    assert(e);

    // Adding LocalTransform should auto-insert WorldTransform (via EcsWith)
    set_local_pos(world, e, HMM_V3(1, 2, 3));
    assert(ecs_get(world, e, PulseLocalTransform) != NULL);
    // Auto-inserted via EcsWith: set triggers the pair, but it's a deferred effect.
    // Run update to let flecs process the With relationships.
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(ecs_get(world, e, PulseWorldTransform) != NULL);

    pulse_destroy_app(app);
    return 0;
}
