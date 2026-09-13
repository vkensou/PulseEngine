#include "test_common.h"

#include "pulse_app.h"
#include "pulse_math.h"

static ecs_entity_t expect_struct(ecs_world_t* w, const char* name, size_t size) {
	char compiler_name[64];
	snprintf(compiler_name, sizeof(compiler_name), "union %s", name);
	assert(ecs_lookup(w, compiler_name) == 0);

	ecs_entity_t type = ecs_lookup(w, name);
	assert(type != 0);

	const EcsType* t = ecs_get(w, type, EcsType);
	assert(t && t->kind == EcsStructType);

	const EcsComponent* comp = ecs_get(w, type, EcsComponent);
	assert(comp && (size_t)comp->size == size);

	return type;
}

static void expect_f32(ecs_world_t* w, ecs_entity_t type, const char* name, int32_t offset) {
	const ecs_member_t* m = ecs_struct_get_member(w, type, name);
	assert(m != NULL);
	assert(m->type == ecs_id(ecs_f32_t));
	assert(m->count == 0);
	assert(m->offset == offset);
}

static void expect_columns(ecs_world_t* w, ecs_entity_t type, ecs_entity_t elem, int32_t count) {
	const ecs_member_t* m = ecs_struct_get_member(w, type, "Columns");
	assert(m != NULL);
	assert(m->type == elem);
	assert(m->count == count);
	assert(m->offset == 0);
}

int main() {
	PulseAppDesc app_desc = {
		.name = "t-math-reflection",
	};
	PulseAppId app = pulse_create_app(&app_desc);
	assert(app);
	assert(pulse_add_math_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

	ecs_world_t* w = pulse_app_world(app);
	flecs::world world(w);

	ecs_entity_t vec2 = expect_struct(w, "HMM_Vec2", sizeof(HMM_Vec2));
	ecs_entity_t vec3 = expect_struct(w, "HMM_Vec3", sizeof(HMM_Vec3));
	ecs_entity_t vec4 = expect_struct(w, "HMM_Vec4", sizeof(HMM_Vec4));
	ecs_entity_t mat2 = expect_struct(w, "HMM_Mat2", sizeof(HMM_Mat2));
	ecs_entity_t mat3 = expect_struct(w, "HMM_Mat3", sizeof(HMM_Mat3));
	ecs_entity_t mat4 = expect_struct(w, "HMM_Mat4", sizeof(HMM_Mat4));
	ecs_entity_t quat = expect_struct(w, "HMM_Quat", sizeof(HMM_Quat));

	assert(world.id<HMM_Vec2>() == vec2);
	assert(world.id<HMM_Vec3>() == vec3);
	assert(world.id<HMM_Vec4>() == vec4);
	assert(world.id<HMM_Mat2>() == mat2);
	assert(world.id<HMM_Mat3>() == mat3);
	assert(world.id<HMM_Mat4>() == mat4);
	assert(world.id<HMM_Quat>() == quat);
	assert(world.id<HMM_Bool>() == ecs_id(ecs_i32_t));

	expect_f32(w, vec2, "X", 0);
	expect_f32(w, vec2, "Y", 4);
	expect_f32(w, vec3, "X", 0);
	expect_f32(w, vec3, "Y", 4);
	expect_f32(w, vec3, "Z", 8);
	expect_f32(w, vec4, "X", 0);
	expect_f32(w, vec4, "Y", 4);
	expect_f32(w, vec4, "Z", 8);
	expect_f32(w, vec4, "W", 12);
	expect_f32(w, quat, "X", 0);
	expect_f32(w, quat, "Y", 4);
	expect_f32(w, quat, "Z", 8);
	expect_f32(w, quat, "W", 12);
	expect_columns(w, mat2, vec2, 2);
	expect_columns(w, mat3, vec3, 3);
	expect_columns(w, mat4, vec4, 4);

	HMM_Vec2 v2 = HMM_V2(1.f, -2.5f);
	HMM_Mat2 m2 = HMM_M2();
	m2.Elements[0][0] = 1.f;
	m2.Elements[1][1] = 2.5f;
	HMM_Mat3 m3 = HMM_M3();
	m3.Elements[0][0] = 1.f;
	m3.Elements[1][1] = 2.5f;
	m3.Elements[2][2] = -3.f;

	char* v2_json = ecs_ptr_to_json(w, vec2, &v2);
	assert(v2_json != NULL);
	printf("HMM_Vec2 json: %s\n", v2_json);
	assert(strstr(v2_json, "{\"X\":1, \"Y\":-2.5}") != NULL);
	ecs_os_free(v2_json);

	char* m2_json = ecs_ptr_to_json(w, mat2, &m2);
	assert(m2_json != NULL);
	printf("HMM_Mat2 json: %s\n", m2_json);
	assert(strstr(m2_json, "{\"Columns\":[{\"X\":1, \"Y\":0}, {\"X\":0, \"Y\":2.5}]}") != NULL);
	ecs_os_free(m2_json);

	char* m3_json = ecs_ptr_to_json(w, mat3, &m3);
	assert(m3_json != NULL);
	printf("HMM_Mat3 json: %s\n", m3_json);
	assert(strstr(m3_json, "\"Columns\":[{\"X\":1, \"Y\":0, \"Z\":0}") != NULL);
	assert(strstr(m3_json, "\"Y\":2.5") != NULL);
	assert(strstr(m3_json, "\"Z\":-3") != NULL);

	HMM_Mat3 m3_decoded = HMM_M3();
	assert(ecs_ptr_from_json(w, mat3, &m3_decoded, m3_json, NULL) != NULL);
	assert(m3_decoded.Elements[0][0] == 1.f);
	assert(m3_decoded.Elements[1][1] == 2.5f);
	assert(m3_decoded.Elements[2][2] == -3.f);
	ecs_os_free(m3_json);

	pulse_destroy_app(app);
	printf("math reflection test passed\n");
	return 0;
}
