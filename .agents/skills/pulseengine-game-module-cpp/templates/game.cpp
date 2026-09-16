#include "<game>.h"

#include <cstdio>
#include <string>

#include "imgui.h"

namespace
{

float absolute(float value)
{
	return value < 0.f ? -value : value;
}

flecs::entity instantiatePrefab(PulseAppId app, PulsePrefabHandle prefab, HMM_Vec3 position, HMM_Vec3 scale)
{
	flecs::entity entity(pulse_app_world(app), pulse_prefab_instantiate(app, prefab));
	entity.set<PulseLocalTransform>({ .translation = position, .rotation = HMM_Q_Identity, .scale = scale });
	return entity;
}

void createFieldEntities(PulseAppId app, const <Game>Prefabs& prefabs, const <Game>Field& field)
{
	float centerX = (field.left + field.right) * 0.5f;
	float centerY = (field.bottom + field.top) * 0.5f;
	float width = (float)(field.right - field.left + 1);
	float height = (float)(field.top - field.bottom + 1);
	instantiatePrefab(app, prefabs.board, HMM_V3((float)field.right, centerY, 0.f), HMM_V3(1.f, height, 1.f));
	instantiatePrefab(app, prefabs.board, HMM_V3((float)field.left, centerY, 0.f), HMM_V3(1.f, height, 1.f));
	instantiatePrefab(app, prefabs.board, HMM_V3(centerX, (float)field.top, 0.f), HMM_V3(width, 1.f, 1.f));
	instantiatePrefab(app, prefabs.board, HMM_V3(centerX, (float)field.bottom, 0.f), HMM_V3(width, 1.f, 1.f));
}

void createBallEntity(PulseAppId app, const <Game>Prefabs& prefabs, float ballSpeed)
{
	flecs::entity entity = instantiatePrefab(app, prefabs.ball, HMM_V3(0.f, 0.f, 0.f), HMM_V3(2.f, 2.f, 1.f));
	entity.set<<Game>Ball>({ .velocity = HMM_V3(ballSpeed, ballSpeed, 0.f) });
}

void createEntities(pulse::command_buffer& command_buffer, PulseAppId app, const <Game>Field& field, const <Game>Prefabs& prefabs, float ballSpeed)
{
	command_buffer.set_singleton<<Game>Score>({ .value = 0 });
	createFieldEntities(app, prefabs, field);
	createBallEntity(app, prefabs, ballSpeed);
}

} // namespace

void load<Game>ResourcesSystem(PulseAppId app, pulse::res<<Game>Assets> assets, pulse::system_state_machine<<Game>GameState> state, pulse::command_buffer& command_buffer, flecs::query<PulseWindow, PulsePrimaryWindow>& primaryWindowQuery)
{
	auto& as = assets.get();

	if (state.is(<Game>GameState::UnInitialized))
	{
		PulseDataTableSystemId tables = pulse_get_data_table_system(app);
		if (!tables || pulse_tables::RegisterSchemas(tables) != PULSE_RESULT_OK)
		{
			printf("<Game> config schema registration failed\n");
			state.to(<Game>GameState::LoadFailed);
			return;
		}

		as.board = pulse_load_prefab(app, "assets/board.prefab");
		as.ball = pulse_load_prefab(app, "assets/ball.prefab");
		as.config = pulse_tables::Pulse<Game>ConfigRowTable::Load(app, "assets/<game>_config.datatable");
		state.to(<Game>GameState::Loading);
		return;
	}

	struct PrefabLoad
	{
		PulsePrefabRequest request;
		const char* path;
	};
	const PrefabLoad loads[] = {
		{ as.board, "assets/board.prefab" },
		{ as.ball, "assets/ball.prefab" },
	};

	PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
	for (const PrefabLoad& load : loads)
	{
		PulseAssetRequest request = pulse_prefab_request_to_asset_request(load.request);
		if (pulse_asset_system_get_state(assetSystem, request) == PULSE_ASSET_STATE_FAILED)
		{
			printf("<Game> prefab load failed: %s: %s\n", load.path, pulse_asset_system_get_error(assetSystem, request));
			state.to(<Game>GameState::LoadFailed);
			return;
		}
	}

	if (pulse_asset_system_get_state(assetSystem, as.config) == PULSE_ASSET_STATE_FAILED)
	{
		printf("<Game> config load failed: %s\n", pulse_tables::Pulse<Game>ConfigRowTable::GetError(app));
		state.to(<Game>GameState::LoadFailed);
		return;
	}

	for (const PrefabLoad& load : loads)
	{
		if (!pulse_prefab_is_ready(app, load.request))
			return;
	}

	if (!pulse_tables::Pulse<Game>ConfigRowTable::IsReady(app))
		return;

	const pulse_tables::Pulse<Game>ConfigRow* configRow = pulse_tables::Pulse<Game>ConfigRowTable::GetRow(app, "default");
	if (!configRow)
	{
		printf("<Game> config row 'default' is missing\n");
		state.to(<Game>GameState::LoadFailed);
		return;
	}

	<Game>Prefabs prefabs = {
		.board = pulse_prefab_get_handle(app, as.board),
		.ball = pulse_prefab_get_handle(app, as.ball),
	};
	command_buffer.set_singleton<<Game>Prefabs>(prefabs);
	command_buffer.set_singleton<<Game>Config>({ .ballSpeed = (float)configRow->ball_speed, .winScore = (int)configRow->win_score });

	auto windowEntity = primaryWindowQuery.first();

	const float cameraZ = -20.f;
	const float cameraFov = 45.f;

	<Game>Field field = { .left = -6, .right = 6, .bottom = -4, .top = 4 };
	command_buffer.set_singleton<<Game>Field>(field);

	createEntities(command_buffer, app, field, prefabs, (float)configRow->ball_speed);

	auto camera = command_buffer.entity();
	camera.set<PulseLocalTransform>({ .translation = HMM_V3(0.f, 0.f, cameraZ), .rotation = HMM_Q_Identity, .scale = HMM_V3_One });
	camera.set<PulseCamera>({ .window_entity = windowEntity, .fov = cameraFov, .near_plane = 0.1f, .far_plane = 1000.f });

	state.to(<Game>GameState::Playing);
	printf("<Game> resources ready.\n");
}

