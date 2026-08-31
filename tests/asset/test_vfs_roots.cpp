// Verifies file-based asset loading through pulse_vfs content roots:
//   - content roots are registered explicitly via pulse_vfs_add_content_root
//     (the asset plugin no longer carries a root path);
//   - additional roots (e.g. package asset dirs added by the package loader)
//     are searched in reverse registration order;
//   - a file that exists only in a later-registered root is found there;
//   - a missing file fails asynchronously with a FAILED state.
#include "test_text.h"

#include "pulse_vfs.h"

#include <string.h>

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-vfs-roots",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    // Content root for the test data directory.
    assert(pulse_vfs_mount("tests/asset/data", "/", false));

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);

    // A nested directory added as an extra content root, like the package
    // asset dirs registered by pulse_package_loader.
    assert(pulse_vfs_mount("tests/asset/data/deep", "/", false));
    assert(pulse_vfs_exists("deep.dat"));
    assert(pulse_vfs_exists("hello.dat"));
    assert(!pulse_vfs_exists("no_such_file.dat"));

    // stat()-style checks confirm the file resolves and reports as a file.
    PulseVfsStat info = {};
    assert(pulse_vfs_stat("deep.dat", &info));
    assert(info.file_type == PULSE_VFS_FILE_TYPE_REGULAR);
    assert(!pulse_vfs_stat("no_such_file.dat", &info));

    PulseAssetTypeDesc type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        text_type,
        sizeof(test_text_asset),
        alignof(test_text_asset),
        destroy_test_text,
        &destroy_count,
    };
    assert(pulse_asset_system_register_type(assetSystem, &type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        text_type,
        "dat",
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
    assert(pulse_asset_system_register_loader(assetSystem, &loader_desc) == PULSE_RESULT_OK);

    // Root file: found in the plugin's content root.
    PulseAssetRequest root_request = load_asset_file(assetSystem, text_type, "hello.dat", nullptr);
    assert(pulse_asset_request_is_valid(root_request));

    // Nested file: found only in the extra content root.
    PulseAssetRequest nested_request = load_asset_file(assetSystem, text_type, "deep.dat", nullptr);
    assert(pulse_asset_request_is_valid(nested_request));

    // Missing file: valid request that fails once processed.
    PulseAssetRequest missing_request = load_asset_file(assetSystem, text_type, "missing.dat", nullptr);
    assert(pulse_asset_request_is_valid(missing_request));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);

    assert(pulse_asset_system_get_state(assetSystem, root_request) == PULSE_ASSET_STATE_LOADED);
    PulseAssetHandle root_handle = pulse_asset_system_get_handle(assetSystem, root_request);
    assert(pulse_asset_handle_is_valid(root_handle));
    assert(pulse_asset_system_retain(assetSystem, root_handle, nullptr));
    void* root_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, root_handle, &root_ptr, nullptr));
    test_text_asset* root_asset = (test_text_asset*)root_ptr;
    assert(root_asset->size == 11);
    assert(strcmp(root_asset->text, "hello world") == 0);
    pulse_asset_system_release(assetSystem, root_handle, nullptr);

    assert(pulse_asset_system_get_state(assetSystem, nested_request) == PULSE_ASSET_STATE_LOADED);
    PulseAssetHandle nested_handle = pulse_asset_system_get_handle(assetSystem, nested_request);
    assert(pulse_asset_handle_is_valid(nested_handle));
    assert(pulse_asset_system_retain(assetSystem, nested_handle, nullptr));
    void* nested_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, nested_handle, &nested_ptr, nullptr));
    test_text_asset* nested_asset = (test_text_asset*)nested_ptr;
    assert(nested_asset->size == 12);
    assert(strcmp(nested_asset->text, "deep content") == 0);
    pulse_asset_system_release(assetSystem, nested_handle, nullptr);

    assert(pulse_asset_system_get_state(assetSystem, missing_request) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_error(assetSystem, missing_request) != nullptr);

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("VFS content root asset test passed!\n");
    return 0;
}
