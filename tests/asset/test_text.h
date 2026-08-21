// Shared "text" asset definitions: cut from tests/asset/test_common.h.
// Used by the basic, fail, dependencies, and builders tests.
#pragma once

#include "test_common.h"

const uint64_t text_type = 1;

struct test_text_asset {
    uint64_t size;
    char text[64];
};

static int destroy_count = 0;

static void destroy_test_text(void* ptr, void* user_data) {
    (void)ptr;
    int* counter = (int*)user_data;
    *counter += 1;
}

static EPulseAssetLoaderStatus step_test_text(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    test_text_asset* asset = (test_text_asset*)ctx->out_asset;
    asset->size = ctx->byte_size;
    uint64_t copy_size = ctx->byte_size < 63 ? ctx->byte_size : 63;
    for (uint64_t i = 0; i < copy_size; ++i) {
        asset->text[i] = (char)ctx->bytes[i];
    }
    asset->text[copy_size] = '\0';
    return PULSE_ASSET_LOADER_STATUS_DONE;
}