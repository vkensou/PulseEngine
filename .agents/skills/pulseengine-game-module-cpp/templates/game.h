#pragma once

#include <string>

#include "pulse_cpp_gameplay.h"

#include "pulse_app.h"
#include "pulse_math.h"
#include "pulse_transform.h"
#include "pulse_renderer.h"
#include "pulse_window.h"
#include "pulse_input.h"
#include "pulse_prefab.h"
#include "pulse_datatable.h"

#include "tables_generated.h"

PULSE_ECS_STATE_MACHINE(INIT=UnInitialized)
enum class <Game>GameState
{
	UnInitialized,
	Loading,
	LoadFailed,
	Playing,
	GameOver,
};

PULSE_ECS_COMPONENT
struct <Game>Ball
{
	HMM_Vec3 velocity;
};

PULSE_ECS_TAG
struct <Game>Player {};

PULSE_ECS_SINGLETON_COMPONENT
struct <Game>Field
{
	int left, right, bottom, top;
};

PULSE_ECS_SINGLETON_COMPONENT
struct <Game>Config
{
	float ballSpeed;
	int winScore;
};

PULSE_ECS_SINGLETON_COMPONENT
struct <Game>Score
{
	int value;
};

PULSE_ECS_SINGLETON_COMPONENT
struct <Game>Prefabs
{
	PulsePrefabHandle board, ball;
};

PULSE_ECS_EVENT
struct <Game>ScoredEvent
{
	int points;
};

PULSE_ECS_EVENT
struct <Game>GameOverEvent {};

PULSE_ECS_EVENT
struct <Game>RestartEvent {};

PULSE_ECS_RESOURCE
struct <Game>Assets
{
	PulsePrefabRequest board, ball;
	PulseAssetRequest config;
};

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=<Game>GameState::UnInitialized|<Game>GameState::Loading)
void load<Game>ResourcesSystem(PulseAppId app, pulse::res<<Game>Assets> assets, pulse::system_state_machine<<Game>GameState> state, pulse::command_buffer& command_buffer, flecs::query<PulseWindow, PulsePrimaryWindow>& primaryWindowQuery);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=<Game>GameState::Playing)
void move<Game>BallSystem(pulse::res<const PulseTimer> timer, pulse::event_writer<<Game>ScoredEvent> scoredEvent, pulse::singleton_query<const <Game>Field>& fieldQuery, <Game>Ball& ball, PulseLocalTransform& transform);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=<Game>GameState::Playing)
void on<Game>ScoredSystem(pulse::event_reader<<Game>ScoredEvent> scoredEvent, pulse::event_writer<<Game>GameOverEvent> gameOverEvent, pulse::singleton_query<const <Game>Config>& configQuery, <Game>Score& score);

PULSE_ECS_SYSTEM(PHASE=UPDATE, STATE=<Game>GameState::Playing)
void on<Game>GameOverSystem(pulse::event_reader<<Game>GameOverEvent> gameOverEvent, pulse::command_buffer& command_buffer, pulse::system_state_machine<<Game>GameState> state, flecs::query<<Game>Ball>& ballQuery);

PULSE_ECS_SYSTEM(PHASE=IMGUI, STATE=<Game>GameState::Playing|<Game>GameState::GameOver)
void <game>UISystem(PulseAppId app, const <Game>Score& score, flecs::query<PulseWindow, PulsePrimaryWindow>& primaryWindowQuery, pulse::system_state_machine<<Game>GameState> state, pulse::event_writer<<Game>RestartEvent> restartEvent);

PULSE_ECS_SYSTEM(PHASE=IMGUI)
void <game>FpsUISystem(pulse::res<const PulseTimer> timer);

PULSE_ECS_SYSTEM(PHASE=IMGUI, STATE=<Game>GameState::GameOver)
void restart<Game>System(pulse::event_reader<<Game>RestartEvent> restartEvent, pulse::command_buffer& command_buffer, PulseAppId app, pulse::system_state_machine<<Game>GameState> state, pulse::singleton_query<const <Game>Field>& fieldQuery, pulse::singleton_query<const <Game>Prefabs>& prefabsQuery, pulse::singleton_query<const <Game>Config>& configQuery);
