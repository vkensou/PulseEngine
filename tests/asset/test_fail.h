// Shared "fail" asset definitions: cut from tests/asset/test_common.h.
// Used by the fail and dependencies tests.
#pragma once

#include "test_common.h"

const uint64_t fail_type = 3;

struct fail_asset {
    int unused;
};

struct fail_loader_state {
    int marker;
};

static EPulseAssetLoaderStatus step_fail_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    fail_loader_state* s = (fail_loader_state*)state;
    assert(s->marker == 0);
    (void)ctx;
    *out_error = "intentional failure";
    return PULSE_ASSET_LOADER_STATUS_FAILED;
}