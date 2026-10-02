#include "font_internal.h"

namespace pulse_font_internal {

namespace {

const char* chain_path(pulse_font_plugin_state* state, PulseFontChainHandle chain) {
    if (!state || !state->asset_system || chain.index == 0 || chain.generation == 0) {
        return nullptr;
    }
    return pulse_asset_system_get_path(state->asset_system, pulse_font_chain_to_handle(chain));
}

int serialize_font_chain_handle(const flecs::serializer* ser, const PulseFontChainHandle* chain) {
    pulse_font_plugin_state* state = state_from_world(const_cast<ecs_world_t*>(ser->world));
    const char* path = chain_path(state, *chain);
    const char* value = path ? path : "";
    return ser->value(ecs_id(ecs_string_t), &value);
}

void assign_font_chain_handle(PulseFontChainHandle* dst, ecs_world_t* world, const char* value) {
    dst->index = 0;
    dst->generation = 0;
    pulse_font_plugin_state* state = state_from_world(world);
    if (!state || !state->asset_system || !value || !value[0]) {
        return;
    }
    const PulseAssetHandle found = pulse_asset_system_find_loaded(state->asset_system, PULSE_TYPE_FONT_CHAIN, value);
    if (!pulse_asset_handle_is_valid(found)) {
        return;
    }
    dst->index = found.index;
    dst->generation = found.generation;
}

} // namespace

void register_font_asset_reflection(ecs_world_t* world) {
    flecs::opaque<PulseFontChainHandle>(world).as_type(ecs_id(ecs_string_t)).serialize(serialize_font_chain_handle).assign_string(assign_font_chain_handle).user_data(PULSE_TYPE_FONT_CHAIN);
}

} // namespace pulse_font_internal
