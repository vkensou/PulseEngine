#pragma once

#include "pulse_prefab.h"
#include "pulse_asset.h"

#include <new>
#include <string>
#include <vector>

namespace pulse_prefab_internal {

extern const char* kPluginName;

struct prefab_reference {
    uint64_t type_id = 0;
    std::string path;
};

struct prefab_reference_set {
    std::string json;
    std::vector<prefab_reference> references;
};

struct prefab_load_state {
    prefab_reference_set* references = nullptr;
};

void register_prefab_type(PulseAssetSystemId asset_system, PulseAppId app);
void register_prefab_load_loader(PulseAssetSystemId asset_system);

EPulsePluginBuildResult prefab_plugin_build(PulseAppId app, void* ctx);
EPulsePluginBuildResult prefab_plugin_post_build(PulseAppId app, void* ctx);
void prefab_plugin_shutdown(PulseAppId app, void* ctx);

} // namespace pulse_prefab_internal
