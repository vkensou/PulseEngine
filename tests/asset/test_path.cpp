#include "test_text.h"

const uint64_t path_builder_type = 2;

struct path_builder_asset {
    uint64_t value;
};

static EPulseAssetLoaderStatus step_path_builder(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    ((path_builder_asset*)ctx->out_asset)->value = 42;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static PulseAssetLoaderDesc make_loader_desc(uint64_t type_id, const char* extensions, PulseProcAssetLoaderStepFn step) {
    PulseAssetLoaderDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoaderDesc);
    desc.version = PULSE_ASSET_LOADER_DESC_VERSION;
    desc.type_id = type_id;
    desc.extensions = extensions;
    desc.step = step;
    return desc;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-path",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);
    PulseVfsPluginDesc vfs_desc = pulse_vfs_plugin_desc_default();
    assert(pulse_add_vfs_plugin(app, &vfs_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetPluginDesc asset_desc = pulse_asset_plugin_desc_default();
    asset_test_add_root("tests/asset/data");
    assert(pulse_add_asset_plugin(app, &asset_desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

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

    PulseAssetLoaderDesc dat_loader_desc = make_loader_desc(text_type, "dat", step_test_text);
    assert(pulse_asset_system_register_loader(assetSystem, &dat_loader_desc) == PULSE_RESULT_OK);

    PulseAssetTypeDesc builder_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        path_builder_type,
        sizeof(path_builder_asset),
        alignof(path_builder_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &builder_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc builder_loader_desc = make_loader_desc(path_builder_type, nullptr, step_path_builder);
    assert(pulse_asset_system_register_loader(assetSystem, &builder_loader_desc) == PULSE_RESULT_OK);

    PulseAssetRequest file_request = load_asset_file(assetSystem, text_type, "deep\\deep.dat", nullptr);
    assert(pulse_asset_request_is_valid(file_request));

    const char mem_bytes[] = "in-memory";
    PulseAssetRequest memory_request = load_asset_memory(assetSystem, text_type, "memory.dat", mem_bytes, 9, nullptr);
    assert(pulse_asset_request_is_valid(memory_request));

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, file_request) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, memory_request) == PULSE_ASSET_STATE_LOADED);

    PulseAssetBuildDesc builder_desc{};
    builder_desc.struct_size = sizeof(PulseAssetBuildDesc);
    builder_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    builder_desc.type_id = path_builder_type;
    builder_desc.name = "dynamic";
    PulseAssetHandle builder_handle = pulse_asset_system_build_sync(assetSystem, &builder_desc);
    assert(pulse_asset_handle_is_valid(builder_handle));

    PulseAssetHandle file_handle = pulse_asset_system_get_handle(assetSystem, file_request);
    const char* file_path = pulse_asset_system_get_path(assetSystem, file_handle);
    assert(file_path != nullptr);
    assert(strcmp(file_path, "deep/deep.dat") == 0);

    PulseAssetHandle memory_handle = pulse_asset_system_get_handle(assetSystem, memory_request);
    assert(pulse_asset_system_get_path(assetSystem, memory_handle) == nullptr);

    assert(pulse_asset_system_get_path(assetSystem, builder_handle) == nullptr);

    assert(pulse_asset_system_get_path(assetSystem, pulse_asset_handle_make_invalid()) == nullptr);
    assert(pulse_asset_system_get_path(nullptr, file_handle) == nullptr);

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("asset path test passed\n");
    return 0;
}
