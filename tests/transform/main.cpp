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

static void set_pos(ecs_world_t* w, ecs_entity_t e, HMM_Vec3 v) {
    PulsePosition d; d.value = v;
    ecs_set_id(w, e, ecs_id(PulsePosition), sizeof(PulsePosition), &d);
}
static void set_rot(ecs_world_t* w, ecs_entity_t e, HMM_Quat q) {
    PulseRotation d; d.value = q;
    ecs_set_id(w, e, ecs_id(PulseRotation), sizeof(PulseRotation), &d);
}
static void set_tree(ecs_world_t* w, ecs_entity_t e, ecs_entity_t p) {
    PulseTree d; memset(&d, 0, sizeof(d)); d.parent = p;
    ecs_set_id(w, e, ecs_id(PulseTree), sizeof(PulseTree), &d);
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
    PulseAppId app = pulse_create_app("t-comp");
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_RESULT_OK);

    assert(ecs_id(PulsePosition) != 0);
    assert(ecs_id(PulseRotation) != 0);
    assert(ecs_id(PulseLocalTransform) != 0);
    assert(ecs_id(PulseWorldTransform) != 0);
    assert(ecs_id(PulseShowMatrix) != 0);
    assert(ecs_id(PulseTree) != 0);

    ecs_world_t* world = pulse_app_world(app);
    assert(ecs_get_type_info(world, ecs_id(PulsePosition))->size == sizeof(PulsePosition));
    assert(ecs_get_type_info(world, ecs_id(PulseRotation))->size == sizeof(PulseRotation));
    assert(ecs_get_type_info(world, ecs_id(PulseLocalTransform))->size == sizeof(PulseLocalTransform));
    assert(ecs_get_type_info(world, ecs_id(PulseWorldTransform))->size == sizeof(PulseWorldTransform));
    assert(ecs_get_type_info(world, ecs_id(PulseTree))->size == sizeof(PulseTree));

    assert(pulse_add_transform_plugin(app) == PULSE_RESULT_ERROR_DUPLICATE_PLUGIN);
    pulse_destroy_app(app);
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_empty_update() {
    printf("  test_empty_update...\n");
    PulseAppId app = pulse_create_app("t-empty");
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    pulse_destroy_app(app);
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_single_transform() {
    printf("  test_single_transform...\n");
    PulseAppId app = pulse_create_app("t-single");
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_RESULT_OK);

    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t e = ecs_new(world);
    assert(e);

    set_pos(world, e, HMM_V3(3.0f, 4.0f, 5.0f));
    set_tree(world, e, 0);

    assert(pulse_app_update(app) == PULSE_RESULT_OK);

    const PulseLocalTransform* lt = ecs_get(world, e, PulseLocalTransform);
    assert(lt);
    // Column-major: translation in column 3
    assert(fabsf(lt->model.Elements[3][0] - 3.0f) < 1e-5f);
    assert(fabsf(lt->model.Elements[3][1] - 4.0f) < 1e-5f);
    assert(fabsf(lt->model.Elements[3][2] - 5.0f) < 1e-5f);
    // Upper-left 3x3 should be identity
    assert(fabsf(lt->model.Elements[0][0] - 1.0f) < 1e-5f);
    assert(fabsf(lt->model.Elements[1][1] - 1.0f) < 1e-5f);
    assert(fabsf(lt->model.Elements[2][2] - 1.0f) < 1e-5f);

    const PulseWorldTransform* wt = ecs_get(world, e, PulseWorldTransform);
    assert(wt);
    assert(mat4_eq(&lt->model, &wt->value, 1e-5f));

    pulse_destroy_app(app);
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_rotation_transform() {
    printf("  test_rotation_transform...\n");
    PulseAppId app = pulse_create_app("t-rot");
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_RESULT_OK);

    ecs_world_t* world = pulse_app_world(app);
    ecs_entity_t e = ecs_new(world);
    assert(e);

    set_pos(world, e, HMM_V3(1.0f, 2.0f, 3.0f));
    set_rot(world, e, HMM_QFromAxisAngle_RH(HMM_V3(0,0,1), HMM_AngleDeg(90)));
    set_tree(world, e, 0);

    assert(pulse_app_update(app) == PULSE_RESULT_OK);

    const PulseLocalTransform* lt = ecs_get(world, e, PulseLocalTransform);
    assert(lt);

    // Translation in column 3
    assert(fabsf(lt->model.Elements[3][0] - 1.0f) < 1e-4f);
    assert(fabsf(lt->model.Elements[3][1] - 2.0f) < 1e-4f);
    assert(fabsf(lt->model.Elements[3][2] - 3.0f) < 1e-4f);

    // 90-deg Z-rotation: X basis -> (0,1,0) in column 0; Y basis -> (-1,0,0) in column 1
    assert(fabsf(lt->model.Elements[0][0] - 0.0f) < 1e-4f);
    assert(fabsf(lt->model.Elements[0][1] - 1.0f) < 1e-4f);
    assert(fabsf(lt->model.Elements[1][0] - (-1.0f)) < 1e-4f);
    assert(fabsf(lt->model.Elements[1][1] - 0.0f) < 1e-4f);

    pulse_destroy_app(app);
    printf("    PASS\n");
}

// ---------------------------------------------------------------------------
static void test_hierarchy() {
    printf("  test_hierarchy...\n");
    PulseAppId app = pulse_create_app("t-hier");
    assert(app);
    assert(pulse_add_transform_plugin(app) == PULSE_RESULT_OK);

    ecs_world_t* world = pulse_app_world(app);

    ecs_entity_t parent = ecs_new(world);
    ecs_entity_t child = ecs_new(world);
    ecs_entity_t grandchild = ecs_new(world);
    assert(parent && child && grandchild);

    set_pos(world, parent, HMM_V3(10, 0, 0));
    set_pos(world, child, HMM_V3(0, 5, 0));
    set_pos(world, grandchild, HMM_V3(0, 0, 3));

    set_tree(world, parent, 0);
    set_tree(world, child, parent);
    set_tree(world, grandchild, child);

    for (int f = 0; f < 3; ++f)
        assert(pulse_app_update(app) == PULSE_RESULT_OK);

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
    test_hierarchy();
    printf("\nAll transform tests passed!\n");
    return 0;
}
