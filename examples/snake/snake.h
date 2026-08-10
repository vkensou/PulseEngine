#pragma once

#include "ecsext.hpp"

#include <SDL3/SDL.h>
#include <vector>

#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_graphics.h"
#include "pulse_transform.h"
#include "pulse_renderer.h"
#include "pulse_window.h"
#include "pulse_input.h"

// ============================================================
// 游戏状态机
//   UnInitialized 初始态（尚未发起资源加载）
//   Loading       异步加载请求已发起，每帧轮询
//   LoadFailed    加载失败（终态，仅打印错误）
//   Gaming        游戏中
//   GameOver      等待重启
// ============================================================

PULSE_ECS_STATE_MACHINE(INIT=UnInitialized)
enum class SnakeGameState
{
	UnInitialized,
	Loading,
	LoadFailed,
	Gaming,
	GameOver,
};

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
	PulseMeshHandle quad;
	PulseMaterialHandle appleMat, snakeHeadMat, snakeBodyMat;
	PulseMaterialHandle boardMat;
};

PULSE_ECS_EVENT
struct RestartEvent {};

// ============================================================
// ECS 资源（单例）
// ============================================================

// 异步加载请求容器：loadSnakeResourcesSystem 自行发起请求并持有结果，
// app 由系统参数注入（pulse_get_app_from_world）。
PULSE_ECS_RESOURCE
struct SnakeAssets
{
	PulseShaderRequest shader;
	PulseMeshRequest mesh;
};

// ============================================================
// 系统
// ============================================================

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::UnInitialized|SnakeGameState::Loading)
void loadSnakeResourcesSystem(PulseAppId app, pulse::res<SnakeAssets> assets, pulse::system_state_machine<SnakeGameState> state, pulse::command_buffer& command_buffer, flecs::query<PulseWindow, PulsePrimaryWindow>& primaryWindowQuery);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::Gaming)
void handleSnakeInputSystem(pulse::res<const PulseKeyboardInput> keyboard, const SnakeInput& input, Facing4W& direction, SnakeMove& move);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::Gaming)
void scheduleSnakeMoveSystem(pulse::res<const PulseTimer> timer, pulse::event_writer<SnakeMoveIntentEvent> snakeMoveIntentEvent, flecs::entity entity, const Facing4W& direction, SnakeMove& move);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::Gaming)
void executeSnakeMoveSystem(pulse::event_reader<SnakeMoveIntentEvent> snakeMoveIntentEvent, pulse::command_buffer& command_buffer, flecs::query<const IsApple, const PulseLocalTransform>& appleQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources, pulse::event_writer<AppleEatenEvent> appleEatenEvent, pulse::event_writer<GameOverEvent> gameOverEvent, SnakeBodies& snake);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::Gaming)
void syncSnakeBodyPositionSystem(SnakeBodies& snake);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::Gaming)
void eatAppleSystem(pulse::event_reader<AppleEatenEvent> appleEatenEvent, pulse::command_buffer& command_buffer);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::Gaming)
void increaseScoreSystem(pulse::event_reader<AppleEatenEvent> appleEatenEvent, Score& score);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::Gaming)
void spawnAppleSystem(pulse::event_reader<AppleEatenEvent> appleEatenEvent, pulse::command_buffer& command_buffer, flecs::query<const SnakeBodies>& snakeQuery, pulse::singleton_query<const Border>& borderQuery, pulse::singleton_query<const SnakeResources>& resources);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=SnakeGameState::Gaming)
void onGameOverSystem(pulse::event_reader<GameOverEvent> gameOverEvent, pulse::command_buffer& command_buffer, pulse::system_state_machine<SnakeGameState> state, flecs::query<SnakeBodies>& snakeQuery, flecs::query<IsApple>& appleQuery);

PULSE_ECS_SYSTEM(PHASE=IMGUI, STATE=SnakeGameState::GameOver)
void restartSystem(pulse::event_reader<RestartEvent> restartEvent, pulse::command_buffer& command_buffer, pulse::system_state_machine<SnakeGameState> state, pulse::singleton_query<const Border> borderQuery, pulse::singleton_query<const SnakeResources> resources);

PULSE_ECS_SYSTEM(PHASE=IMGUI, STATE=SnakeGameState::Gaming|SnakeGameState::GameOver)
void snakeUISystem(PulseAppId app, const Score& score, flecs::query<PulseWindow, PulsePrimaryWindow>& primaryWindowQuery, pulse::system_state_machine<SnakeGameState> state, pulse::res<const PulseKeyboardInput> keyboard, pulse::event_writer<RestartEvent> restartEvent);
