#include "test_common.h"

#include <math.h>

#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_transform.h"

static void expect_substr(const char* haystack, const char* needle) {
	if (!strstr(haystack, needle)) {
		printf("missing '%s' in json:\n%s\n", needle, haystack);
		assert(0);
	}
}

static PulseAppId make_meta_transform_app(const char* name) {
	PulseAppDesc app_desc = {
		.name = name,
	};
	PulseAppId app = pulse_create_app(&app_desc);
	assert(app);
	assert(pulse_add_math_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
	assert(pulse_add_transform_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
	return app;
}

static ecs_entity_t named_entity(ecs_world_t* world, const char* name) {
	ecs_entity_desc_t edesc = { 0 };
	edesc.name = name;
	return ecs_entity_init(world, &edesc);
}

int main() {
	PulseAppId app = make_meta_transform_app("t-meta-json");
	ecs_world_t* world = pulse_app_world(app);

	ecs_entity_t vec3_type = ecs_lookup(world, "HMM_Vec3");
	assert(vec3_type != 0);
	assert(ecs_has_id(world, vec3_type, ecs_id(EcsStruct)));
	assert(ecs_lookup(world, "union HMM_Vec3") == 0);

	PulseLocalTransform lt = {};
	lt.translation = HMM_V3(1.5f, -2.5f, 4.f);
	lt.rotation = HMM_Q(0.f, 0.7071068f, 0.f, 0.7071068f);
	lt.scale = HMM_V3(2.f, 2.f, 2.f);

	ecs_entity_t thing = named_entity(world, "thing");
	ecs_set_id(world, thing, ecs_id(PulseLocalTransform), sizeof(lt), &lt);
	assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
	assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);

	char* json = ecs_entity_to_json(world, thing, nullptr);
	assert(json);
	printf("thing json: %s\n", json);
	expect_substr(json, "\"PulseLocalTransform\"");
	expect_substr(json, "\"translation\":{\"X\":1.5, \"Y\":-2.5, \"Z\":4}");
	expect_substr(json, "\"scale\":{\"X\":2, \"Y\":2, \"Z\":2}");

	const PulseWorldTransform* src_wt = ecs_get(world, thing, PulseWorldTransform);
	assert(src_wt);

	PulseAppId app2 = make_meta_transform_app("t-meta-json2");
	ecs_world_t* world2 = pulse_app_world(app2);
	ecs_entity_t loaded = named_entity(world2, "thing2");

	assert(ecs_entity_from_json(world2, loaded, json, nullptr) != nullptr);
	ecs_os_free(json);

	const PulseLocalTransform* lt2 = ecs_get(world2, loaded, PulseLocalTransform);
	assert(lt2);
	assert(lt2->translation.X == 1.5f && lt2->translation.Y == -2.5f && lt2->translation.Z == 4.f);
	assert(lt2->scale.X == 2.f && lt2->scale.Z == 2.f);
	assert(fabsf(lt2->rotation.Y - 0.7071068f) < 1e-6f);

	assert(pulse_app_prepare(app2) == PULSE_APP_PREPARE_RESULT_OK);
	assert(pulse_app_update(app2) == PULSE_APP_UPDATE_RESULT_OK);

	const PulseWorldTransform* wt2 = ecs_get(world2, loaded, PulseWorldTransform);
	assert(wt2);
	assert(fabsf(wt2->value.Elements[3][0] - 1.5f) < 1e-4f);
	assert(fabsf(wt2->value.Elements[3][1] + 2.5f) < 1e-4f);
	assert(fabsf(wt2->value.Elements[3][2] - 4.f) < 1e-4f);

	pulse_destroy_app(app);
	pulse_destroy_app(app2);
	printf("transform json round-trip test passed\n");
	return 0;
}
