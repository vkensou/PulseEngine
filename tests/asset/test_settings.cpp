#include "test_common.h"

const uint64_t settings_type = 4;

struct settings_asset {
    int value;
};

struct settings_loader_settings {
    alignas(16) int value;
};

static int settings_step_count = 0;

static EPulseAssetLoaderStatus step_settings_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    settings_step_count += 1;
    settings_asset* asset = (settings_asset*)ctx->out_asset;
    const settings_loader_settings* settings = (const settings_loader_settings*)ctx->settings;
    asset->value = settings ? settings->value : -1;
    assert((((uintptr_t)ctx->settings) % alignof(settings_loader_settings)) == 0);
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-settings",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    asset_test_add_root("tests/asset/data");
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    const char hello_bytes[] = "hello world";

    PulseAssetTypeDesc settings_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        settings_type,
        sizeof(settings_asset),
        alignof(settings_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &settings_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc settings_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        settings_type,
        "txt",
        nullptr,
        nullptr,
        nullptr,
        step_settings_asset,
        0,
        0,
        sizeof(settings_loader_settings),
        alignof(settings_loader_settings),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &settings_loader_desc) == PULSE_RESULT_OK);

    settings_loader_settings stack_settings{77};
    PulseAssetRequest settings_request = load_asset_memory(assetSystem, settings_type, "settings.txt", hello_bytes, 11, &stack_settings);
    stack_settings.value = 12;

    settings_loader_settings ignored_settings{99};
    PulseAssetRequest same_settings_request = load_asset_memory(assetSystem, settings_type, "settings.txt", hello_bytes, 11, &ignored_settings);
    assert(same_settings_request.index == settings_request.index);
    assert(same_settings_request.generation == settings_request.generation);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(settings_step_count == 1);

    PulseAssetHandle settings_handle = pulse_asset_system_get_handle(assetSystem, settings_request);
    assert(pulse_asset_handle_is_valid(settings_handle));
    assert(pulse_asset_system_retain(assetSystem, settings_handle, nullptr));
    void* settings_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, settings_handle, &settings_ptr, nullptr));
    assert(settings_ptr != nullptr);
    assert(((settings_asset*)settings_ptr)->value == 77);
    assert(pulse_asset_system_release(assetSystem, settings_handle, nullptr));

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Settings asset test passed!\n");
    return 0;
}
