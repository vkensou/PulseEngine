#pragma once

#include "predefine_components.h"
#include "ecsext.hpp"
#include <SDL3/SDL.h>
#include <vector>

enum class Direction
{
	Right,
	Up,
	Left,
	Down,
};

struct SnakeMove
{
	float interval;
	float lastTime;
};

struct Border
{
	int up, bottom, left, right;
};

struct Score
{
	int value;
};

struct SnakeInput
{
	SDL_Scancode rightKey, upKey, leftKey, downKey;
};

struct Snake
{
	flecs::entity head;
	std::vector<flecs::entity> bodies;
};

struct IsApple {};

struct Game {};

struct AppleEat
{
	flecs::entity apple;
};

struct GameOver {};

struct SnakeResources
{
	int quad;
	int appleMat, snakeHeadMat, snakeBodyMat;
	int boardMat;
};

void initSnakeGame(pulse::command_buffer& command_buffer, pulse::singleton_query<const SnakeResources> resources);

void snakeInput(pulse::res<const KeyboardState> keyboardState, const SnakeInput& input, Direction& direction, SnakeMove& move);

void snakeMove(pulse::command_buffer& command_buffer, pulse::res<const SystemContext> context, flecs::query<const IsApple, const Position>& apple, pulse::singleton_query<const Border>& border, pulse::singleton_query<const SnakeResources>& resources, pulse::event_writer<AppleEat> appleEatWriter, pulse::event_writer<GameOver> gameOverWriter, Snake& snake);

void eatApple(pulse::event_reader<AppleEat> eventAppleEat, pulse::command_buffer& command_buffer);

void addScore(pulse::event_reader<AppleEat> eventAppleEat, Score& score);

void createAppleSystem(pulse::event_reader<AppleEat> eventAppleEat, pulse::command_buffer& command_buffer, flecs::query<const Snake>& snakeQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources);

void gameover(pulse::event_reader<GameOver> eventAppleEat, pulse::command_buffer& command_buffer, flecs::query<Snake>& snakeQuery, flecs::query<IsApple>& appleQuery);

void restart(pulse::command_buffer& command_buffer, pulse::singleton_query<const Border> borderQuery, pulse::singleton_query<const SnakeResources> resources);
