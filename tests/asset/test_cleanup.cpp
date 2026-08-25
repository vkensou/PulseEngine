#include "test_common.h"

const uint64_t cleanup_type = 5;

struct cleanup_asset {
    int value;
};

struct cleanup_loader_state {
    int step;
    int fail;
};

static int cleanup_ctor_count = 0;
static int cleanup_dtor_count = 0;

static EPulseResult ctor_cleanup_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    cleanup_ctor_count += 1;
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    assert(s->step == 0);
    assert(s->fail == 0);
    s->fail = ctx->settings ? *((const int*)ctx->settings) : 0;
    return PULSE_RESULT_OK;
}

static void dtor_cleanup_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)ctx;
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    assert(s != nullptr);
    cleanup_dtor_count += 1;
}

static EPulseAssetLoaderStatus step_cleanup_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    s->step += 1;
    if (s->fail) {
        *out_error = "cleanup loader intentional failure";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    if (s->step < 2) {
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }
    cleanup_asset* asset = (cleanup_asset*)ctx->out_asset;
    asset->value = 9;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-cleanup",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    asset_test_add_root("tests/asset/data");
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    const char hello_bytes[] = "hello world";
    const char cleanup_bytes[] = "cleanup";

    PulseAssetTypeDesc cleanup_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        cleanup_type,
        sizeof(cleanup_asset),
        alignof(cleanup_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &cleanup_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc cleanup_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        cleanup_type,
        "txt",
        nullptr,
        ctor_cleanup_asset,
        dtor_cleanup_asset,
        step_cleanup_asset,
        sizeof(cleanup_loader_state),
        alignof(cleanup_loader_state),
        sizeof(int),
        alignof(int),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &cleanup_loader_desc) == PULSE_RESULT_OK);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    int cleanup_success = 0;
    PulseAssetRequest cleanup_done = load_asset_memory(assetSystem, cleanup_type, "cleanup_done.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_done) == PULSE_ASSET_STATE_PROCESSING);
    assert(cleanup_ctor_count == 1);
    assert(cleanup_dtor_count == 0);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_done) == PULSE_ASSET_STATE_LOADED);
    assert(cleanup_dtor_count == 1);

    int cleanup_failure = 1;
    PulseAssetRequest cleanup_failed = load_asset_memory(assetSystem, cleanup_type, "cleanup_fail.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_failure);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_failed) == PULSE_ASSET_STATE_FAILED);
    assert(cleanup_dtor_count == 2);

    PulseAssetRequest cleanup_unload = load_asset_memory(assetSystem, cleanup_type, "cleanup_unload.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_unload) == PULSE_ASSET_STATE_PROCESSING);
    pulse_asset_system_cancel(assetSystem, cleanup_unload);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_unload) == PULSE_ASSET_STATE_PENDING_DELETE);
    assert(cleanup_dtor_count == 2);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_unload) == PULSE_ASSET_STATE_EMPTY);
    assert(cleanup_dtor_count == 3);

    PulseAssetRequest cleanup_shutdown = load_asset_memory(assetSystem, cleanup_type, "cleanup_shutdown.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_shutdown) == PULSE_ASSET_STATE_PROCESSING);
    assert(cleanup_dtor_count == 3);

    pulse_app_teardown(app);
    assert(cleanup_dtor_count == 4);
    pulse_destroy_app(app);

    printf("Cleanup asset test passed!\n");
    return 0;
}
