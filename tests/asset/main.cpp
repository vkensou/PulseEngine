#undef NDEBUG
#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"
#include "pulse_asset.h"

const uint64_t text_type = 1;
const uint64_t slow_type = 2;
const uint64_t fail_type = 3;
const uint64_t settings_type = 4;
const uint64_t cleanup_type = 5;
const uint64_t aligned_type = 6;
const uint64_t parent_type = 7;
const uint64_t dynamic_parent_type = 8;

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

struct settings_asset {
    int value;
};

struct settings_loader_settings {
    alignas(16) int value;
};

struct cleanup_asset {
    int value;
};

struct cleanup_loader_state {
    int step;
    int fail;
};

struct alignas(64) aligned_asset {
    int value;
};

struct alignas(64) aligned_loader_state {
    int constructed;
};

struct alignas(64) aligned_loader_settings {
    int value;
};

struct parent_asset {
    int value;
};

struct dynamic_parent_asset {
    int value;
};

struct dynamic_loader_state {
    int step;
    pulse_asset_handle required_dep;
    pulse_asset_handle optional_dep;
    int add_optional;
};

static int destroy_count = 0;
static int settings_step_count = 0;
static int cleanup_ctor_count = 0;
static int cleanup_dtor_count = 0;
static int aligned_ctor_count = 0;
static int parent_step_count = 0;
static int dynamic_step_count = 0;
static int dynamic_ctor_count = 0;
static int dynamic_dtor_count = 0;

static void destroy_test_text(void* ptr, void* user_data) {
    (void)ptr;
    int* counter = (int*)user_data;
    *counter += 1;
}

static pulse_asset_loader_status_t step_test_text(
    void* state,
    const pulse_asset_load_task* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    test_text_asset* asset = (test_text_asset*)ctx->out_asset;
    asset->size = ctx->byte_size;
    uint64_t copy_size = ctx->byte_size < 63 ? ctx->byte_size : 63;
    for (uint64_t i = 0; i < copy_size; ++i) {
        asset->text[i] = (char)ctx->bytes[i];
    }
    asset->text[copy_size] = '\0';
    return PULSE_ASSET_LOADER_DONE;
}

static pulse_asset_loader_status_t step_slow_asset(
    void* raw_state,
    const pulse_asset_load_task* ctx,
    const char** out_error
) {
    (void)ctx;
    (void)out_error;
    slow_loader_state* s = (slow_loader_state*)raw_state;
    s->step += 1;
    if (s->step < 2) {
        return PULSE_ASSET_LOADER_PENDING;
    }
    slow_asset* asset = (slow_asset*)ctx->out_asset;
    asset->value = 42;
    return PULSE_ASSET_LOADER_DONE;
}

static pulse_asset_loader_status_t step_fail_asset(
    void* state,
    const pulse_asset_load_task* ctx,
    const char** out_error
) {
    fail_loader_state* s = (fail_loader_state*)state;
    assert(s->marker == 0);
    (void)ctx;
    *out_error = "intentional failure";
    return PULSE_ASSET_LOADER_FAILED;
}

static pulse_asset_loader_status_t step_settings_asset(
    void* state,
    const pulse_asset_load_task* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    settings_step_count += 1;
    settings_asset* asset = (settings_asset*)ctx->out_asset;
    const settings_loader_settings* settings = (const settings_loader_settings*)ctx->settings;
    asset->value = settings ? settings->value : -1;
    assert((((uintptr_t)ctx->settings) % alignof(settings_loader_settings)) == 0);
    return PULSE_ASSET_LOADER_DONE;
}

static pulse_result_t ctor_cleanup_asset(
    void* state,
    const pulse_asset_load_task* ctx
) {
    cleanup_ctor_count += 1;
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    assert(s->step == 0);
    assert(s->fail == 0);
    s->fail = ctx->settings ? *((const int*)ctx->settings) : 0;
    return PULSE_OK;
}

