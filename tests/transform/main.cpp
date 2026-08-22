#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_transform.h"

// ---------------------------------------------------------------------------
// HMM_Mat4 uses column-major storage: Elements[Column][Row]
// Translation is in column 3: Elements[3][0]=X, [3][1]=Y, [3][2]=Z
// Rotation is in the upper-left 3x3: Elements[0..2][0..2]
// ---------------------------------------------------------------------------

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

static bool mat4_eq(const HMM_Mat4* a, const HMM_Mat4* b, float tol) {
    for (int c = 0; c < 4; ++c)
        for (int r = 0; r < 4; ++r)
            if (fabsf(a->Elements[c][r] - b->Elements[c][r]) > tol) return false;
    return true;
}

// ---------------------------------------------------------------------------
static void test_components() {
    printf("  test_components...\n");
    PulseAppDesc app_desc = {
        .name = "t-comp",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    assert(ecs_id(PulseLocalTransform) != 0);
    assert(ecs_id(PulseWorldTransform) != 0);
    assert(ecs_id(PulseShowMatrix) != 0);

    ecs_world_t* world = pulse_app_world(app);
    assert(ecs_get_type_info(world, ecs_id(PulseLocalTransform))->size == sizeof(PulseLocalTransform));
    assert(ecs_get_type_info(world, ecs_id(PulseWorldTransform))->size == sizeof(PulseWorldTransform));

    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN);
    pulse_destroy_app(app);
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_empty_update() {
    printf("  test_empty_update...\n");
    PulseAppDesc app_desc = {
        .name = "t-empty",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    pulse_destroy_app(app);
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_single_transform() {
    printf("  test_single_transform...\n");
    PulseAppDesc app_desc = {
        .name = "t-single",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

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
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_rotation_transform() {
    printf("  test_rotation_transform...\n");
    PulseAppDesc app_desc = {
        .name = "t-rot",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

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
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_scale_transform() {
    printf("  test_scale_transform...\n");
    PulseAppDesc app_desc = {
        .name = "t-scale",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

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
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_hierarchy() {
    printf("  test_hierarchy...\n");
    PulseAppDesc app_desc = {
        .name = "t-hier",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

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
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_auto_insertion() {
    printf("  test_auto_insertion...\n");
    PulseAppDesc app_desc = {
        .name = "t-auto",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

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
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
int main() {
    printf("PulseTransform Tests\n");
    printf("====================\n\n");
    test_components();
    test_empty_update();
    test_single_transform();
    test_rotation_transform();
    test_scale_transform();
    test_hierarchy();
    test_auto_insertion();
    printf("\nAll transform tests passed!\n");
    return 0;
}
