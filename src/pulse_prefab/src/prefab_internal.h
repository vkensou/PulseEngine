#pragma once

#include "pulse_prefab.h"
#include "pulse_asset.h"
#include "pulse_datalist.h"

#include <new>

namespace pulse_prefab_internal {

extern const char* kPluginName;

struct prefab_load_state {
    PulseDatalist* datalist = nullptr;
    bool references_ready = false;
};

void register_prefab_type(PulseAssetSystemId asset_system, PulseAppId app);
void register_prefab_load_loader(PulseAssetSystemId asset_system);

EPulsePluginBuildResult prefab_plugin_build(PulseAppId app, void* ctx);
EPulsePluginBuildResult prefab_plugin_post_build(PulseAppId app, void* ctx);
void prefab_plugin_shutdown(PulseAppId app, void* ctx);

} // namespace pulse_prefab_internal