static void dtor_cleanup_asset(
    void* state,
    const pulse_asset_load_task* ctx
) {
    (void)ctx;
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    assert(s != nullptr);
    cleanup_dtor_count += 1;
}

static pulse_asset_loader_status_t step_cleanup_asset(
    void* state,
    const pulse_asset_load_task* ctx,
    const char** out_error
) {
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    s->step += 1;
    if (s->fail) {
        *out_error = "cleanup loader intentional failure";
        return PULSE_ASSET_LOADER_FAILED;
    }
    if (s->step < 2) {
        return PULSE_ASSET_LOADER_PENDING;
    }
    cleanup_asset* asset = (cleanup_asset*)ctx->out_asset;
    asset->value = 9;
    return PULSE_ASSET_LOADER_DONE;
}

static pulse_result_t ctor_aligned_asset(
    void* state,
    const pulse_asset_load_task* ctx
) {
    (void)ctx;
    assert((((uintptr_t)state) % alignof(aligned_loader_state)) == 0);
    aligned_loader_state* s = (aligned_loader_state*)state;
    assert(s->constructed == 0);
    s->constructed = 1;
    aligned_ctor_count += 1;
    return PULSE_OK;
}

static pulse_asset_loader_status_t step_aligned_asset(
    void* state,
    const pulse_asset_load_task* ctx,
    const char** out_error
) {
    (void)out_error;
    assert((((uintptr_t)state) % alignof(aligned_loader_state)) == 0);
    assert((((uintptr_t)ctx->settings) % alignof(aligned_loader_settings)) == 0);
    assert((((uintptr_t)ctx->out_asset) % alignof(aligned_asset)) == 0);
    aligned_asset* asset = (aligned_asset*)ctx->out_asset;
    const aligned_loader_settings* settings = (const aligned_loader_settings*)ctx->settings;
    asset->value = settings->value;
    return PULSE_ASSET_LOADER_DONE;
}

static pulse_asset_loader_status_t step_parent_asset(
    void* state,
    const pulse_asset_load_task* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    parent_step_count += 1;
    parent_asset* asset = (parent_asset*)ctx->out_asset;
    asset->value = (int)ctx->dependency_count;
    return PULSE_ASSET_LOADER_DONE;
}

static pulse_result_t ctor_dynamic_parent_asset(
    void* state,
    const pulse_asset_load_task* ctx
) {
    dynamic_ctor_count += 1;
    dynamic_loader_state* s = (dynamic_loader_state*)state;
    assert(s->step == 0);
    const pulse_asset_handle* handles = (const pulse_asset_handle*)ctx->settings;
    s->required_dep = handles[0];
    s->optional_dep = handles[1];
    s->add_optional = handles[1].index != PULSE_ASSET_INVALID_INDEX;
    return PULSE_OK;
}

static void dtor_dynamic_parent_asset(
    void* state,
    const pulse_asset_load_task* ctx
) {
    (void)state;
    (void)ctx;
    dynamic_dtor_count += 1;
}

