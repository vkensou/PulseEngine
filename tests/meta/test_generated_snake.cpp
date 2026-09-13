#include "test_common.h"

#include "pulse_cpp_gameplay.h"

#include "snake.h"
#include "snake_module.h"

int main() {
	PulseAppDesc app_desc = {
		.name = "t-generated-snake",
	};
	PulseAppId app = pulse_create_app(&app_desc);
	assert(app);
	assert(pulse_add_math_plugin(app) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
	ecs_world_t* w = pulse_app_world(app);
	flecs::world world(w);

	pulse::GameplayModuleState state;
	pulse::init_gameplay_base(world);
	pulse::ModuleContext moduleContext = pulse::make_module_context(world, flecs::PostUpdate, &state.eventCenter);
	importModule(&moduleContext);

	ecs_entity_t dir = ecs_lookup(w, "Direction4W");
	assert(dir != 0);
	assert(ecs_has_id(w, dir, ecs_id(EcsEnum)));
	assert(ecs_lookup_child(w, dir, "Up") != 0);
	assert(ecs_lookup_child(w, dir, "Down") != 0);

	ecs_entity_t game_state = ecs_lookup(w, "SnakeGameState");
	assert(game_state != 0);
	assert(ecs_has_id(w, game_state, ecs_id(EcsEnum)));
	assert(ecs_lookup_child(w, game_state, "UnInitialized") != 0);

	ecs_entity_t facing = world.id<Facing4W>();
	assert(ecs_has_id(w, facing, ecs_id(EcsStruct)));
	const ecs_member_t* value = ecs_struct_get_member(w, facing, "value");
	assert(value && value->type == dir);
	assert(value->offset == offsetof(Facing4W, value));

	ecs_entity_t move = world.id<SnakeMove>();
	const ecs_member_t* interval = ecs_struct_get_member(w, move, "interval");
	assert(interval && interval->type == ecs_id(ecs_f32_t));

	ecs_entity_t border = world.id<Border>();
	const ecs_member_t* up = ecs_struct_get_member(w, border, "up");
	assert(up && up->type == ecs_id(ecs_i32_t) && up->offset == offsetof(Border, up));
	assert(ecs_struct_get_member(w, border, "bottom") != nullptr);
	assert(ecs_struct_get_member(w, border, "left") != nullptr);
	assert(ecs_struct_get_member(w, border, "right") != nullptr);

	ecs_entity_t intent = world.id<SnakeMoveIntentEvent>();
	const ecs_member_t* delta = ecs_struct_get_member(w, intent, "delta");
	assert(delta && delta->type == world.id<HMM_Vec3>());
	assert(ecs_has_id(w, world.id<HMM_Vec3>(), ecs_id(EcsStruct)));

	ecs_entity_t eaten = world.id<AppleEatenEvent>();
	const ecs_member_t* apple = ecs_struct_get_member(w, eaten, "apple");
	assert(apple && apple->type == ecs_id(ecs_entity_t));

	ecs_entity_t input = world.id<SnakeInput>();
	assert(input != 0);
	assert(!ecs_has_id(w, input, ecs_id(EcsStruct)));
	assert(world.id<SnakeBodies>() != 0);
	assert(world.id<SnakePrefabs>() != 0);

	assert(ecs_lookup(w, "IsApple") == world.id<IsApple>());
	assert(ecs_lookup(w, "GameOverEvent") == world.id<GameOverEvent>());
	assert(ecs_lookup(w, "RestartEvent") == world.id<RestartEvent>());
	assert(ecs_lookup(w, "Border") == border);
	assert(ecs_lookup(w, "Score") == world.id<Score>());

	flecs::entity snake = world.entity("snake");
	snake.set<Facing4W>(Facing4W{Direction4W::Right});
	snake.set<SnakeMove>(SnakeMove{0.2f, 0.f});
	snake.set<Score>(Score{3});
	std::string json = snake.to_json().c_str();
	printf("snake json: %s\n", json.c_str());
	assert(json.find("\"value\":\"Right\"") != std::string::npos);
	assert(json.find("\"interval\":0.2") != std::string::npos);
	assert(json.find("\"Score\":{\"value\":3}") != std::string::npos);

	snake.set<SnakeMoveIntentEvent>(SnakeMoveIntentEvent{HMM_V3(1.f, 2.f, 3.f)});
	std::string move_json = snake.to_json().c_str();
	assert(move_json.find("\"delta\":{\"X\":1, \"Y\":2, \"Z\":3}") != std::string::npos);

	state.eventCenter.clear();
	pulse_destroy_app(app);
	printf("generated snake module reflection test passed\n");
	return 0;
}
