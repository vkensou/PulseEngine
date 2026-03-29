#pragma once

#include "predefine_components.h"
#include "ecsext.hpp"
#include <SDL3/SDL.h>
#include <vector>

enum class Direction4W
{
	Right,
	Up,
	Left,
	Down,
};

PULSE_ECS_COMPONENT
struct Facing4W
{
	Direction4W value;
};

PULSE_ECS_COMPONENT
struct SnakeMove
{
	float interval;
	float lastTime;
};

PULSE_ECS_SINGLETON_COMPONENT
struct Border
{
	int up, bottom, left, right;
};

PULSE_ECS_SINGLETON_COMPONENT
struct Score
{
	int value;
};

PULSE_ECS_COMPONENT
struct SnakeInput
{
	SDL_Scancode rightKey, upKey, leftKey, downKey;
};

struct SnakeBody
{
	HMM_Vec3 position;
	flecs::entity entity;
};

PULSE_ECS_COMPONENT
struct SnakeBodies
{
	std::vector<SnakeBody> bodies;
};

PULSE_ECS_TAG
struct IsApple {};

PULSE_ECS_EVENT
struct AppleEatenEvent
{
	flecs::entity apple;
};

PULSE_ECS_EVENT
struct SnakeMoveIntentEvent
{
	HMM_Vec3 delta;
};

PULSE_ECS_EVENT
struct GameOverEvent {};

PULSE_ECS_SINGLETON_COMPONENT
struct SnakeResources
{
	int quad;
	int appleMat, snakeHeadMat, snakeBodyMat;
	int boardMat;
};

PULSE_ECS_SYSTEM
void initSnakeGameSystem(pulse::command_buffer& command_buffer, pulse::singleton_query<const SnakeResources> resources);

PULSE_ECS_SYSTEM
void handleSnakeInputSystem(pulse::res<const KeyboardState> keyboardState, const SnakeInput& input, Facing4W& direction, SnakeMove& move);

PULSE_ECS_SYSTEM
void scheduleSnakeMoveSystem(pulse::res<const SystemContext> context, pulse::event_writer<SnakeMoveIntentEvent> snakeMoveIntentEvent, flecs::entity entity, const Facing4W& direction, SnakeMove& move);

PULSE_ECS_SYSTEM
void executeSnakeMoveSystem(pulse::event_reader<SnakeMoveIntentEvent> snakeMoveIntentEvent, pulse::command_buffer& command_buffer, flecs::query<const IsApple, const Position>& appleQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources, pulse::event_writer<AppleEatenEvent> appleEatenEvent, pulse::event_writer<GameOverEvent> gameOverEvent, SnakeBodies& snake);

PULSE_ECS_SYSTEM
void syncSnakeBodyPositionSystem(SnakeBodies& snake);

PULSE_ECS_SYSTEM
void eatAppleSystem(pulse::event_reader<AppleEatenEvent> appleEatenEvent, pulse::command_buffer& command_buffer);

PULSE_ECS_SYSTEM
void increaseScoreSystem(pulse::event_reader<AppleEatenEvent> appleEatenEvent, Score& score);

PULSE_ECS_SYSTEM
void spawnAppleSystem(pulse::event_reader<AppleEatenEvent> appleEatenEvent, pulse::command_buffer& command_buffer, flecs::query<const SnakeBodies>& snakeQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources);

PULSE_ECS_SYSTEM
void onGameOverSystem(pulse::event_reader<GameOverEvent> gameOverEvent, pulse::command_buffer& command_buffer, flecs::query<SnakeBodies>& snakeQuery, flecs::query<IsApple>& appleQuery);

PULSE_ECS_SYSTEM
void restartSystem(pulse::command_buffer& command_buffer, pulse::singleton_query<const Border> borderQuery, pulse::singleton_query<const SnakeResources> resources);
