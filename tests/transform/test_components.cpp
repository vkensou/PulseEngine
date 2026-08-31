#include "test_common.h"

int main() {
    PulseAppId app = make_transform_app("t-comp");

    assert(ecs_id(PulseLocalTransform) != 0);
    assert(ecs_id(PulseWorldTransform) != 0);
    assert(ecs_id(PulseShowMatrix) != 0);

    ecs_world_t* world = pulse_app_world(app);
    assert(ecs_get_type_info(world, ecs_id(PulseLocalTransform))->size == sizeof(PulseLocalTransform));
    assert(ecs_get_type_info(world, ecs_id(PulseWorldTransform))->size == sizeof(PulseWorldTransform));

    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN);
    pulse_destroy_app(app);
    return 0;
}
