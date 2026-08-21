#include "test_text.h"
#include "test_slow.h"
#include "test_fail.h"

const uint64_t parent_type = 7;
const uint64_t dynamic_parent_type = 8;

struct parent_asset {
    int value;
};

struct dynamic_parent_asset {
    int value;
};

struct dynamic_loader_state {
    int step;
    PulseAssetRequest required_dep;
    PulseAssetRequest optional_dep;
    int add_optional;
};

static int parent_step_count = 0;
static int dynamic_step_count = 0;
static int dynamic_ctor_count = 0;
static int dynamic_dtor_count = 0;

static EPulseAssetLoaderStatus step_parent_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    parent_step_count += 1;
    parent_asset* asset = (parent_asset*)ctx->out_asset;
    asset->value = (int)ctx->dependency_count;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseResult ctor_dynamic_parent_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    dynamic_ctor_count += 1;
    dynamic_loader_state* s = (dynamic_loader_state*)state;
    assert(s->step == 0);
    assert(ctx->dependency_hint == nullptr);
    const PulseAssetRequest* requests = (const PulseAssetRequest*)ctx->settings;
    s->required_dep = requests[0];
    s->optional_dep = requests[1];
    s->add_optional = pulse_asset_request_is_valid(requests[1]);
    assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, s->required_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
    return PULSE_RESULT_OK;
}

static void dtor_dynamic_parent_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    dynamic_loader_state* s = (dynamic_loader_state*)state;
    assert(s != nullptr);
    assert(ctx->dependency_hint == nullptr);
    assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, s->required_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
    dynamic_dtor_count += 1;
}

static EPulseAssetLoaderStatus step_dynamic_parent_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)out_error;
    dynamic_step_count += 1;
    dynamic_loader_state* s = (dynamic_loader_state*)state;
    s->step += 1;
    assert(ctx->dependency_hint != nullptr);

    if (s->step == 1) {
        assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, s->required_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED) == PULSE_RESULT_OK);
        if (s->add_optional) {
            assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, s->optional_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL) == PULSE_RESULT_OK);
        }
        return PULSE_ASSET_LOADER_STATUS_WAIT_DEPENDENCIES;
    }

    dynamic_parent_asset* asset = (dynamic_parent_asset*)ctx->out_asset;
    asset->value = (int)ctx->dependency_count;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static void register_support_types(PulseAssetSystemId assetSystem) {
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
}

