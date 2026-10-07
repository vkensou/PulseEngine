#pragma once

#include "pulse_prefab.h"
#include "pulse_asset.h"
#include "pulse_datalist.h"

#include <new>
#include <string>
#include <vector>

namespace pulse_prefab_internal {

extern const char* kPluginName;

struct prefab_definition {
    std::string name;
    const PulseDatalist* node = nullptr;
    ecs_entity_t entity = 0;
    size_t parent = SIZE_MAX;
    size_t base = SIZE_MAX;
    bool expanded = false;
};

struct prefab_library {
    std::vector<prefab_definition> definitions;
    size_t root = SIZE_MAX;
    bool single_root = false;
};

struct prefab_load_state {
    PulseDatalist* datalist = nullptr;
    prefab_library* library = nullptr;
    bool references_ready = false;
};

inline PulseAssetRequest request_file_load(PulseAssetSystemId asset_system, uint64_t type_id, const char* path) {
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    return pulse_asset_system_load(asset_system, &desc);
}

inline bool asset_state_is_pending(EPulseAssetState state) {
    return state == PULSE_ASSET_STATE_WAITING_LOAD || state == PULSE_ASSET_STATE_LOADING || state == PULSE_ASSET_STATE_WAITING_DEPENDENCIES || state == PULSE_ASSET_STATE_PROCESSING;
}

void register_prefab_type(PulseAssetSystemId asset_system, PulseAppId app);
void register_prefab_load_loader(PulseAssetSystemId asset_system);
void register_prefab_script_type(PulseAssetSystemId asset_system, PulseAppId app);
void register_prefab_script_loader(PulseAssetSystemId asset_system);

EPulsePluginBuildResult prefab_plugin_build(PulseAppId app, void* ctx);
EPulsePluginBuildResult prefab_plugin_post_build(PulseAppId app, void* ctx);
void prefab_plugin_shutdown(PulseAppId app, void* ctx);

} // namespace pulse_prefab_internal
