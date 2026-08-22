// Shared transform test definitions, cut from tests/transform/main.cpp.
// Only what every transform test needs: the pulse app/math/transform APIs
// and the local-transform helpers. Anything used by a single test lives in
// that test file.
#pragma once

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_transform.h"

// HMM_Mat4 uses column-major storage: Elements[Column][Row]
// Translation is in column 3: Elements[3][0]=X, [3][1]=Y, [3][2]=Z
// Rotation is in the upper-left 3x3: Elements[0..2][0..2]

static void set_local_transform(ecs_world_t* w, ecs_entity_t e,
                                HMM_Vec3 translation, HMM_Quat rotation, HMM_Vec3 scale) {
    PulseLocalTransform lt;
    lt.translation = translation;
    lt.rotation = rotation;
    lt.scale = scale;
    ecs_set_id(w, e, ecs_id(PulseLocalTransform), sizeof(PulseLocalTransform), &lt);
}

static void set_local_pos(ecs_world_t* w, ecs_entity_t e, HMM_Vec3 v) {
    set_local_transform(w, e, v, HMM_Q(0.f, 0.f, 0.f, 1.f), HMM_V3(1.f, 1.f, 1.f));
}

static PulseAppId make_transform_app(const char* name) {
    PulseAppDesc app_desc = {
        .name = name,
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    return app;
}