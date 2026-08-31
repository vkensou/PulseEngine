#include "test_text.h"
#include "test_fail.h"

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-fail",
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

    const char hello_bytes[] = "hello world";

    PulseAssetTypeDesc text_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        text_type,
        sizeof(test_text_asset),
        alignof(test_text_asset),
        destroy_test_text,
        &destroy_count,
    };
    assert(pulse_asset_system_register_type(assetSystem, &text_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc text_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        text_type,
        "txt",
        nullptr,
        nullptr,
        nullptr,
        step_test_text,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &text_loader_desc) == PULSE_RESULT_OK);

    PulseAssetTypeDesc fail_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        fail_type,
        sizeof(fail_asset),
        alignof(fail_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &fail_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc fail_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        fail_type,
        "txt",
        nullptr,
        nullptr,
        nullptr,
        step_fail_asset,
        sizeof(fail_loader_state),
        alignof(fail_loader_state),
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &fail_loader_desc) == PULSE_RESULT_OK);

    PulseAssetRequest fail_handle = load_asset_memory(assetSystem, fail_type, "fail.txt", hello_bytes, 11, nullptr);
    assert(pulse_asset_system_get_state(assetSystem, fail_handle) == PULSE_ASSET_STATE_WAITING_LOAD);

    PulseAssetRequest missing_handle = load_asset_file(assetSystem, text_type, "missing.txt", nullptr);
    assert(pulse_asset_request_is_valid(missing_handle));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);

    assert(pulse_asset_system_get_state(assetSystem, fail_handle) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_error(assetSystem, fail_handle) != nullptr);
    void* fail_ptr = nullptr;
    assert(!pulse_asset_system_borrow(assetSystem, pulse_asset_system_get_handle(assetSystem, fail_handle), &fail_ptr, nullptr));

    assert(pulse_asset_system_get_state(assetSystem, missing_handle) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_error(assetSystem, missing_handle) != nullptr);

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Fail/missing asset test passed!\n");
    return 0;
}
