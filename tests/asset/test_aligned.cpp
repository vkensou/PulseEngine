#include "test_common.h"

const uint64_t aligned_type = 6;

struct alignas(64) aligned_asset {
    int value;
};

struct alignas(64) aligned_loader_state {
    int constructed;
};

struct alignas(64) aligned_loader_settings {
    int value;
};

static int aligned_ctor_count = 0;

static EPulseResult ctor_aligned_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)ctx;
    assert((((uintptr_t)state) % alignof(aligned_loader_state)) == 0);
    aligned_loader_state* s = (aligned_loader_state*)state;
    assert(s->constructed == 0);
    s->constructed = 1;
    aligned_ctor_count += 1;
    return PULSE_RESULT_OK;
}

static EPulseAssetLoaderStatus step_aligned_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)out_error;
    assert((((uintptr_t)state) % alignof(aligned_loader_state)) == 0);
    assert((((uintptr_t)ctx->settings) % alignof(aligned_loader_settings)) == 0);
    assert((((uintptr_t)ctx->out_asset) % alignof(aligned_asset)) == 0);
    aligned_asset* asset = (aligned_asset*)ctx->out_asset;
    const aligned_loader_settings* settings = (const aligned_loader_settings*)ctx->settings;
    asset->value = settings->value;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-aligned",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    const char hello_bytes[] = "hello world";

    PulseAssetTypeDesc aligned_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        aligned_type,
        sizeof(aligned_asset),
        alignof(aligned_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &aligned_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc aligned_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        aligned_type,
        "txt",
        nullptr,
        ctor_aligned_asset,
        nullptr,
        step_aligned_asset,
        sizeof(aligned_loader_state),
        alignof(aligned_loader_state),
        sizeof(aligned_loader_settings),
        alignof(aligned_loader_settings),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &aligned_loader_desc) == PULSE_RESULT_OK);

    aligned_loader_settings aligned_settings{123};
    PulseAssetRequest aligned_request = load_asset_memory(assetSystem, aligned_type, "aligned.txt", hello_bytes, 11, &aligned_settings);
    assert(pulse_app_prepare(app) == PULSE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, aligned_request) == PULSE_ASSET_STATE_LOADED);
    assert(aligned_ctor_count == 1);

    PulseAssetHandle aligned_handle = pulse_asset_system_get_handle(assetSystem, aligned_request);
    assert(pulse_asset_handle_is_valid(aligned_handle));
    assert(pulse_asset_system_retain(assetSystem, aligned_handle, nullptr));
    void* aligned_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, aligned_handle, &aligned_ptr, nullptr));
    assert(aligned_ptr != nullptr);
    assert((((uintptr_t)aligned_ptr) % alignof(aligned_asset)) == 0);
    assert(((aligned_asset*)aligned_ptr)->value == 123);
    assert(pulse_asset_system_release(assetSystem, aligned_handle, nullptr));

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Aligned asset test passed!\n");
    return 0;
}
