#include "test_common.h"

static ecs_world_t* wc(flecs::world& w) { return w.c_ptr(); }

int main() {
	flecs::world world;
	register_test_meta(world);

	ecs_entity_t prim = world.id<TestPrim>();
	ecs_entity_t mixed = world.id<TestMixed>();
	ecs_entity_t opaque = world.id<TestOpaque>();
	ecs_entity_t tag = world.id<TestTag>();
	ecs_entity_t dir = world.id<TestDirection>();

	assert(ecs_has_id(wc(world), prim, ecs_id(EcsStruct)));
	assert(ecs_has_id(wc(world), mixed, ecs_id(EcsStruct)));
	assert(!ecs_has_id(wc(world), opaque, ecs_id(EcsStruct)));
	assert(ecs_has_id(wc(world), dir, ecs_id(EcsEnum)));

	const ecs_member_t* hp = ecs_struct_get_member(wc(world), prim, "hp");
	assert(hp && hp->type == ecs_id(ecs_i32_t));
	const ecs_member_t* shield = ecs_struct_get_member(wc(world), prim, "shield");
	assert(shield && shield->type == ecs_id(ecs_u32_t));
	const ecs_member_t* alive = ecs_struct_get_member(wc(world), prim, "alive");
	assert(alive && alive->type == ecs_id(ecs_bool_t));
	const ecs_member_t* sign = ecs_struct_get_member(wc(world), prim, "sign");
	assert(sign && sign->type == ecs_id(ecs_char_t));
	const ecs_member_t* ratio = ecs_struct_get_member(wc(world), prim, "ratio");
	assert(ratio && ratio->type == ecs_id(ecs_f32_t));
	const ecs_member_t* precise = ecs_struct_get_member(wc(world), prim, "precise");
	assert(precise && precise->type == ecs_id(ecs_f64_t));
	assert(hp->offset == offsetof(TestPrim, hp));
	assert(precise->offset == offsetof(TestPrim, precise));

	const ecs_member_t* pos = ecs_struct_get_member(wc(world), mixed, "pos");
	assert(pos && pos->type == ecs_id(ecs_f32_t) && pos->count == 3);
	assert(pos->offset == offsetof(TestMixed, pos));
	const ecs_member_t* rot = ecs_struct_get_member(wc(world), mixed, "rot");
	assert(rot && rot->type == ecs_id(ecs_f32_t) && rot->count == 4);
	const ecs_member_t* target = ecs_struct_get_member(wc(world), mixed, "target");
	assert(target && target->type == ecs_id(ecs_entity_t));
	const ecs_member_t* samples = ecs_struct_get_member(wc(world), mixed, "samples");
	assert(samples && samples->type == ecs_id(ecs_i32_t) && samples->count == 4);
	const ecs_member_t* member_dir = ecs_struct_get_member(wc(world), mixed, "dir");
	assert(member_dir && member_dir->type == dir);

	assert(ecs_lookup_child(wc(world), dir, "Left") != 0);
	assert(ecs_lookup_child(wc(world), dir, "Down") != 0);

	const EcsComponent* tag_comp = ecs_get(wc(world), tag, EcsComponent);
	assert(tag_comp && tag_comp->size == 0);

	assert(ecs_lookup(wc(world), "TestPrim") == prim);
	assert(ecs_lookup(wc(world), "TestTag") == tag);

	printf("meta registration test passed\n");
	return 0;
}
