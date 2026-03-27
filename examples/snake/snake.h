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

struct AppleEat {};

struct GameOver {};

struct SnakeResources
{
	int quad;
	int appleMat, snakeHeadMat, snakeBodyMat;
	int boardMat;
};

void createSnakeGame(flecs::world& world, int up, int bottom, int left, int right);

void snakeInput(pulse::res<const KeyboardState> keyboardState, const SnakeInput& input, Direction& direction, SnakeMove& move);

void snakeMove(flecs::world& world, pulse::res<const SystemContext> context, flecs::query<const IsApple, const Position> apple, pulse::singleton_query<const Border> border, pulse::singleton_query<const SnakeResources> resources, Snake& snake);

void eatApple(flecs::entity apple, const IsApple&);

void addScore(Score& score);

void createAppleSystem(flecs::world& world, pulse::singleton_query<const SnakeResources> resources);

void gameover(flecs::world& world, flecs::entity entity);

void restart(flecs::world& world, pulse::singleton_query<const SnakeResources> resources);
