#include "test_common.h"

static void expect_substr(const char* haystack, const char* needle) {
	if (!strstr(haystack, needle)) {
		printf("missing '%s' in json:\n%s\n", needle, haystack);
		assert(0);
	}
}

int main() {
	flecs::world world;
	register_test_meta(world);

	TestPrim prim = {};
	prim.hp = 77;
	prim.shield = 5u;
	prim.alive = true;
	prim.sign = 'S';
	prim.ratio = 0.5f;
	prim.precise = 3.25;

	TestMixed mixed = {};
	mixed.dir = TestDirection::Left;
	mixed.pos = HMM_V3(1.f, 2.f, 3.f);
	mixed.rot = HMM_Q(0.f, 0.f, 0.f, 1.f);

	flecs::entity apple = world.entity("apple");
	mixed.target = apple;
	mixed.samples[0] = 10;
	mixed.samples[3] = 40;

	flecs::entity hero = world.entity("hero")
		.set<TestPrim>(prim)
		.set<TestMixed>(mixed)
		.add<TestTag>();

	flecs::string json = hero.to_json();
	printf("hero json: %s\n", json.c_str());
	const char* js = json.c_str();
	expect_substr(js, "\"TestPrim\"");
	expect_substr(js, "\"hp\":77");
	expect_substr(js, "\"alive\":true");
	expect_substr(js, "\"ratio\":0.5");
	expect_substr(js, "\"TestMixed\"");
	expect_substr(js, "\"Left\"");
	expect_substr(js, "\"pos\":[1, 2, 3]");
	expect_substr(js, "\"samples\":[10, 0, 0, 40]");
	expect_substr(js, "\"target\":\"#");
	expect_substr(js, "\"TestTag\"");

	TestPrim mutated = prim;
	mutated.hp = 1;
	mutated.alive = false;
	hero.set<TestPrim>(mutated);
	assert(ecs_entity_from_json(world.c_ptr(), hero, js, nullptr) != nullptr);
	assert(hero.get<TestPrim>().hp == 77);
	assert(hero.get<TestPrim>().alive);
	assert(hero.get<TestMixed>().dir == TestDirection::Left);
	assert(hero.get<TestMixed>().pos.X == 1.f && hero.get<TestMixed>().pos.Z == 3.f);
	assert(hero.get<TestMixed>().rot.W == 1.f);
	assert(hero.get<TestMixed>().samples[3] == 40);

	{
		TestPrim decoded = {};
		flecs::string pj = world.to_json(&prim);
		assert(ecs_ptr_from_json(world.c_ptr(), world.id<TestPrim>(), &decoded, pj.c_str(), nullptr) != nullptr);
		assert(decoded.hp == prim.hp && decoded.shield == prim.shield);
		assert(decoded.alive == prim.alive && decoded.sign == prim.sign);
		assert(decoded.ratio == prim.ratio && decoded.precise == prim.precise);
	}

	flecs::string wjson = world.to_json();
	flecs::world world2;
	register_test_meta(world2);
	assert(ecs_world_from_json(world2.c_ptr(), wjson.c_str(), nullptr) != nullptr);

	flecs::entity hero2 = world2.lookup("hero");
	assert(hero2.id() != 0);
	assert(hero2.get<TestPrim>().hp == 77);
	assert(hero2.get<TestMixed>().dir == TestDirection::Left);
	assert(hero2.has<TestTag>());

	flecs::string rejson = hero2.to_json();
	expect_substr(rejson.c_str(), "\"hp\":77");
	expect_substr(rejson.c_str(), "\"Left\"");

	printf("json round-trip test passed\n");
	return 0;
}
