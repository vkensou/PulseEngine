#pragma once

#include "predefine_components.h"
#include "flecs.h"
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

struct SnakeApp {};

struct SnakeResources
{
	int quad;
	int appleMat, snakeHeadMat, snakeBodyMat;
	int boardMat;
};

struct SystemSnakeInputState
{
	std::vector<uint8_t> lastKeyboardStates;
};

struct SystemSnakeMoveState
{
	flecs::query<IsApple, Position> apple;
	flecs::query<Border> border;
};

void createSnakeGame(flecs::world& world, int up, int bottom, int left, int right);

void snakeInput(SystemSnakeInputState& systemSnakeInputState, const SnakeInput& input, Direction& direction, SnakeMove& move);

void snakeMove(flecs::world& world, const SystemContext& context, SystemSnakeMoveState& systemSnakeMoveState, const SnakeResources& resources, Snake& snake);

void eatApple(flecs::entity apple, const IsApple&);

void addScore(Score& score);

void createAppleSystem(flecs::world& world, const SnakeResources& resources);

void gameover(flecs::world& world, flecs::entity entity);

void restart(flecs::world& world, const SnakeResources& resources);