void move<Game>BallSystem(pulse::res<const PulseTimer> timer, pulse::event_writer<<Game>ScoredEvent> scoredEvent, pulse::singleton_query<const <Game>Field>& fieldQuery, <Game>Ball& ball, PulseLocalTransform& transform)
{
	const <Game>Field& field = fieldQuery.get();
	HMM_Vec3 next = transform.translation + ball.velocity * timer.get().delta_time;
	bool bounced = false;

	if (next.X <= (float)field.left + 1.f)
	{
		next.X = (float)field.left + 1.f;
		ball.velocity.X = absolute(ball.velocity.X);
		bounced = true;
	}
	else if (next.X >= (float)field.right - 1.f)
	{
		next.X = (float)field.right - 1.f;
		ball.velocity.X = -absolute(ball.velocity.X);
		bounced = true;
	}

	if (next.Y <= (float)field.bottom + 1.f)
	{
		next.Y = (float)field.bottom + 1.f;
		ball.velocity.Y = absolute(ball.velocity.Y);
		bounced = true;
	}
	else if (next.Y >= (float)field.top - 1.f)
	{
		next.Y = (float)field.top - 1.f;
		ball.velocity.Y = -absolute(ball.velocity.Y);
		bounced = true;
	}

	transform.translation = next;

	if (bounced)
		scoredEvent.broadcast({ .points = 1 });
}

void on<Game>ScoredSystem(pulse::event_reader<<Game>ScoredEvent> scoredEvent, pulse::event_writer<<Game>GameOverEvent> gameOverEvent, pulse::singleton_query<const <Game>Config>& configQuery, <Game>Score& score)
{
	score.value += scoredEvent.read().points;

	if (score.value >= configQuery.get().winScore)
		gameOverEvent.broadcast();
}

void on<Game>GameOverSystem(pulse::event_reader<<Game>GameOverEvent> gameOverEvent, pulse::command_buffer& command_buffer, pulse::system_state_machine<<Game>GameState> state, flecs::query<<Game>Ball>& ballQuery)
{
	ballQuery.each([&](flecs::entity entity, <Game>Ball&)
		{
			command_buffer.destruct(entity);
		});

	state.to(<Game>GameState::GameOver);
}

void <game>UISystem(PulseAppId app, const <Game>Score& score, flecs::query<PulseWindow, PulsePrimaryWindow>& primaryWindowQuery, pulse::system_state_machine<<Game>GameState> state, pulse::event_writer<<Game>RestartEvent> restartEvent)
{
	auto windowEntity = primaryWindowQuery.first();
	std::string title = std::string("<GAME_NAME> - ") + std::to_string(score.value);
	pulse_window_set_title(app, windowEntity, title.c_str());

	if (!state.is(<Game>GameState::GameOver))
		return;

	ImGui::Text("Game Over! Final score: %d", score.value);

	if (ImGui::Button("Restart"))
		restartEvent.broadcast();
}

void <game>FpsUISystem(pulse::res<const PulseTimer> timer)
{
	ImGui::Text("FPS: %d", timer.get().fps);
}

void restart<Game>System(pulse::event_reader<<Game>RestartEvent> restartEvent, pulse::command_buffer& command_buffer, PulseAppId app, pulse::system_state_machine<<Game>GameState> state, pulse::singleton_query<const <Game>Field>& fieldQuery, pulse::singleton_query<const <Game>Prefabs>& prefabsQuery, pulse::singleton_query<const <Game>Config>& configQuery)
{
	command_buffer.defer_suspend();
	createEntities(command_buffer, app, fieldQuery.get(), prefabsQuery.get(), configQuery.get().ballSpeed);
	command_buffer.defer_resume();

	state.to(<Game>GameState::Playing);
}
