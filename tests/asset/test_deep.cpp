#include "test_common.h"

const uint64_t deep_settings_type = 17;
const uint64_t deep_fail_copy_type = 18;

struct deep_settings_asset {
    int value;
    int name_hash;
};

struct deep_loader_settings {
    int value;
    uint32_t count;
    const int* values;
    const char* name;
};

static int deep_settings_size_count = 0;
static int deep_settings_copy_count = 0;

// Deep-copy settings: the size callback reports the total size (struct + nested int
// array + name string) and the copy callback lays the nested data right after the
// struct and fixes the pointers into the block allocated by the asset system.
static uint64_t deep_settings_size_fn(const void* settings, void* user_data) {
    (void)user_data;
    deep_settings_size_count += 1;
    const deep_loader_settings* s = (const deep_loader_settings*)settings;
    uint64_t total = sizeof(deep_loader_settings);
    if (s->values && s->count > 0) {
        total += (uint64_t)s->count * sizeof(int);
    }
    if (s->name) {
        total += strlen(s->name) + 1;
    }
    return total;
}

static bool deep_settings_copy_fn(void* dst, const void* src, uint64_t byte_size, void* user_data) {
    (void)user_data;
    deep_settings_copy_count += 1;
    deep_loader_settings* d = (deep_loader_settings*)dst;
    const deep_loader_settings* s = (const deep_loader_settings*)src;
    uint8_t* cursor = (uint8_t*)dst + sizeof(deep_loader_settings);
    const uint8_t* end = (const uint8_t*)dst + byte_size;

    if (s->values && s->count > 0) {
        uint64_t n = (uint64_t)s->count * sizeof(int);
        if (cursor + n > end) {
            return false;
        }
        memcpy(cursor, s->values, n);
        d->values = (const int*)cursor;
        cursor += n;
    } else {
        d->values = nullptr;
    }
    if (s->name) {
        size_t len = strlen(s->name) + 1;
        if (cursor + len > end) {
            return false;
        }
        memcpy(cursor, s->name, len);
        d->name = (const char*)cursor;
        cursor += len;
    } else {
        d->name = nullptr;
    }
    return true;
}

static bool deep_settings_copy_fail_fn(void* dst, const void* src, uint64_t byte_size, void* user_data) {
    (void)dst;
    (void)src;
    (void)byte_size;
    (void)user_data;
    return false;
}

static EPulseAssetLoaderStatus step_deep_settings_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    const deep_loader_settings* settings = (const deep_loader_settings*)ctx->settings;
    deep_settings_asset* asset = (deep_settings_asset*)ctx->out_asset;
    asset->value = settings->value;
    for (uint32_t i = 0; i < settings->count; ++i) {
        asset->value += settings->values[i];
    }
    asset->name_hash = 0;
    if (settings->name) {
        for (const char* p = settings->name; *p; ++p) {
            asset->name_hash = asset->name_hash * 31 + *p;
        }
    }
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

int main(void) {
    PulseAppDesc app_desc = {
        .name = "test-asset-deep-settings",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_APP_ADD_PLUGIN_RESULT_OK);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);
    assert(assetSystem != nullptr);

    PulseAssetTypeDesc deep_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        deep_settings_type,
        sizeof(deep_settings_asset),
        alignof(deep_settings_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &deep_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc deep_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        deep_settings_type,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        step_deep_settings_asset,
        0,
        0,
        sizeof(deep_loader_settings),
        alignof(deep_loader_settings),
        deep_settings_size_fn,
        deep_settings_copy_fn,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &deep_loader_desc) == PULSE_RESULT_OK);

    // Invalid callback combinations must be rejected
    PulseAssetLoaderDesc bad_pair_desc = deep_loader_desc;
    bad_pair_desc.settings_copy_fn = nullptr;
    assert(pulse_asset_system_register_loader(assetSystem, &bad_pair_desc) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
    bad_pair_desc = deep_loader_desc;
    bad_pair_desc.settings_size_fn = nullptr;
    assert(pulse_asset_system_register_loader(assetSystem, &bad_pair_desc) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
    bad_pair_desc = deep_loader_desc;
    bad_pair_desc.settings_size = 0;
    assert(pulse_asset_system_register_loader(assetSystem, &bad_pair_desc) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);

    PulseAssetTypeDesc deep_fail_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        deep_fail_copy_type,
        sizeof(deep_settings_asset),
        alignof(deep_settings_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &deep_fail_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc deep_fail_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        deep_fail_copy_type,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        step_deep_settings_asset,
        0,
        0,
        sizeof(deep_loader_settings),
        alignof(deep_loader_settings),
        deep_settings_size_fn,
        deep_settings_copy_fail_fn,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &deep_fail_loader_desc) == PULSE_RESULT_OK);

    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);

    int deep_values[] = {1, 2, 3};
    char deep_name[] = "deep-settings-name";
    deep_loader_settings deep_stack_settings = {10, 3, deep_values, deep_name};
    PulseAssetBuildDesc deep_desc{};
    deep_desc.struct_size = sizeof(PulseAssetBuildDesc);
    deep_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    deep_desc.type_id = deep_settings_type;
    deep_desc.name = "deep-builder";
    deep_desc.settings = &deep_stack_settings;
    PulseAssetRequest deep_request = pulse_asset_system_build(assetSystem, &deep_desc);
    assert(pulse_asset_request_is_valid(deep_request));
    assert(pulse_asset_system_get_state(assetSystem, deep_request) == PULSE_ASSET_STATE_LOADED);
    assert(deep_settings_size_count == 1);
    assert(deep_settings_copy_count == 1);

    // The loader must see the copied values, not the mutated caller data
    deep_values[0] = 100;
    deep_values[1] = 200;
    deep_values[2] = 300;
    deep_name[0] = 'X';
    deep_stack_settings.value = 999;
    PulseAssetHandle deep_handle = pulse_asset_system_get_handle(assetSystem, deep_request);
    assert(pulse_asset_handle_is_valid(deep_handle));
    assert(pulse_asset_system_retain(assetSystem, deep_handle, nullptr));
    void* deep_ptr = nullptr;
    assert(pulse_asset_system_borrow(assetSystem, deep_handle, &deep_ptr, nullptr));
    deep_settings_asset* deep_asset = (deep_settings_asset*)deep_ptr;
    assert(deep_asset->value == 10 + 1 + 2 + 3);
    int expected_hash = 0;
    for (const char* p = "deep-settings-name"; *p; ++p) {
        expected_hash = expected_hash * 31 + *p;
    }
    assert(deep_asset->name_hash == expected_hash);
    assert(pulse_asset_system_release(assetSystem, deep_handle, nullptr));

    // Copy callback failure must fail the request (the block is released with the job)
    PulseAssetBuildDesc deep_fail_desc = deep_desc;
    deep_fail_desc.type_id = deep_fail_copy_type;
    deep_fail_desc.name = "deep-fail-builder";
    PulseAssetRequest deep_fail_request = pulse_asset_system_build(assetSystem, &deep_fail_desc);
    assert(!pulse_asset_request_is_valid(deep_fail_request));

    pulse_app_teardown(app);
    pulse_destroy_app(app);

    printf("Deep settings asset test passed!\n");
    return 0;
}
