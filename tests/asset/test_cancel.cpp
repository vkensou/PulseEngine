#include "test_common.h"

const uint64_t self_cancel_type = 9;
const uint64_t dtor_unload_type = 10;

struct self_cancel_asset {
    int value;
};

struct dtor_unload_asset {
    int value;
};

static int self_cancel_step_count = 0;
static int self_cancel_ctor_count = 0;
static int self_cancel_dtor_count = 0;
static int dtor_unload_step_count = 0;
static int dtor_unload_dtor_count = 0;

static EPulseResult ctor_self_cancel_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)state;
    self_cancel_ctor_count += 1;
    assert(ctx->dependency_hint == nullptr);
    assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, ctx->request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
    return PULSE_RESULT_OK;
}

static void dtor_self_cancel_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)state;
    self_cancel_dtor_count += 1;
    assert(ctx->dependency_hint == nullptr);
    assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, ctx->request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
}

static EPulseAssetLoaderStatus step_self_cancel_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    self_cancel_step_count += 1;
    assert(ctx->dependency_hint != nullptr);
    pulse_asset_system_cancel(ctx->asset_system, ctx->request);
    self_cancel_asset* asset = (self_cancel_asset*)ctx->out_asset;
    asset->value = 101;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static void dtor_unload_asset_loader(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)state;
    dtor_unload_dtor_count += 1;
    assert(ctx->dependency_hint == nullptr);
    pulse_asset_system_cancel(ctx->asset_system, ctx->request);
}

static EPulseAssetLoaderStatus step_dtor_unload_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    dtor_unload_step_count += 1;
    dtor_unload_asset* asset = (dtor_unload_asset*)ctx->out_asset;
    asset->value = 202;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-cancel",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    asset_test_add_root("tests/asset/data");
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    const char parent_bytes[] = "parent";

    PulseAssetTypeDesc self_cancel_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        self_cancel_type,
        sizeof(self_cancel_asset),
        alignof(self_cancel_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &self_cancel_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc self_cancel_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        self_cancel_type,
        "txt",
        nullptr,
        ctor_self_cancel_asset,
        dtor_self_cancel_asset,
        step_self_cancel_asset,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &self_cancel_loader_desc) == PULSE_RESULT_OK);

    PulseAssetTypeDesc dtor_unload_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        dtor_unload_type,
        sizeof(dtor_unload_asset),
        alignof(dtor_unload_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &dtor_unload_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc dtor_unload_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        dtor_unload_type,
        "txt",
        nullptr,
        nullptr,
        dtor_unload_asset_loader,
        step_dtor_unload_asset,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &dtor_unload_loader_desc) == PULSE_RESULT_OK);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    PulseAssetRequest self_cancel_handle = load_asset_memory(assetSystem, self_cancel_type, "self_cancel.txt", parent_bytes, 6, nullptr);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(self_cancel_ctor_count == 1);
    assert(self_cancel_step_count == 1);
    assert(self_cancel_dtor_count == 1);
    assert(pulse_asset_system_get_state(assetSystem, self_cancel_handle) == PULSE_ASSET_STATE_EMPTY);

    PulseAssetRequest dtor_unload_handle = load_asset_memory(assetSystem, dtor_unload_type, "dtor_unload.txt", parent_bytes, 6, nullptr);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(dtor_unload_step_count == 1);
    assert(dtor_unload_dtor_count == 1);
    assert(pulse_asset_system_get_state(assetSystem, dtor_unload_handle) == PULSE_ASSET_STATE_EMPTY);

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Cancel/unload asset test passed!\n");
    return 0;
}
