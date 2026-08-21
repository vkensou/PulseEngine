#include "test_common.h"

const uint64_t force_type = 16;

struct force_asset {
    int value;
};

struct force_loader_state {
    int step;
};

static int force_destroy_count = 0;
static int force_ctor_count = 0;
static int force_dtor_count = 0;
static int force_step_count = 0;

static void destroy_force_asset(void* ptr, void* user_data) {
    (void)ptr;
    (void)user_data;
    force_destroy_count += 1;
}

static EPulseResult ctor_force_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)ctx;
    force_ctor_count += 1;
    force_loader_state* s = (force_loader_state*)state;
    assert(s->step == 0);
    return PULSE_RESULT_OK;
}

static void dtor_force_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)ctx;
    force_dtor_count += 1;
    force_loader_state* s = (force_loader_state*)state;
    assert(s != nullptr);
}

static EPulseAssetLoaderStatus step_force_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)out_error;
    force_step_count += 1;
    force_loader_state* s = (force_loader_state*)state;
    s->step += 1;
    if (s->step < 2) {
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }

    force_asset* asset = (force_asset*)ctx->out_asset;
    asset->value = (int)ctx->byte_size;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-force",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    const char force_bytes[] = "force";

    PulseAssetTypeDesc force_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        force_type,
        sizeof(force_asset),
        alignof(force_asset),
        destroy_force_asset,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &force_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc force_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        force_type,
        "txt",
        nullptr,
        ctor_force_asset,
        dtor_force_asset,
        step_force_asset,
        sizeof(force_loader_state),
        alignof(force_loader_state),
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &force_loader_desc) == PULSE_RESULT_OK);

    assert(pulse_app_prepare(app) == PULSE_RESULT_OK);

    PulseAssetRequest force_loaded = load_asset_memory(assetSystem, force_type, "force_loaded.txt", force_bytes, sizeof(force_bytes), nullptr);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, force_loaded) == PULSE_ASSET_STATE_PROCESSING);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, force_loaded) == PULSE_ASSET_STATE_LOADED);
    assert(force_ctor_count == 1);
    assert(force_dtor_count == 1);

    PulseAssetHandle force_loaded_handle = pulse_asset_system_get_handle(assetSystem, force_loaded);
    assert(pulse_asset_handle_is_valid(force_loaded_handle));
    assert(pulse_asset_system_retain(assetSystem, force_loaded_handle, nullptr));
    void* force_loaded_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, force_loaded_handle, &force_loaded_ptr, nullptr));
    assert(force_loaded_ptr != nullptr);
    assert(((force_asset*)force_loaded_ptr)->value == sizeof(force_bytes));

    PulseAssetRequest force_pending = load_asset_memory(assetSystem, force_type, "force_pending.txt", force_bytes, sizeof(force_bytes), nullptr);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, force_pending) == PULSE_ASSET_STATE_PROCESSING);
    assert(force_ctor_count == 2);
    assert(force_dtor_count == 1);

    int force_steps_before_unload = force_step_count;
    pulse_asset_system_force_unload_assets(assetSystem, force_type);
    assert(pulse_asset_system_get_state(assetSystem, force_loaded) == PULSE_ASSET_STATE_EMPTY);
    assert(pulse_asset_system_get_state(assetSystem, force_pending) == PULSE_ASSET_STATE_EMPTY);
    assert(force_destroy_count == 1);
    assert(force_dtor_count == 2);
    void* missing_force_ptr = nullptr;
    assert(!pulse_asset_system_borrow(assetSystem, force_loaded_handle, &missing_force_ptr, nullptr));
    pulse_asset_system_release(assetSystem, force_loaded_handle, nullptr);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(force_step_count == force_steps_before_unload);

    PulseAssetRequest force_reloaded = load_asset_memory(assetSystem, force_type, "force_loaded.txt", force_bytes, sizeof(force_bytes), nullptr);
    assert(pulse_asset_request_is_valid(force_reloaded));
    assert(!pulse_asset_request_equals(force_reloaded, force_loaded));
    pulse_asset_system_force_unload_assets(assetSystem, 0);

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Force unload asset test passed!\n");
    return 0;
}
