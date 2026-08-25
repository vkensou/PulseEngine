#include "test_slow.h"
#include "test_builder.h"

const uint64_t builder_pending_type = 12;
const uint64_t builder_wait_type = 13;
const uint64_t builder_dynamic_type = 14;

struct builder_pending_state {
    int step;
};

struct builder_dynamic_state {
    int step;
    PulseAssetRequest required_dep;
};

static int builder_pending_step_count = 0;
static int builder_pending_ctor_count = 0;
static int builder_pending_dtor_count = 0;
static int builder_wait_step_count = 0;
static int builder_dynamic_step_count = 0;
static int builder_dynamic_ctor_count = 0;
static int builder_dynamic_dtor_count = 0;

static EPulseResult ctor_builder_pending_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)ctx;
    builder_pending_ctor_count += 1;
    builder_pending_state* s = (builder_pending_state*)state;
    assert(s->step == 0);
    return PULSE_RESULT_OK;
}

static void dtor_builder_pending_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)state;
    (void)ctx;
    builder_pending_dtor_count += 1;
}

static EPulseAssetLoaderStatus step_builder_pending_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)out_error;
    builder_pending_step_count += 1;
    assert(ctx->source == PULSE_ASSET_LOAD_SOURCE_BUILDER);
    builder_pending_state* s = (builder_pending_state*)state;
    s->step += 1;
    if (s->step == 1) {
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }
    const builder_settings* settings = (const builder_settings*)ctx->settings;
    builder_asset* asset = (builder_asset*)ctx->out_asset;
    asset->value = settings->value + *settings->external;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseAssetLoaderStatus step_builder_wait_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    builder_wait_step_count += 1;
    assert(ctx->source == PULSE_ASSET_LOAD_SOURCE_BUILDER);
    builder_asset* asset = (builder_asset*)ctx->out_asset;
    asset->value = (int)ctx->dependency_count;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseResult ctor_builder_dynamic_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    builder_dynamic_ctor_count += 1;
    builder_dynamic_state* s = (builder_dynamic_state*)state;
    assert(s->step == 0);
    assert(ctx->dependency_hint == nullptr);
    s->required_dep = *(const PulseAssetRequest*)ctx->settings;
    assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, s->required_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
    return PULSE_RESULT_OK;
}

static void dtor_builder_dynamic_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)state;
    assert(ctx->dependency_hint == nullptr);
    builder_dynamic_dtor_count += 1;
}

