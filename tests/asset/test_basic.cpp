#include "test_text.h"

int main(void) {
    PulseAssetPluginDesc default_desc = pulse_asset_plugin_desc_default();
    assert(default_desc.struct_size == sizeof(PulseAssetPluginDesc));
    assert(default_desc.version == PULSE_ASSET_PLUGIN_DESC_VERSION);
    assert(default_desc.root_path != nullptr);
    assert(default_desc.max_requests_per_update == 8);

    PulseAppDesc app_desc = {
        .name = "test-asset-basic",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);
    assert(pulse_app_has_plugin(app, "pulse_asset"));
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_ERROR_DUPLICATE_PLUGIN);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);

    const char hello_bytes[] = "hello world";

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

    PulseAssetTypeDesc bad_desc = {0, 0, 0, 0, 0, nullptr, nullptr};
    assert(pulse_asset_system_register_type(assetSystem, &bad_desc) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);

    PulseAssetLoaderDesc loader_desc = {
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
    assert(pulse_asset_system_register_loader(assetSystem, &loader_desc) == PULSE_RESULT_OK);
    assert(pulse_asset_system_register_loader(assetSystem, &loader_desc) == PULSE_RESULT_ERROR_INVALID_STATE);

    PulseAssetRequest invalid{};
    assert(!pulse_asset_request_is_valid(invalid));
    assert(pulse_asset_request_equals(invalid, pulse_asset_request_make_invalid()));
    assert(pulse_asset_system_get_state(assetSystem, invalid) == PULSE_ASSET_STATE_EMPTY);
    assert(!pulse_asset_system_is_ready(assetSystem, invalid));
    assert(pulse_asset_system_get_error(assetSystem, invalid) == nullptr);

    PulseAssetHandle invalid_handle = pulse_asset_system_get_handle(assetSystem, invalid);
    assert(!pulse_asset_handle_is_valid(invalid_handle));
    void* invalid_ptr = nullptr;
    assert(!pulse_asset_system_borrow(assetSystem, invalid_handle, &invalid_ptr, nullptr));
    assert(invalid_ptr == nullptr);

    PulseAssetRequest text_request = load_asset_memory(assetSystem, text_type, "hello.txt", hello_bytes, 11, nullptr);
    assert(text_request.type_id == text_type);
    assert(pulse_asset_request_is_valid(text_request));
    assert(text_request.index != PULSE_ASSET_INVALID_INDEX);
    assert(pulse_asset_system_get_state(assetSystem, text_request) == PULSE_ASSET_STATE_WAITING_LOAD);

    PulseAssetRequest same_text_request = load_asset_memory(assetSystem, text_type, "hello.txt", hello_bytes, 11, nullptr);
    assert(pulse_asset_request_equals(same_text_request, text_request));

    assert(load_asset_memory(assetSystem, text_type, "", hello_bytes, 11, nullptr).index == PULSE_ASSET_INVALID_INDEX);
    assert(load_asset_memory(assetSystem, text_type, "hello", hello_bytes, 11, nullptr).index == PULSE_ASSET_INVALID_INDEX);
    assert(load_asset_memory(assetSystem, text_type, "hello.txt", nullptr, 11, nullptr).index == PULSE_ASSET_INVALID_INDEX);
    assert(load_asset_memory(assetSystem, text_type, "hello.txt", hello_bytes, 0, nullptr).index == PULSE_ASSET_INVALID_INDEX);
    assert(load_asset_file(assetSystem, text_type, "missing.bin", nullptr).index == PULSE_ASSET_INVALID_INDEX);

    const char cache_bytes[] = "cache-one";
    const char cache_new_bytes[] = "cache-two";
    PulseAssetRequest cache_handle = load_asset_memory(assetSystem, text_type, "cache.txt", cache_bytes, 9, nullptr);
    PulseAssetRequest same_cache_handle = load_asset_memory(assetSystem, text_type, "cache.txt", cache_new_bytes, 9, nullptr);
    assert(same_cache_handle.index == cache_handle.index);
    assert(same_cache_handle.generation == cache_handle.generation);

    PulseAssetMemoryLoadDesc skip_cache_desc{};
    skip_cache_desc.struct_size = sizeof(PulseAssetMemoryLoadDesc);
    skip_cache_desc.version = PULSE_ASSET_MEMORY_LOAD_DESC_VERSION;
    skip_cache_desc.type_id = text_type;
    skip_cache_desc.path = "cache.txt";
    skip_cache_desc.data = cache_new_bytes;
    skip_cache_desc.size = 9;
    skip_cache_desc.flags = PULSE_ASSET_LOAD_SKIP_CACHE;
    PulseAssetRequest refreshed_cache_handle = pulse_asset_system_load_from_memory(assetSystem, &skip_cache_desc);
    assert(refreshed_cache_handle.index != cache_handle.index ||
           refreshed_cache_handle.generation != cache_handle.generation);

    PulseAssetRequest latest_cache_handle = load_asset_memory(assetSystem, text_type, "cache.txt", cache_bytes, 9, nullptr);
    assert(latest_cache_handle.index == refreshed_cache_handle.index);
    assert(latest_cache_handle.generation == refreshed_cache_handle.generation);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, text_request) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_is_ready(assetSystem, text_request));

    PulseAssetHandle text_handle = pulse_asset_system_get_handle(assetSystem, text_request);
    assert(pulse_asset_handle_is_valid(text_handle));
    assert(pulse_asset_system_retain(assetSystem, text_handle, nullptr));
    void* text_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, text_handle, &text_ptr, nullptr));
    assert(text_ptr != nullptr);
    test_text_asset* text_asset = (test_text_asset*)text_ptr;
    assert(text_asset->size == 11);
    assert(text_asset->text[0] == 'h');
    assert(strcmp(text_asset->text, hello_bytes) == 0);

    text_asset->text[0] = 'H';
    pulse_asset_system_mark_modified(assetSystem, text_handle);

    void* second_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, text_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, text_handle, &second_ptr, nullptr));
    assert(second_ptr == text_ptr);
    assert(pulse_asset_system_release(assetSystem, text_handle, nullptr));
    assert(pulse_asset_system_release(assetSystem, text_handle, nullptr));

    // Bad generation
    PulseAssetRequest bad_generation = text_request;
    bad_generation.generation += 1;
    assert(pulse_asset_system_get_state(assetSystem, bad_generation) == PULSE_ASSET_STATE_EMPTY);
    assert(!pulse_asset_system_borrow(assetSystem, pulse_asset_system_get_handle(assetSystem, bad_generation), &text_ptr, nullptr));

    // Unload: pin_count starts at 1 after load; release the last pin and check destroy.
    assert(pulse_asset_system_release(assetSystem, text_handle, nullptr));
    assert(destroy_count == 1);
    assert(pulse_asset_system_get_state(assetSystem, text_request) == PULSE_ASSET_STATE_EMPTY);

    PulseAssetRequest reused_text_request = load_asset_memory(assetSystem, text_type, "reuse_after_unload.txt", hello_bytes, 11, nullptr);
    assert(reused_text_request.index == text_request.index);
    assert(reused_text_request.generation == text_request.generation + 1);
    assert(pulse_asset_request_is_valid(reused_text_request));

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Basic asset test passed!\n");
    return 0;
}
