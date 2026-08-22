#include "test_slow.h"

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-slow",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    const char hello_bytes[] = "hello world";

    PulseAssetTypeDesc slow_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        slow_type,
        sizeof(slow_asset),
        alignof(slow_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &slow_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc slow_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        slow_type,
        "txt",
        nullptr,
        nullptr,
        nullptr,
        step_slow_asset,
        sizeof(slow_loader_state),
        alignof(slow_loader_state),
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &slow_loader_desc) == PULSE_RESULT_OK);

    PulseAssetRequest slow_request = load_asset_memory(assetSystem, slow_type, "slow.txt", hello_bytes, 11, nullptr);
    assert(pulse_asset_system_get_state(assetSystem, slow_request) == PULSE_ASSET_STATE_WAITING_LOAD);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, slow_request) == PULSE_ASSET_STATE_PROCESSING);

    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, slow_request) == PULSE_ASSET_STATE_LOADED);

    PulseAssetHandle slow_handle = pulse_asset_system_get_handle(assetSystem, slow_request);
    assert(pulse_asset_handle_is_valid(slow_handle));
    assert(pulse_asset_system_retain(assetSystem, slow_handle, nullptr));
    void* slow_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, slow_handle, &slow_ptr, nullptr));
    assert(slow_ptr != nullptr);
    assert(((slow_asset*)slow_ptr)->value == 42);
    assert(pulse_asset_system_release(assetSystem, slow_handle, nullptr));

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Slow pending asset test passed!\n");
    return 0;
}
