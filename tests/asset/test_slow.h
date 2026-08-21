// Shared "slow" asset definitions: cut from tests/asset/test_common.h.
// Used by the slow, dependencies, and builder-async tests.
#pragma once

#include "test_common.h"

const uint64_t slow_type = 2;

struct slow_asset {
    int value;
};

struct slow_loader_state {
    int step;
};

static EPulseAssetLoaderStatus step_slow_asset(
    void* raw_state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)ctx;
    (void)out_error;
    slow_loader_state* s = (slow_loader_state*)raw_state;
    s->step += 1;
    if (s->step < 2) {
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }
    slow_asset* asset = (slow_asset*)ctx->out_asset;
    asset->value = 42;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}