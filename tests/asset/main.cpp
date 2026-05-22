#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_asset.h"

const uint64_t text_type = 1;
const uint64_t slow_type = 2;
const uint64_t fail_type = 3;

struct test_text_asset {
    uint64_t size;
    char text[64];
};

struct slow_asset {
    int value;
};

struct slow_loader_state {
    int step;
};

struct fail_asset {
    int unused;
};

struct fail_loader_state {
    int marker;
};

static int destroy_count = 0;
static int finally_count = 0;

static void destroy_test_text(void* ptr, void* user_data) {
    (void)ptr;
    int* counter = (int*)user_data;
    *counter += 1;
}

static pulse_result_t start_test_text(
    const pulse_asset_load_task* ctx,
    void** out_state,
    void* user_data
) {
    (void)ctx;
    (void)user_data;
    *out_state = nullptr;
    return PULSE_OK;
}

static pulse_asset_loader_status_t step_test_text(
    const pulse_asset_load_task* ctx,
    void* state,
    void* out_asset,
    const char** out_error,
    void* user_data
) {
    (void)state;
    (void)out_error;
    (void)user_data;
    test_text_asset* asset = (test_text_asset*)out_asset;
    asset->size = ctx->byte_size;
    uint64_t copy_size = ctx->byte_size < 63 ? ctx->byte_size : 63;
    for (uint64_t i = 0; i < copy_size; ++i) {
        asset->text[i] = (char)ctx->bytes[i];
    }
    asset->text[copy_size] = '\0';
    return PULSE_ASSET_LOADER_DONE;
}

static pulse_result_t start_slow_asset(
    const pulse_asset_load_task* ctx,
    void** out_state,
    void* user_data
) {
    (void)ctx;
    (void)user_data;
    slow_loader_state* s = new slow_loader_state();
    s->step = 0;
    *out_state = s;
    return PULSE_OK;
}

static pulse_asset_loader_status_t step_slow_asset(
    const pulse_asset_load_task* ctx,
    void* raw_state,
    void* out_asset,
    const char** out_error,
    void* user_data
) {
    (void)ctx;
    (void)out_error;
    (void)user_data;
    slow_loader_state* s = (slow_loader_state*)raw_state;
    s->step += 1;
    if (s->step < 2) {
        return PULSE_ASSET_LOADER_PENDING;
    }
    slow_asset* asset = (slow_asset*)out_asset;
    asset->value = 42;
    return PULSE_ASSET_LOADER_DONE;
}

static void destroy_slow_asset(void* raw_state, void* user_data) {
    (void)user_data;
    delete (slow_loader_state*)raw_state;
}

static bool progress_slow_asset(
    void* raw_state,
    pulse_asset_progress* out_progress,
    void* user_data
) {
    (void)user_data;
    slow_loader_state* s = (slow_loader_state*)raw_state;
    out_progress->stage = "slow-step";
    out_progress->progress = s->step == 0 ? 0.0f : 0.5f;
    out_progress->detail = "test pending loader";
    return true;
}

static pulse_result_t start_fail_asset(
    const pulse_asset_load_task* ctx,
    void** out_state,
    void* user_data
) {
    (void)ctx;
    (void)user_data;
    fail_loader_state* s = new fail_loader_state();
    s->marker = 7;
    *out_state = s;
    return PULSE_OK;
}

static pulse_asset_loader_status_t step_fail_asset(
    const pulse_asset_load_task* ctx,
    void* state,
    void* out_asset,
    const char** out_error,
    void* user_data
) {
    (void)ctx;
    (void)state;
    (void)out_asset;
    (void)user_data;
    *out_error = "intentional failure";
    return PULSE_ASSET_LOADER_FAILED;
}

static void destroy_fail_asset(void* state, void* user_data) {
    int* counter = (int*)user_data;
    *counter += 1;
    delete (fail_loader_state*)state;
}

