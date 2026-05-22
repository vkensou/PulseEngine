#include "asset_internal.h"

#include <SDL3/SDL.h>

namespace pulse_asset_internal {

static std::string extension_from_path(const std::string& path) {
    size_t slash = path.find_last_of("/");
    size_t dot = path.find_last_of('.');
    if (dot == std::string::npos || (slash != std::string::npos && dot < slash)) {
        return "";
    }
    return normalize_extension(path.c_str() + dot + 1);
}

AssetLoader* find_loader(pulse_asset_state_o* state, uint64_t type_id, const std::string& path) {
    std::string extension = extension_from_path(path);
    if (extension.empty()) {
        return nullptr;
    }
    for (AssetLoader& loader : state->loaders) {
        if (loader.desc.type_id != type_id) {
            continue;
        }
        for (const std::string& candidate : loader.extensions) {
            if (candidate == extension) {
                return &loader;
            }
        }
    }
    return nullptr;
}

std::string join_asset_path(const std::string& root_path, const std::string& path) {
    if (root_path.empty()) {
        return path;
    }
    if (root_path.back() == '/' || root_path.back() == '\\') {
        return root_path + path;
    }
    return root_path + "/" + path;
}

std::optional<std::vector<uint8_t>> read_file_sdl(const char* filename) {
    SDL_IOStream* stream = SDL_IOFromFile(filename, "rb");
    if (!stream) {
        return std::optional<std::vector<uint8_t>>{};
    }
    Sint64 size = SDL_GetIOSize(stream);
    if (size < 0) {
        SDL_CloseIO(stream);
        return std::optional<std::vector<uint8_t>>{};
    }
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
    size_t read = SDL_ReadIO(stream, buffer.data(), buffer.size());
    SDL_CloseIO(stream);
    if (read != buffer.size()) {
        return std::optional<std::vector<uint8_t>>{};
    }
    return buffer;
}

void process_load_requests_system(ecs_iter_t* it) {
    pulse_asset_state_o* state = static_cast<pulse_asset_state_o*>(it->ctx);
    if (!state) {
        return;
    }

    uint32_t processed = 0;
    while (processed < state->desc.max_requests_per_update && !state->pending_requests.empty()) {
        LoadRequest request = state->pending_requests.front();
        state->pending_requests.pop_front();
        processed += 1;

        AssetSlot* slot = get_slot(state, request.handle);
        if (!slot || slot->state != PULSE_ASSET_STATE_WAITING_LOAD) {
            continue;
        }

        slot->state = PULSE_ASSET_STATE_LOADING;
        AssetLoader* loader = find_loader(state, request.handle.type_id, slot->path);
        if (!loader) {
            slot->state = PULSE_ASSET_STATE_FAILED;
            slot->error = "no loader registered for asset extension";
            continue;
        }

        std::string full_path = join_asset_path(state->root_path, slot->path);
        auto bytes = read_file_sdl(full_path.c_str());
        if (!bytes.has_value()) {
            slot->state = PULSE_ASSET_STATE_FAILED;
            slot->error = "failed to read asset file";
            continue;
        }

        slot->loader = loader;
        ActiveLoad active{};
        active.handle = request.handle;
        active.bytes = std::move(bytes.value());
        active.ctx.app = state->app;
        active.ctx.type_id = request.handle.type_id;
        active.ctx.path = slot->path.c_str();
        active.ctx.bytes = active.bytes.data();
        active.ctx.byte_size = static_cast<uint64_t>(active.bytes.size());

        void* loader_state = nullptr;
        pulse_result_t begin_result = loader->desc.start(&active.ctx, &loader_state, loader->desc.user_data);
        if (begin_result != PULSE_OK) {
            slot->state = PULSE_ASSET_STATE_FAILED;
            slot->error = "asset loader begin failed";
            continue;
        }
        slot->loader_state = loader_state;
        slot->state = PULSE_ASSET_STATE_PROCESSING;
        state->active_loads.push_back(std::move(active));
    }

    for (size_t i = 0; i < state->active_loads.size();) {
        ActiveLoad& active = state->active_loads[i];
        AssetSlot* slot = get_slot(state, active.handle);
        if (!slot || slot->state != PULSE_ASSET_STATE_PROCESSING || !slot->loader) {
            state->active_loads.erase(state->active_loads.begin() + i);
            continue;
        }

        active.ctx.path = slot->path.c_str();
        active.ctx.bytes = active.bytes.data();
        active.ctx.byte_size = static_cast<uint64_t>(active.bytes.size());

        const char* error = nullptr;
        pulse_asset_loader_status_t status = slot->loader->desc.step(
            &active.ctx,
            slot->loader_state,
            slot->data,
            &error,
            slot->loader->desc.user_data
        );

        if (status == PULSE_ASSET_LOADER_PENDING) {
            ++i;
            continue;
        }

        if (status == PULSE_ASSET_LOADER_DONE) {
            slot->constructed = true;
            slot->state = PULSE_ASSET_STATE_LOADED;
            slot->error.clear();
        } else {
            slot->state = PULSE_ASSET_STATE_FAILED;
            slot->error = error ? error : "asset loader step failed";
        }

        if (slot->loader->desc.finally && slot->loader_state) {
            slot->loader->desc.finally(slot->loader_state, slot->loader->desc.user_data);
        }

        slot->loader_state = nullptr;
        state->active_loads.erase(state->active_loads.begin() + i);
    }
}

void install_process_system(pulse_asset_state_o* state, ecs_world_t* world) {
    ecs_entity_desc_t entity_desc{};
    entity_desc.name = "PulseAssetProcessLoadRequests";

    ecs_system_desc_t system_desc{};
    system_desc.entity = ecs_entity_init(world, &entity_desc);
    system_desc.phase = EcsOnLoad;
    system_desc.callback = process_load_requests_system;
    system_desc.ctx = state;
    state->process_system = ecs_system_init(world, &system_desc);
}

void uninstall_process_system(pulse_asset_state_o* state, ecs_world_t* world) {
    if (state && world && state->process_system && ecs_is_alive(world, state->process_system)) {
        ecs_delete(world, state->process_system);
    }
    if (state) {
        state->process_system = 0;
    }
}

void cancel_active_loads(pulse_asset_state_o* state) {
    if (!state) {
        return;
    }
    for (ActiveLoad& active : state->active_loads) {
        AssetSlot* slot = get_slot(state, active.handle);
        if (slot && slot->loader && slot->loader->desc.finally && slot->loader_state) {
            slot->loader->desc.finally(slot->loader_state, slot->loader->desc.user_data);
            slot->loader_state = nullptr;
            slot->state = PULSE_ASSET_STATE_FAILED;
            slot->error = "asset load cancelled";
        }
    }
    state->active_loads.clear();
    state->pending_requests.clear();
}

} // namespace pulse_asset_internal