static pulse_asset_loader_status_t step_dynamic_parent_asset(
    void* state,
    const pulse_asset_load_task* ctx,
    const char** out_error
) {
    (void)out_error;
    dynamic_step_count += 1;
    dynamic_loader_state* s = (dynamic_loader_state*)state;
    s->step += 1;
    assert(ctx->job != nullptr);

    if (s->step == 1) {
        assert(pulse_asset_add_load_dependency(ctx, s->required_dep, PULSE_DEP_REQUIRED) == PULSE_OK);
        if (s->add_optional) {
            assert(pulse_asset_add_load_dependency(ctx, s->optional_dep, PULSE_DEP_OPTIONAL) == PULSE_OK);
        }
        return PULSE_ASSET_LOADER_WAIT_DEPENDENCIES;
    }

    dynamic_parent_asset* asset = (dynamic_parent_asset*)ctx->out_asset;
    asset->value = (int)ctx->dependency_count;
    return PULSE_ASSET_LOADER_DONE;
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

    const char hello_bytes[] = "hello world";

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

    pulse_asset_type_desc bad_desc = {0, 0, 0, 0, 0, nullptr, nullptr};
    assert(pulse_asset_register_type(app, &bad_desc) == PULSE_ERROR_INVALID_ARGUMENT);

    pulse_asset_loader_desc loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        text_type,
        "txt",
        0,
        0,
        step_test_text,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &loader_desc) == PULSE_OK);
    assert(pulse_asset_register_loader(app, &loader_desc) == PULSE_ERROR_INVALID_STATE);

    pulse_asset_handle invalid = {0, PULSE_ASSET_INVALID_INDEX, 0};
    assert(pulse_asset_get_state(app, invalid) == PULSE_ASSET_STATE_EMPTY);
    assert(!pulse_asset_is_available(app, invalid));
    assert(pulse_asset_get_error(app, invalid) == nullptr);

    pulse_asset_ref invalid_ref{};
    assert(!pulse_asset_acquire(app, invalid, &invalid_ref));
    assert(invalid_ref.ptr == nullptr);

    pulse_asset_handle text_handle = pulse_asset_load_from_memory(app, text_type, "hello.txt", hello_bytes, 11, NULL);
    assert(text_handle.type_id == text_type);
    assert(text_handle.index != PULSE_ASSET_INVALID_INDEX);
    assert(pulse_asset_get_state(app, text_handle) == PULSE_ASSET_STATE_WAITING_LOAD);

    pulse_asset_handle same_text_handle = pulse_asset_load_from_memory(app, text_type, "hello.txt", hello_bytes, 11, NULL);
    assert(same_text_handle.type_id == text_handle.type_id);
    assert(same_text_handle.index == text_handle.index);

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
        0,
        0,
        step_slow_asset,
        sizeof(slow_loader_state),
        alignof(slow_loader_state),
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &slow_loader_desc) == PULSE_OK);

    pulse_asset_handle slow_handle = pulse_asset_load_from_memory(app, slow_type, "slow.txt", hello_bytes, 11, NULL);
    assert(pulse_asset_get_state(app, slow_handle) == PULSE_ASSET_STATE_WAITING_LOAD);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, slow_handle) == PULSE_ASSET_STATE_PROCESSING);

    pulse_asset_ref slow_ref{};
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
        0,
        0,
        step_fail_asset,
        sizeof(fail_loader_state),
        alignof(fail_loader_state),
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &fail_loader_desc) == PULSE_OK);

    pulse_asset_handle fail_handle = pulse_asset_load_from_memory(app, fail_type, "fail.txt", hello_bytes, 11, NULL);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, fail_handle) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_get_error(app, fail_handle) != nullptr);
    pulse_asset_ref fail_ref{};
    assert(!pulse_asset_acquire(app, fail_handle, &fail_ref));

    // Missing file
    pulse_asset_handle missing_handle = pulse_asset_load(app, text_type, "missing.txt", NULL);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, missing_handle) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_get_error(app, missing_handle) != nullptr);

    pulse_asset_type_desc settings_type_desc = {
        sizeof(pulse_asset_type_desc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        settings_type,
        sizeof(settings_asset),
        alignof(settings_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_register_type(app, &settings_type_desc) == PULSE_OK);

    pulse_asset_loader_desc settings_loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        settings_type,
        "txt",
        nullptr,
        nullptr,
        step_settings_asset,
        0,
        0,
        sizeof(settings_loader_settings),
        alignof(settings_loader_settings),
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &settings_loader_desc) == PULSE_OK);

    settings_loader_settings stack_settings{77};
    pulse_asset_handle settings_handle = pulse_asset_load_from_memory(app, settings_type, "settings.txt", hello_bytes, 11, &stack_settings);
    stack_settings.value = 12;

    settings_loader_settings ignored_settings{99};
    pulse_asset_handle same_settings_handle = pulse_asset_load_from_memory(app, settings_type, "settings.txt", hello_bytes, 11, &ignored_settings);
    assert(same_settings_handle.index == settings_handle.index);
    assert(same_settings_handle.generation == settings_handle.generation);

    assert(pulse_app_update(app) == PULSE_OK);
    assert(settings_step_count == 1);
    pulse_asset_ref settings_ref{};
    assert(pulse_asset_acquire(app, settings_handle, &settings_ref));
    assert(((settings_asset*)settings_ref.ptr)->value == 77);
    pulse_asset_release(app, &settings_ref);

    pulse_asset_type_desc cleanup_type_desc = {
        sizeof(pulse_asset_type_desc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        cleanup_type,
        sizeof(cleanup_asset),
        alignof(cleanup_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_register_type(app, &cleanup_type_desc) == PULSE_OK);

    pulse_asset_loader_desc cleanup_loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        cleanup_type,
        "txt",
        ctor_cleanup_asset,
        dtor_cleanup_asset,
        step_cleanup_asset,
        sizeof(cleanup_loader_state),
        alignof(cleanup_loader_state),
        sizeof(int),
        alignof(int),
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &cleanup_loader_desc) == PULSE_OK);

    const char cleanup_bytes[] = "cleanup";
    int cleanup_success = 0;
    pulse_asset_handle cleanup_done = pulse_asset_load_from_memory(app, cleanup_type, "cleanup_done.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, cleanup_done) == PULSE_ASSET_STATE_PROCESSING);
    assert(cleanup_ctor_count == 1);
    assert(cleanup_dtor_count == 0);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, cleanup_done) == PULSE_ASSET_STATE_LOADED);
    assert(cleanup_dtor_count == 1);

    int cleanup_failure = 1;
    pulse_asset_handle cleanup_failed = pulse_asset_load_from_memory(app, cleanup_type, "cleanup_fail.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_failure);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, cleanup_failed) == PULSE_ASSET_STATE_FAILED);
    assert(cleanup_dtor_count == 2);

    pulse_asset_handle cleanup_unload = pulse_asset_load_from_memory(app, cleanup_type, "cleanup_unload.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, cleanup_unload) == PULSE_ASSET_STATE_PROCESSING);
    pulse_asset_unload(app, cleanup_unload);
    assert(cleanup_dtor_count == 3);

    pulse_asset_handle cleanup_shutdown = pulse_asset_load_from_memory(app, cleanup_type, "cleanup_shutdown.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, cleanup_shutdown) == PULSE_ASSET_STATE_PROCESSING);
    assert(cleanup_dtor_count == 3);

    pulse_asset_type_desc aligned_type_desc = {
        sizeof(pulse_asset_type_desc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        aligned_type,
        sizeof(aligned_asset),
        alignof(aligned_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_register_type(app, &aligned_type_desc) == PULSE_OK);

    pulse_asset_loader_desc aligned_loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        aligned_type,
        "txt",
        ctor_aligned_asset,
        nullptr,
        step_aligned_asset,
        sizeof(aligned_loader_state),
        alignof(aligned_loader_state),
        sizeof(aligned_loader_settings),
        alignof(aligned_loader_settings),
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &aligned_loader_desc) == PULSE_OK);

    aligned_loader_settings aligned_settings{123};
    pulse_asset_handle aligned_handle = pulse_asset_load_from_memory(app, aligned_type, "aligned.txt", hello_bytes, 11, &aligned_settings);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, aligned_handle) == PULSE_ASSET_STATE_LOADED);
    assert(aligned_ctor_count == 1);
    pulse_asset_ref aligned_ref{};
    assert(pulse_asset_acquire(app, aligned_handle, &aligned_ref));
    assert((((uintptr_t)aligned_ref.ptr) % alignof(aligned_asset)) == 0);
    assert(((aligned_asset*)aligned_ref.ptr)->value == 123);
    pulse_asset_release(app, &aligned_ref);

    pulse_asset_type_desc parent_type_desc = {
        sizeof(pulse_asset_type_desc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        parent_type,
        sizeof(parent_asset),
        alignof(parent_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_register_type(app, &parent_type_desc) == PULSE_OK);

    pulse_asset_loader_desc parent_loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        parent_type,
        "txt",
        nullptr,
        nullptr,
        step_parent_asset,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &parent_loader_desc) == PULSE_OK);

    pulse_asset_type_desc dynamic_parent_type_desc = {
        sizeof(pulse_asset_type_desc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        dynamic_parent_type,
        sizeof(dynamic_parent_asset),
        alignof(dynamic_parent_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_register_type(app, &dynamic_parent_type_desc) == PULSE_OK);

    pulse_asset_loader_desc dynamic_parent_loader_desc = {
        sizeof(pulse_asset_loader_desc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        dynamic_parent_type,
        "txt",
        ctor_dynamic_parent_asset,
        dtor_dynamic_parent_asset,
        step_dynamic_parent_asset,
        sizeof(dynamic_loader_state),
        alignof(dynamic_loader_state),
        sizeof(pulse_asset_handle) * 2,
        alignof(pulse_asset_handle),
        nullptr,
    };
    assert(pulse_asset_register_loader(app, &dynamic_parent_loader_desc) == PULSE_OK);

    parent_step_count = 0;
    const char dep_bytes[] = "dependency";
    const char parent_bytes[] = "parent";
    pulse_asset_handle static_dep = pulse_asset_load_from_memory(app, text_type, "static_dep.txt", dep_bytes, 10, NULL);
    pulse_asset_dependency static_deps[] = {{static_dep, PULSE_DEP_REQUIRED}};
    pulse_asset_handle static_parent = pulse_asset_load_from_memory_with_deps(
        app,
        parent_type,
        "static_parent.txt",
        parent_bytes,
        6,
        static_deps,
        1,
        NULL);
    assert(pulse_asset_get_state(app, static_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, static_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_get_state(app, static_parent) == PULSE_ASSET_STATE_PROCESSING);
    assert(parent_step_count == 0);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, static_parent) == PULSE_ASSET_STATE_LOADED);
    assert(parent_step_count == 1);
    pulse_asset_ref static_parent_ref{};
    assert(pulse_asset_acquire(app, static_parent, &static_parent_ref));
    assert(((parent_asset*)static_parent_ref.ptr)->value == 1);
    pulse_asset_release(app, &static_parent_ref);

    pulse_asset_handle static_failed_dep = pulse_asset_load_from_memory(app, fail_type, "static_failed_dep.txt", hello_bytes, 11, NULL);
    pulse_asset_dependency failed_static_deps[] = {{static_failed_dep, PULSE_DEP_REQUIRED}};
    pulse_asset_handle failed_static_parent = pulse_asset_load_from_memory_with_deps(
        app,
        parent_type,
        "failed_static_parent.txt",
        parent_bytes,
        6,
        failed_static_deps,
        1,
        NULL);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, static_failed_dep) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_get_state(app, failed_static_parent) == PULSE_ASSET_STATE_FAILED);
    pulse_asset_ref failed_static_parent_ref{};
    assert(!pulse_asset_acquire(app, failed_static_parent, &failed_static_parent_ref));

    dynamic_step_count = 0;
    dynamic_ctor_count = 0;
    dynamic_dtor_count = 0;
    pulse_asset_handle dynamic_required_dep = pulse_asset_load_from_memory(app, slow_type, "dynamic_required_dep.txt", hello_bytes, 11, NULL);
    pulse_asset_handle no_optional = {0, PULSE_ASSET_INVALID_INDEX, 0};
    pulse_asset_handle dynamic_settings[] = {dynamic_required_dep, no_optional};
    pulse_asset_handle dynamic_parent = pulse_asset_load_from_memory(
        app,
        dynamic_parent_type,
        "dynamic_parent.txt",
        parent_bytes,
        6,
        dynamic_settings);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, dynamic_required_dep) == PULSE_ASSET_STATE_PROCESSING);
    assert(pulse_asset_get_state(app, dynamic_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(dynamic_step_count == 1);
    assert(dynamic_ctor_count == 1);
    assert(dynamic_dtor_count == 0);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, dynamic_required_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_get_state(app, dynamic_parent) == PULSE_ASSET_STATE_LOADED);
    assert(dynamic_step_count == 2);
    assert(dynamic_dtor_count == 1);
    pulse_asset_ref dynamic_parent_ref{};
    assert(pulse_asset_acquire(app, dynamic_parent, &dynamic_parent_ref));
    assert(((dynamic_parent_asset*)dynamic_parent_ref.ptr)->value == 1);
    pulse_asset_release(app, &dynamic_parent_ref);

    pulse_asset_handle dynamic_failed_dep = pulse_asset_load_from_memory(app, fail_type, "dynamic_failed_dep.txt", hello_bytes, 11, NULL);
    pulse_asset_handle dynamic_failed_settings[] = {dynamic_failed_dep, no_optional};
    pulse_asset_handle dynamic_failed_parent = pulse_asset_load_from_memory(
        app,
        dynamic_parent_type,
        "dynamic_failed_parent.txt",
        parent_bytes,
        6,
        dynamic_failed_settings);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, dynamic_failed_dep) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_get_state(app, dynamic_failed_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, dynamic_failed_parent) == PULSE_ASSET_STATE_FAILED);

    pulse_asset_handle dynamic_ready_dep = pulse_asset_load_from_memory(app, text_type, "dynamic_ready_dep.txt", hello_bytes, 11, NULL);
    pulse_asset_handle dynamic_optional_failed_dep = pulse_asset_load_from_memory(app, fail_type, "dynamic_optional_failed_dep.txt", hello_bytes, 11, NULL);
    pulse_asset_handle dynamic_optional_settings[] = {dynamic_ready_dep, dynamic_optional_failed_dep};
    pulse_asset_handle dynamic_optional_parent = pulse_asset_load_from_memory(
        app,
        dynamic_parent_type,
        "dynamic_optional_parent.txt",
        parent_bytes,
        6,
        dynamic_optional_settings);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, dynamic_ready_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_get_state(app, dynamic_optional_failed_dep) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_get_state(app, dynamic_optional_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(pulse_app_update(app) == PULSE_OK);
    assert(pulse_asset_get_state(app, dynamic_optional_parent) == PULSE_ASSET_STATE_LOADED);
    pulse_asset_ref dynamic_optional_parent_ref{};
    assert(pulse_asset_acquire(app, dynamic_optional_parent, &dynamic_optional_parent_ref));
    assert(((dynamic_parent_asset*)dynamic_optional_parent_ref.ptr)->value == 2);
    pulse_asset_release(app, &dynamic_optional_parent_ref);

    // Bad generation
    pulse_asset_handle bad_generation = text_handle;
    bad_generation.generation += 1;
    assert(pulse_asset_get_state(app, bad_generation) == PULSE_ASSET_STATE_EMPTY);
    assert(!pulse_asset_acquire(app, bad_generation, &fail_ref));

    // Unload test - pin_count starts at 1 after load
    pulse_asset_unload(app, text_handle);
    // After unload, pin_count should be 0 and asset destroyed
    assert(destroy_count == 1);

    pulse_app_destroy(app);
    assert(cleanup_dtor_count == 4);

    printf("Asset plugin lifecycle tests passed!\n");
    return 0;
}