int main(void) {
    pulse_asset_plugin_desc default_desc = pulse_asset_plugin_desc_default();
    assert(default_desc.struct_size == sizeof(pulse_asset_plugin_desc));
    assert(default_desc.version == PULSE_ASSET_PLUGIN_DESC_VERSION);
    assert(default_desc.root_path != nullptr);
    assert(default_desc.max_requests_per_update == 8);

    pulse_app_t app = pulse_app_create();
    assert(app != nullptr);

    pulse_asset_plugin_desc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_asset_add_plugin(app, &desc) == PULSE_OK);
    assert(pulse_app_has_plugin(app, "PulseAssetPlugin"));
    assert(pulse_asset_add_plugin(app, &desc) == PULSE_ERROR_DUPLICATE_PLUGIN);

    // Type registration — plugin auto-assigns type_id
    pulse_asset_type_desc type_desc = {
        sizeof(pulse_asset_type_desc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        1,
        sizeof(test_text_asset),
        alignof(test_text_asset),
        destroy_test_text,
        &destroy_count,
    };
    assert(pulse_asset_register_type(app, &type_desc) == PULSE_OK);

    // Bad registration: test validation
    pulse_asset_type_desc bad_desc = {0, 0, 0, 0, 0, nullptr, nullptr};
    assert(pulse_asset_register_type(app, &bad_desc) == PULSE_ERROR_INVALID_ARGUMENT);

    // Loader registration uses returned type_id
    pulse_asset_loader_desc loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        text_type,
        "txt",
        start_test_text,
        step_test_text,
        nullptr,
        nullptr,
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &loader_desc) == PULSE_OK);
    assert(pulse_asset_register_loader(app, &loader_desc) == PULSE_ERROR_INVALID_STATE);

    // Invalid handle queries
    pulse_asset_handle invalid = {0, PULSE_ASSET_INVALID_INDEX, 0};
    assert(pulse_asset_get_state(app, invalid) == PULSE_ASSET_STATE_EMPTY);
    assert(!pulse_asset_is_available(app, invalid));
    assert(pulse_asset_get_error(app, invalid) == nullptr);

    pulse_asset_ref invalid_ref{};
    assert(!pulse_asset_acquire(app, invalid, &invalid_ref));
    assert(invalid_ref.ptr == nullptr);

    // Load
    pulse_asset_handle text_handle = pulse_asset_load(app, text_type, "hello.txt");
    assert(text_handle.type_id == text_type);
    assert(text_handle.index != PULSE_ASSET_INVALID_INDEX);
    assert(pulse_asset_get_state(app, text_handle) == PULSE_ASSET_STATE_WAITING_LOAD);

    pulse_asset_handle same_text_handle = pulse_asset_load(app, text_type, "hello.txt");
    assert(same_text_handle.type_id == text_handle.type_id);
    assert(same_text_handle.index == text_handle.index);

    // Acquire/release after load completes
    assert(!pulse_asset_acquire(app, text_handle, &invalid_ref));
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, text_handle) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_is_available(app, text_handle));

    pulse_asset_ref text_ref{};
    assert(pulse_asset_acquire(app, text_handle, &text_ref));
    assert(text_ref.ptr != nullptr);
    test_text_asset* text_asset = (test_text_asset*)text_ref.ptr;
    assert(text_asset->size == 11);
    assert(text_asset->text[0] == 'h');

    text_asset->text[0] = 'H';
    pulse_asset_mark_modified(app, text_handle);

    pulse_asset_ref second_ref{};
    assert(pulse_asset_acquire(app, text_handle, &second_ref));
    assert(second_ref.ptr == text_ref.ptr);
    pulse_asset_release(app, &second_ref);
    assert(second_ref.ptr == nullptr);
    assert(second_ref.handle.index == PULSE_ASSET_INVALID_INDEX);

    pulse_asset_release(app, &text_ref);
    assert(text_ref.ptr == nullptr);
    assert(text_ref.handle.index == PULSE_ASSET_INVALID_INDEX);

    // Multi-step pending loader
    pulse_asset_type_desc slow_type_desc = {
        sizeof(pulse_asset_type_desc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        slow_type,
        sizeof(slow_asset),
        alignof(slow_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_register_type(app, &slow_type_desc) == PULSE_OK);
    assert(slow_type != 0 && slow_type != text_type);

    pulse_asset_loader_desc slow_loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        slow_type,
        "txt",
        start_slow_asset,
        step_slow_asset,
        destroy_slow_asset,
        progress_slow_asset,
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &slow_loader_desc) == PULSE_OK);

    pulse_asset_handle slow_handle = pulse_asset_load(app, slow_type, "hello.txt");
    assert(pulse_asset_get_state(app, slow_handle) == PULSE_ASSET_STATE_WAITING_LOAD);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, slow_handle) == PULSE_ASSET_STATE_PROCESSING);

    pulse_asset_progress progress{};
    assert(pulse_asset_get_progress(app, slow_handle, &progress));
    assert(progress.stage != nullptr);
    assert(progress.progress >= 0.0f);

    pulse_asset_ref slow_ref{};
    assert(!pulse_asset_acquire(app, slow_handle, &slow_ref));
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, slow_handle) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_acquire(app, slow_handle, &slow_ref));
    assert(((slow_asset*)slow_ref.ptr)->value == 42);
    pulse_asset_release(app, &slow_ref);

    // Failure paths
    pulse_asset_type_desc fail_type_desc = {
        sizeof(pulse_asset_type_desc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        fail_type,
        sizeof(fail_asset),
        alignof(fail_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_register_type(app, &fail_type_desc) == PULSE_OK);

    pulse_asset_loader_desc fail_loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        fail_type,
        "txt",
        start_fail_asset,
        step_fail_asset,
        destroy_fail_asset,
        nullptr,
        &finally_count,
    };
    assert(pulse_asset_register_loader(app, &fail_loader_desc) == PULSE_OK);

    pulse_asset_handle fail_handle = pulse_asset_load(app, fail_type, "hello.txt");
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, fail_handle) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_get_error(app, fail_handle) != nullptr);
    assert(finally_count == 1);
    pulse_asset_ref fail_ref{};
    assert(!pulse_asset_acquire(app, fail_handle, &fail_ref));

    // Missing file
    pulse_asset_handle missing_handle = pulse_asset_load(app, text_type, "missing.txt");
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, missing_handle) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_get_error(app, missing_handle) != nullptr);

    // Bad generation
    pulse_asset_handle bad_generation = text_handle;
    bad_generation.generation += 1;
    assert(pulse_asset_get_state(app, bad_generation) == PULSE_ASSET_STATE_EMPTY);
    assert(!pulse_asset_acquire(app, bad_generation, &fail_ref));

    pulse_app_destroy(app);
    assert(destroy_count == 1);

    printf("Asset plugin lifecycle tests passed!\n");
    return 0;
}