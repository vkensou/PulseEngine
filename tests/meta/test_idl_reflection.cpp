#include "test_common.h"

#include "pulse_app.h"
#include "pulse_input.h"
#include "pulse_math.h"
#include "pulse_transform.h"

static const ecs_member_t* expect_member(ecs_world_t* world, ecs_entity_t type, const char* name) {
	const ecs_member_t* m = ecs_struct_get_member(world, type, name);
	assert(m != NULL);
	return m;
}

static ecs_entity_t expect_struct(ecs_world_t* world, const char* name) {
	ecs_entity_t type = ecs_lookup(world, name);
	assert(type != 0);
	assert(ecs_has_id(world, type, ecs_id(EcsStruct)));
	return type;
}

int main() {
	PulseAppDesc app_desc = {
		.name = "t-idl-reflection",
	};
	PulseAppId app = pulse_create_app(&app_desc);
	assert(app);
	assert(pulse_add_math_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
	assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
	assert(pulse_add_input_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

	ecs_world_t* world = pulse_app_world(app);

	ecs_entity_t vec3 = expect_struct(world, "HMM_Vec3");
	expect_member(world, vec3, "X");
	expect_member(world, vec3, "Y");
	expect_member(world, vec3, "Z");
	ecs_entity_t mat4 = expect_struct(world, "HMM_Mat4");
	assert(expect_member(world, mat4, "Columns")->count == 4);

	expect_struct(world, "PulseLocalTransform");
	const ecs_member_t* translation = expect_member(world, ecs_id(PulseLocalTransform), "translation");
	assert(translation->type == vec3);
	assert(translation->offset == offsetof(PulseLocalTransform, translation));
	expect_member(world, ecs_id(PulseLocalTransform), "rotation");
	expect_member(world, ecs_id(PulseLocalTransform), "scale");
	expect_member(world, ecs_id(PulseWorldTransform), "value");

	const ecs_member_t* pressed = expect_member(world, ecs_id(PulseKeyboardInput), "pressed");
	assert(pressed->count == 512);
	const ecs_member_t* window = expect_member(world, ecs_id(PulseMouseMotionEvent), "window");
	assert(window->type == ecs_id(ecs_entity_t));

	expect_member(world, ecs_id(PulseTimer), "delta_time");
	const ecs_member_t* fps = expect_member(world, ecs_id(PulseTimer), "fps");
	assert(fps->range.min == 0.0);
	assert(fps->range.max == 1000.0);

	pulse_destroy_app(app);
	printf("idl reflection test passed\n");
	return 0;
}