static EPulseAssetLoaderStatus step_builder_dynamic_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)out_error;
    builder_dynamic_step_count += 1;
    assert(ctx->source == PULSE_ASSET_LOAD_SOURCE_BUILDER);
    assert(ctx->dependency_hint != nullptr);
    builder_dynamic_state* s = (builder_dynamic_state*)state;
    s->step += 1;
    if (s->step == 1) {
        assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, s->required_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED) == PULSE_RESULT_OK);
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
    }
    builder_asset* asset = (builder_asset*)ctx->out_asset;
    asset->value = (int)ctx->dependency_count;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static void register_slow_support(PulseAssetSystemId assetSystem) {
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
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-builder-async",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    asset_test_add_root("tests/asset/data");
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    register_slow_support(assetSystem);

    const char hello_bytes[] = "hello world";

    PulseAssetTypeDesc builder_pending_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        builder_pending_type,
        sizeof(builder_asset),
        alignof(builder_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &builder_pending_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc builder_pending_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        builder_pending_type,
        "",
        nullptr,
        ctor_builder_pending_asset,
        dtor_builder_pending_asset,
        step_builder_pending_asset,
        sizeof(builder_pending_state),
        alignof(builder_pending_state),
        sizeof(builder_settings),
        alignof(builder_settings),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &builder_pending_loader_desc) == PULSE_RESULT_OK);

    PulseAssetTypeDesc builder_wait_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        builder_wait_type,
        sizeof(builder_asset),
        alignof(builder_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &builder_wait_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc builder_wait_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        builder_wait_type,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        step_builder_wait_asset,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &builder_wait_loader_desc) == PULSE_RESULT_OK);

    PulseAssetTypeDesc builder_dynamic_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        builder_dynamic_type,
        sizeof(builder_asset),
        alignof(builder_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &builder_dynamic_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc builder_dynamic_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        builder_dynamic_type,
        nullptr,
        nullptr,
        ctor_builder_dynamic_asset,
        dtor_builder_dynamic_asset,
        step_builder_dynamic_asset,
        sizeof(builder_dynamic_state),
        alignof(builder_dynamic_state),
        sizeof(PulseAssetRequest),
        alignof(PulseAssetRequest),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &builder_dynamic_loader_desc) == PULSE_RESULT_OK);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    // Builder that is pending for one step, then completes.
    int builder_pending_external = 3;
    builder_settings builder_pending_settings{14, &builder_pending_external};
    PulseAssetBuildDesc builder_pending_desc{};
    builder_pending_desc.struct_size = sizeof(PulseAssetBuildDesc);
    builder_pending_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    builder_pending_desc.type_id = builder_pending_type;
    builder_pending_desc.name = "pending-builder";
    builder_pending_desc.settings = &builder_pending_settings;
    PulseAssetRequest builder_pending_request = pulse_asset_system_build(assetSystem, &builder_pending_desc);
    assert(pulse_asset_system_get_state(assetSystem, builder_pending_request) == PULSE_ASSET_STATE_PROCESSING);
    assert(builder_pending_ctor_count == 1);
    assert(builder_pending_step_count == 1);
    assert(builder_pending_dtor_count == 0);
    builder_pending_settings.value = 99;
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_pending_request) == PULSE_ASSET_STATE_LOADED);
    assert(builder_pending_step_count == 2);
    assert(builder_pending_dtor_count == 1);
    PulseAssetHandle builder_pending_handle = pulse_asset_system_get_handle(assetSystem, builder_pending_request);
    assert(pulse_asset_handle_is_valid(builder_pending_handle));
    assert(pulse_asset_system_retain(assetSystem, builder_pending_handle, nullptr));
    void* builder_pending_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, builder_pending_handle, &builder_pending_ptr, nullptr));
    assert(((builder_asset*)builder_pending_ptr)->value == 17);
    assert(pulse_asset_system_release(assetSystem, builder_pending_handle, nullptr));

    // Invalid static dependency in a builder is rejected before stepping.
    PulseAssetDependency invalid_builder_static_deps[] = {{pulse_asset_dep_ref_make_invalid(), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED}};
    PulseAssetBuildDesc invalid_builder_wait_desc{};
    invalid_builder_wait_desc.struct_size = sizeof(PulseAssetBuildDesc);
    invalid_builder_wait_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    invalid_builder_wait_desc.type_id = builder_wait_type;
    invalid_builder_wait_desc.name = "invalid-wait-builder";
    invalid_builder_wait_desc.dependencies = invalid_builder_static_deps;
    invalid_builder_wait_desc.dependency_count = 1;
    PulseAssetRequest invalid_builder_wait_handle = pulse_asset_system_build(assetSystem, &invalid_builder_wait_desc);
    assert(!pulse_asset_request_is_valid(invalid_builder_wait_handle));
    assert(builder_wait_step_count == 0);

    // Static dependency wait.
    PulseAssetRequest builder_required_dep = load_asset_memory(assetSystem, slow_type, "builder_required_dep.txt", hello_bytes, 11, nullptr);
    PulseAssetDependency builder_static_deps[] = {{pulse_asset_system_to_asset_dep_ref_from_request(assetSystem, builder_required_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED}};
    PulseAssetBuildDesc builder_wait_desc{};
    builder_wait_desc.struct_size = sizeof(PulseAssetBuildDesc);
    builder_wait_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    builder_wait_desc.type_id = builder_wait_type;
    builder_wait_desc.name = "wait-builder";
    builder_wait_desc.dependencies = builder_static_deps;
    builder_wait_desc.dependency_count = 1;
    PulseAssetRequest builder_wait_handle = pulse_asset_system_build(assetSystem, &builder_wait_desc);
    assert(pulse_asset_system_get_state(assetSystem, builder_wait_handle) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(builder_wait_step_count == 0);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_required_dep) == PULSE_ASSET_STATE_PROCESSING);
    assert(pulse_asset_system_get_state(assetSystem, builder_wait_handle) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(builder_wait_step_count == 0);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_required_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, builder_wait_handle) == PULSE_ASSET_STATE_PROCESSING);
    assert(builder_wait_step_count == 0);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_wait_handle) == PULSE_ASSET_STATE_LOADED);
    assert(builder_wait_step_count == 1);
    PulseAssetHandle builder_wait_handle_value = pulse_asset_system_get_handle(assetSystem, builder_wait_handle);
    assert(pulse_asset_handle_is_valid(builder_wait_handle_value));
    assert(pulse_asset_system_retain(assetSystem, builder_wait_handle_value, nullptr));
    void* builder_wait_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, builder_wait_handle_value, &builder_wait_ptr, nullptr));
    assert(((builder_asset*)builder_wait_ptr)->value == 1);
    assert(pulse_asset_system_release(assetSystem, builder_wait_handle_value, nullptr));

    // Dynamic dependency wait.
    PulseAssetRequest builder_dynamic_dep = load_asset_memory(assetSystem, slow_type, "builder_dynamic_dep.txt", hello_bytes, 11, nullptr);
    PulseAssetBuildDesc builder_dynamic_desc{};
    builder_dynamic_desc.struct_size = sizeof(PulseAssetBuildDesc);
    builder_dynamic_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    builder_dynamic_desc.type_id = builder_dynamic_type;
    builder_dynamic_desc.name = "dynamic-builder";
    builder_dynamic_desc.settings = &builder_dynamic_dep;
    PulseAssetRequest builder_dynamic_request = pulse_asset_system_build(assetSystem, &builder_dynamic_desc);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_request) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(builder_dynamic_ctor_count == 1);
    assert(builder_dynamic_step_count == 1);
    assert(builder_dynamic_dtor_count == 0);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_dep) == PULSE_ASSET_STATE_PROCESSING);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_request) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(builder_dynamic_step_count == 1);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_request) == PULSE_ASSET_STATE_LOADED);
    assert(builder_dynamic_step_count == 2);
    assert(builder_dynamic_dtor_count == 1);
    PulseAssetHandle builder_dynamic_handle = pulse_asset_system_get_handle(assetSystem, builder_dynamic_request);
    assert(pulse_asset_handle_is_valid(builder_dynamic_handle));
    assert(pulse_asset_system_retain(assetSystem, builder_dynamic_handle, nullptr));
    void* builder_dynamic_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, builder_dynamic_handle, &builder_dynamic_ptr, nullptr));
    assert(((builder_asset*)builder_dynamic_ptr)->value == 1);
    assert(pulse_asset_system_release(assetSystem, builder_dynamic_handle, nullptr));

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Builder async asset test passed!\n");
    return 0;
}