static void register_parent_types(PulseAssetSystemId assetSystem) {
    PulseAssetTypeDesc parent_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        parent_type,
        sizeof(parent_asset),
        alignof(parent_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &parent_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc parent_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        parent_type,
        "txt",
        nullptr,
        nullptr,
        nullptr,
        step_parent_asset,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &parent_loader_desc) == PULSE_RESULT_OK);

    PulseAssetTypeDesc dynamic_parent_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        dynamic_parent_type,
        sizeof(dynamic_parent_asset),
        alignof(dynamic_parent_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &dynamic_parent_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc dynamic_parent_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        dynamic_parent_type,
        "txt",
        nullptr,
        ctor_dynamic_parent_asset,
        dtor_dynamic_parent_asset,
        step_dynamic_parent_asset,
        sizeof(dynamic_loader_state),
        alignof(dynamic_loader_state),
        sizeof(PulseAssetRequest) * 2,
        alignof(PulseAssetRequest),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &dynamic_parent_loader_desc) == PULSE_RESULT_OK);
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-dependencies",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    register_support_types(assetSystem);
    register_parent_types(assetSystem);

    const char hello_bytes[] = "hello world";
    const char dep_bytes[] = "dependency";
    const char parent_bytes[] = "parent";

    assert(pulse_app_prepare(app) == PULSE_RESULT_OK);

    // Static required dependency succeeds.
    PulseAssetRequest static_dep = load_asset_memory(assetSystem, text_type, "static_dep.txt", dep_bytes, 10, nullptr);
    PulseAssetDependency static_deps[] = {{pulse_asset_system_to_asset_dep_ref_from_request(assetSystem, static_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED}};
    PulseAssetRequest static_parent = load_asset_memory_with_deps(
        assetSystem,
        parent_type,
        "static_parent.txt",
        parent_bytes,
        6,
        static_deps,
        1,
        nullptr);
    assert(pulse_asset_system_get_state(assetSystem, static_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, static_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, static_parent) == PULSE_ASSET_STATE_PROCESSING);
    assert(parent_step_count == 0);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, static_parent) == PULSE_ASSET_STATE_LOADED);
    assert(parent_step_count == 1);
    PulseAssetHandle static_parent_handle = pulse_asset_system_get_handle(assetSystem, static_parent);
    assert(pulse_asset_handle_is_valid(static_parent_handle));
    assert(pulse_asset_system_retain(assetSystem, static_parent_handle, nullptr));
    void* static_parent_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, static_parent_handle, &static_parent_ptr, nullptr));
    assert(((parent_asset*)static_parent_ptr)->value == 1);
    assert(pulse_asset_system_release(assetSystem, static_parent_handle, nullptr));

    // Static failed dependency fails the parent.
    PulseAssetRequest static_failed_dep = load_asset_memory(assetSystem, fail_type, "static_failed_dep.txt", hello_bytes, 11, nullptr);
    PulseAssetDependency failed_static_deps[] = {{pulse_asset_system_to_asset_dep_ref_from_request(assetSystem, static_failed_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED}};
    PulseAssetRequest failed_static_parent = load_asset_memory_with_deps(
        assetSystem,
        parent_type,
        "failed_static_parent.txt",
        parent_bytes,
        6,
        failed_static_deps,
        1,
        nullptr);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, static_failed_dep) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_state(assetSystem, failed_static_parent) == PULSE_ASSET_STATE_FAILED);
    void* failed_static_parent_ptr = nullptr;
    assert(!pulse_asset_system_borrow(assetSystem, pulse_asset_system_get_handle(assetSystem, failed_static_parent), &failed_static_parent_ptr, nullptr));

    // Dynamic required dependency succeeds.
    dynamic_step_count = 0;
    dynamic_ctor_count = 0;
    dynamic_dtor_count = 0;
    PulseAssetRequest dynamic_required_dep = load_asset_memory(assetSystem, slow_type, "dynamic_required_dep.txt", hello_bytes, 11, nullptr);
    PulseAssetRequest no_optional{};
    PulseAssetRequest dynamic_settings[] = {dynamic_required_dep, no_optional};
    PulseAssetRequest dynamic_parent = load_asset_memory(
        assetSystem,
        dynamic_parent_type,
        "dynamic_parent.txt",
        parent_bytes,
        6,
        dynamic_settings);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_required_dep) == PULSE_ASSET_STATE_PROCESSING);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(dynamic_step_count == 1);
    assert(dynamic_ctor_count == 1);
    assert(dynamic_dtor_count == 0);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_required_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_parent) == PULSE_ASSET_STATE_LOADED);
    assert(dynamic_step_count == 2);
    assert(dynamic_dtor_count == 1);
    PulseAssetHandle dynamic_parent_handle = pulse_asset_system_get_handle(assetSystem, dynamic_parent);
    assert(pulse_asset_handle_is_valid(dynamic_parent_handle));
    assert(pulse_asset_system_retain(assetSystem, dynamic_parent_handle, nullptr));
    void* dynamic_parent_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, dynamic_parent_handle, &dynamic_parent_ptr, nullptr));
    assert(((dynamic_parent_asset*)dynamic_parent_ptr)->value == 1);
    assert(pulse_asset_system_release(assetSystem, dynamic_parent_handle, nullptr));

    // Dynamic required dependency failure fails the parent.
    PulseAssetRequest dynamic_failed_dep = load_asset_memory(assetSystem, fail_type, "dynamic_failed_dep.txt", hello_bytes, 11, nullptr);
    PulseAssetRequest dynamic_failed_settings[] = {dynamic_failed_dep, no_optional};
    PulseAssetRequest dynamic_failed_parent = load_asset_memory(
        assetSystem,
        dynamic_parent_type,
        "dynamic_failed_parent.txt",
        parent_bytes,
        6,
        dynamic_failed_settings);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_failed_dep) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_failed_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_failed_parent) == PULSE_ASSET_STATE_FAILED);

    // Optional dependency failure does not fail the parent and is counted.
    PulseAssetRequest dynamic_ready_dep = load_asset_memory(assetSystem, text_type, "dynamic_ready_dep.txt", hello_bytes, 11, nullptr);
    PulseAssetRequest dynamic_optional_failed_dep = load_asset_memory(assetSystem, fail_type, "dynamic_optional_failed_dep.txt", hello_bytes, 11, nullptr);
    PulseAssetRequest dynamic_optional_settings[] = {dynamic_ready_dep, dynamic_optional_failed_dep};
    PulseAssetRequest dynamic_optional_parent = load_asset_memory(
        assetSystem,
        dynamic_parent_type,
        "dynamic_optional_parent.txt",
        parent_bytes,
        6,
        dynamic_optional_settings);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_ready_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_optional_failed_dep) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_optional_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, dynamic_optional_parent) == PULSE_ASSET_STATE_LOADED);
    PulseAssetHandle dynamic_optional_parent_handle = pulse_asset_system_get_handle(assetSystem, dynamic_optional_parent);
    assert(pulse_asset_handle_is_valid(dynamic_optional_parent_handle));
    assert(pulse_asset_system_retain(assetSystem, dynamic_optional_parent_handle, nullptr));
    void* dynamic_optional_parent_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, dynamic_optional_parent_handle, &dynamic_optional_parent_ptr, nullptr));
    assert(((dynamic_parent_asset*)dynamic_optional_parent_ptr)->value == 2);
    assert(pulse_asset_system_release(assetSystem, dynamic_optional_parent_handle, nullptr));

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Dependency asset test passed!\n");
    return 0;
}
