#include "test_text.h"
#include "test_builder.h"

const uint64_t builder_named_type = 19;
const uint64_t builder_fail_once_type = 15;

static int builder_step_count = 0;

static EPulseAssetLoaderStatus step_builder_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    builder_step_count += 1;
    assert(ctx->source == PULSE_ASSET_LOAD_SOURCE_BUILDER);
    assert(ctx->bytes == nullptr);
    assert(ctx->byte_size == 0);
    assert(strcmp(ctx->path, "runtime-builder") == 0);
    const builder_settings* settings = (const builder_settings*)ctx->settings;
    builder_asset* asset = (builder_asset*)ctx->out_asset;
    asset->value = settings->value + *settings->external;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

// 命名 builder：同一类型可注册多个 builder（按 AssetLoaderDesc.name 区分），
// build 请求用 name 选择 loader；ctx->path 携带请求的 name。
static int builder_named_a_step_count = 0;
static int builder_named_b_step_count = 0;

static EPulseAssetLoaderStatus step_builder_named_a(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    builder_named_a_step_count += 1;
    assert(strcmp(ctx->loader_identifier, "named-a") == 0);
    ((builder_asset*)ctx->out_asset)->value = 1001;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseAssetLoaderStatus step_builder_named_b(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    builder_named_b_step_count += 1;
    assert(strcmp(ctx->loader_identifier, "named-b") == 0);
    ((builder_asset*)ctx->out_asset)->value = 2002;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static int builder_fail_once_step_count = 0;

static EPulseAssetLoaderStatus step_builder_fail_once_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    builder_fail_once_step_count += 1;
    assert(ctx->source == PULSE_ASSET_LOAD_SOURCE_BUILDER);
    if (builder_fail_once_step_count == 1) {
        *out_error = "builder failed before returning a handle";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }

    builder_asset* asset = (builder_asset*)ctx->out_asset;
    asset->value = 77;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static void register_text_support(PulseAssetSystemId assetSystem) {
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
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-builders",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    register_text_support(assetSystem);

    PulseAssetTypeDesc builder_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        builder_type,
        sizeof(builder_asset),
        alignof(builder_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &builder_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc builder_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        builder_type,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        step_builder_asset,
        0,
        0,
        sizeof(builder_settings),
        alignof(builder_settings),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &builder_loader_desc) == PULSE_RESULT_OK);
    assert(pulse_asset_system_register_loader(assetSystem, &builder_loader_desc) == PULSE_RESULT_ERROR_INVALID_STATE);

    // 同一类型可注册多个命名 builder，build 请求按 name 选择 loader。
    PulseAssetTypeDesc builder_named_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        builder_named_type,
        sizeof(builder_asset),
        alignof(builder_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &builder_named_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc builder_named_a_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        builder_named_type,
        nullptr,
        "named-a",
        nullptr,
        nullptr,
        step_builder_named_a,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &builder_named_a_desc) == PULSE_RESULT_OK);
    assert(pulse_asset_system_register_loader(assetSystem, &builder_named_a_desc) == PULSE_RESULT_ERROR_INVALID_STATE);

    PulseAssetLoaderDesc builder_named_b_desc = builder_named_a_desc;
    builder_named_b_desc.step = step_builder_named_b;
    builder_named_b_desc.loader_identifier = "named-b";
    assert(pulse_asset_system_register_loader(assetSystem, &builder_named_b_desc) == PULSE_RESULT_OK);

    PulseAssetTypeDesc builder_fail_once_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        builder_fail_once_type,
        sizeof(builder_asset),
        alignof(builder_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &builder_fail_once_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc builder_fail_once_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        builder_fail_once_type,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        step_builder_fail_once_asset,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &builder_fail_once_loader_desc) == PULSE_RESULT_OK);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    // Named builder a
    PulseAssetBuildDesc named_a_build{};
    named_a_build.struct_size = sizeof(PulseAssetBuildDesc);
    named_a_build.version = PULSE_ASSET_BUILD_DESC_VERSION;
    named_a_build.type_id = builder_named_type;
    named_a_build.loader_identifier = "named-a";
    PulseAssetRequest named_a_request = pulse_asset_system_build(assetSystem, &named_a_build);
    assert(pulse_asset_request_is_valid(named_a_request));
    assert(pulse_asset_system_get_state(assetSystem, named_a_request) == PULSE_ASSET_STATE_LOADED);
    assert(builder_named_a_step_count == 1 && builder_named_b_step_count == 0);
    PulseAssetHandle named_a_handle = pulse_asset_system_get_handle(assetSystem, named_a_request);
    assert(pulse_asset_handle_is_valid(named_a_handle));
    assert(pulse_asset_system_retain(assetSystem, named_a_handle, nullptr));
    void* named_a_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, named_a_handle, &named_a_ptr, nullptr));
    assert(((builder_asset*)named_a_ptr)->value == 1001);
    assert(pulse_asset_system_release(assetSystem, named_a_handle, nullptr));

    // Named builder b
    PulseAssetBuildDesc named_b_build{};
    named_b_build.struct_size = sizeof(PulseAssetBuildDesc);
    named_b_build.version = PULSE_ASSET_BUILD_DESC_VERSION;
    named_b_build.type_id = builder_named_type;
    named_b_build.loader_identifier = "named-b";
    PulseAssetRequest named_b_request = pulse_asset_system_build(assetSystem, &named_b_build);
    assert(pulse_asset_request_is_valid(named_b_request));
    assert(pulse_asset_system_get_state(assetSystem, named_b_request) == PULSE_ASSET_STATE_LOADED);
    assert(builder_named_a_step_count == 1 && builder_named_b_step_count == 1);
    PulseAssetHandle named_b_handle = pulse_asset_system_get_handle(assetSystem, named_b_request);
    assert(pulse_asset_handle_is_valid(named_b_handle));
    assert(pulse_asset_system_retain(assetSystem, named_b_handle, nullptr));
    void* named_b_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, named_b_handle, &named_b_ptr, nullptr));
    assert(((builder_asset*)named_b_ptr)->value == 2002);
    assert(pulse_asset_system_release(assetSystem, named_b_handle, nullptr));

    // 未注册的 name：无匹配且无默认 builder -> 加载失败
    PulseAssetBuildDesc unknown_name_build{};
    unknown_name_build.struct_size = sizeof(PulseAssetBuildDesc);
    unknown_name_build.version = PULSE_ASSET_BUILD_DESC_VERSION;
    unknown_name_build.type_id = builder_named_type;
    unknown_name_build.name = "no-such-builder";
    assert(pulse_asset_system_build(assetSystem, &unknown_name_build).index == PULSE_ASSET_INVALID_INDEX);

    // Fail once builder
    PulseAssetBuildDesc builder_fail_once_desc{};
    builder_fail_once_desc.struct_size = sizeof(PulseAssetBuildDesc);
    builder_fail_once_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    builder_fail_once_desc.type_id = builder_fail_once_type;
    builder_fail_once_desc.name = "fail-once-builder";
    PulseAssetRequest failed_builder_request = pulse_asset_system_build(assetSystem, &builder_fail_once_desc);
    assert(!pulse_asset_request_is_valid(failed_builder_request));
    assert(builder_fail_once_step_count == 1);

    PulseAssetRequest recovered_builder_request = pulse_asset_system_build(assetSystem, &builder_fail_once_desc);
    assert(pulse_asset_request_is_valid(recovered_builder_request));
    assert(recovered_builder_request.index == 1);
    assert(recovered_builder_request.generation == 2);
    assert(pulse_asset_system_get_state(assetSystem, recovered_builder_request) == PULSE_ASSET_STATE_LOADED);
    assert(builder_fail_once_step_count == 2);
    PulseAssetHandle recovered_builder_handle = pulse_asset_system_get_handle(assetSystem, recovered_builder_request);
    assert(pulse_asset_handle_is_valid(recovered_builder_handle));
    assert(pulse_asset_system_retain(assetSystem, recovered_builder_handle, nullptr));
    void* recovered_builder_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, recovered_builder_handle, &recovered_builder_ptr, nullptr));
    assert(((builder_asset*)recovered_builder_ptr)->value == 77);
    assert(pulse_asset_system_release(assetSystem, recovered_builder_handle, nullptr));

    // 非 builder 类型不能 build
    PulseAssetBuildDesc missing_builder_desc{};
    missing_builder_desc.struct_size = sizeof(PulseAssetBuildDesc);
    missing_builder_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    missing_builder_desc.type_id = text_type;
    assert(pulse_asset_system_build(assetSystem, &missing_builder_desc).index == PULSE_ASSET_INVALID_INDEX);

    // 普通 builder：settings 拷贝生效，重复 build 生成新请求。
    int builder_external = 5;
    builder_settings builder_stack_settings{37, &builder_external};
    PulseAssetBuildDesc builder_desc{};
    builder_desc.struct_size = sizeof(PulseAssetBuildDesc);
    builder_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    builder_desc.type_id = builder_type;
    builder_desc.name = "runtime-builder";
    builder_desc.settings = &builder_stack_settings;
    PulseAssetRequest builder_request = pulse_asset_system_build(assetSystem, &builder_desc);
    assert(builder_request.index != PULSE_ASSET_INVALID_INDEX);
    assert(pulse_asset_system_get_state(assetSystem, builder_request) == PULSE_ASSET_STATE_LOADED);
    assert(builder_step_count == 1);
    PulseAssetHandle builder_handle = pulse_asset_system_get_handle(assetSystem, builder_request);
    assert(pulse_asset_handle_is_valid(builder_handle));
    assert(pulse_asset_system_retain(assetSystem, builder_handle, nullptr));
    void* builder_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, builder_handle, &builder_ptr, nullptr));
    assert(((builder_asset*)builder_ptr)->value == 42);
    assert(pulse_asset_system_release(assetSystem, builder_handle, nullptr));

    PulseAssetRequest second_builder_request = pulse_asset_system_build(assetSystem, &builder_desc);
    assert(second_builder_request.index != PULSE_ASSET_INVALID_INDEX);
    assert(second_builder_request.index != builder_request.index ||
           second_builder_request.generation != builder_request.generation);
    assert(builder_step_count == 2);

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Builder asset test passed!\n");
    return 0;
}
