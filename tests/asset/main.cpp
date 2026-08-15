#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

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
const uint64_t self_cancel_type = 9;
const uint64_t dtor_unload_type = 10;
const uint64_t builder_type = 11;
const uint64_t builder_pending_type = 12;
const uint64_t builder_wait_type = 13;
const uint64_t builder_dynamic_type = 14;
const uint64_t builder_fail_once_type = 15;
const uint64_t force_type = 16;
const uint64_t deep_settings_type = 17;
const uint64_t deep_fail_copy_type = 18;
const uint64_t builder_named_type = 19;

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

struct self_cancel_asset {
    int value;
};

struct dtor_unload_asset {
    int value;
};

struct builder_asset {
    int value;
};

struct builder_settings {
    int value;
    const int* external;
};

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

struct builder_pending_state {
    int step;
};

struct builder_dynamic_state {
    int step;
    PulseAssetRequest required_dep;
};

struct force_asset {
    int value;
};

struct force_loader_state {
    int step;
};

struct dynamic_loader_state {
    int step;
    PulseAssetRequest required_dep;
    PulseAssetRequest optional_dep;
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
static int self_cancel_step_count = 0;
static int self_cancel_ctor_count = 0;
static int self_cancel_dtor_count = 0;
static int dtor_unload_step_count = 0;
static int dtor_unload_dtor_count = 0;
static int builder_step_count = 0;
static int builder_fail_once_step_count = 0;
static int builder_pending_step_count = 0;
static int builder_pending_ctor_count = 0;
static int builder_pending_dtor_count = 0;
static int builder_wait_step_count = 0;
static int builder_dynamic_step_count = 0;
static int builder_dynamic_ctor_count = 0;
static int builder_dynamic_dtor_count = 0;
static int force_destroy_count = 0;
static int force_ctor_count = 0;
static int force_dtor_count = 0;
static int force_step_count = 0;
static int deep_settings_size_count = 0;
static int deep_settings_copy_count = 0;

static void destroy_test_text(void* ptr, void* user_data) {
    (void)ptr;
    int* counter = (int*)user_data;
    *counter += 1;
}

static EPulseAssetLoaderStatus step_test_text(
    void* state,
    const PulseAssetLoadTask* ctx,
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
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseAssetLoaderStatus step_slow_asset(
    void* raw_state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)ctx;
    (void)out_error;
    slow_loader_state* s = (slow_loader_state*)raw_state;
    s->step += 1;
    if (s->step < 2) {
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }
    slow_asset* asset = (slow_asset*)ctx->out_asset;
    asset->value = 42;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseAssetLoaderStatus step_fail_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    fail_loader_state* s = (fail_loader_state*)state;
    assert(s->marker == 0);
    (void)ctx;
    *out_error = "intentional failure";
    return PULSE_ASSET_LOADER_STATUS_FAILED;
}

static EPulseAssetLoaderStatus step_settings_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    settings_step_count += 1;
    settings_asset* asset = (settings_asset*)ctx->out_asset;
    const settings_loader_settings* settings = (const settings_loader_settings*)ctx->settings;
    asset->value = settings ? settings->value : -1;
    assert((((uintptr_t)ctx->settings) % alignof(settings_loader_settings)) == 0);
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseResult ctor_cleanup_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    cleanup_ctor_count += 1;
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    assert(s->step == 0);
    assert(s->fail == 0);
    s->fail = ctx->settings ? *((const int*)ctx->settings) : 0;
    return PULSE_RESULT_OK;
}

static void dtor_cleanup_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)ctx;
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    assert(s != nullptr);
    cleanup_dtor_count += 1;
}

static EPulseAssetLoaderStatus step_cleanup_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    cleanup_loader_state* s = (cleanup_loader_state*)state;
    s->step += 1;
    if (s->fail) {
        *out_error = "cleanup loader intentional failure";
        return PULSE_ASSET_LOADER_STATUS_FAILED;
    }
    if (s->step < 2) {
        return PULSE_ASSET_LOADER_STATUS_PENDING;
    }
    cleanup_asset* asset = (cleanup_asset*)ctx->out_asset;
    asset->value = 9;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static EPulseResult ctor_aligned_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)ctx;
    assert((((uintptr_t)state) % alignof(aligned_loader_state)) == 0);
    aligned_loader_state* s = (aligned_loader_state*)state;
    assert(s->constructed == 0);
    s->constructed = 1;
    aligned_ctor_count += 1;
    return PULSE_RESULT_OK;
}

static EPulseAssetLoaderStatus step_aligned_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)out_error;
    assert((((uintptr_t)state) % alignof(aligned_loader_state)) == 0);
    assert((((uintptr_t)ctx->settings) % alignof(aligned_loader_settings)) == 0);
    assert((((uintptr_t)ctx->out_asset) % alignof(aligned_asset)) == 0);
    aligned_asset* asset = (aligned_asset*)ctx->out_asset;
    const aligned_loader_settings* settings = (const aligned_loader_settings*)ctx->settings;
    asset->value = settings->value;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

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

static EPulseResult ctor_self_cancel_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)state;
    self_cancel_ctor_count += 1;
    assert(ctx->dependency_hint == nullptr);
    assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, ctx->request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
    return PULSE_RESULT_OK;
}

static void dtor_self_cancel_asset(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)state;
    self_cancel_dtor_count += 1;
    assert(ctx->dependency_hint == nullptr);
    assert(pulse_asset_load_task_add_dependency(ctx->dependency_hint, pulse_asset_system_to_asset_dep_ref_from_request(ctx->asset_system, ctx->request), PULSE_LOAD_DEPENDENCY_REQUIREMENT_OPTIONAL) == PULSE_RESULT_ERROR_INVALID_ARGUMENT);
}

static EPulseAssetLoaderStatus step_self_cancel_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    self_cancel_step_count += 1;
    assert(ctx->dependency_hint != nullptr);
    pulse_asset_system_cancel(ctx->asset_system, ctx->request);
    self_cancel_asset* asset = (self_cancel_asset*)ctx->out_asset;
    asset->value = 101;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

static void dtor_unload_asset_loader(
    void* state,
    const PulseAssetLoadTask* ctx
) {
    (void)state;
    dtor_unload_dtor_count += 1;
    assert(ctx->dependency_hint == nullptr);
    pulse_asset_system_cancel(ctx->asset_system, ctx->request);
}

static EPulseAssetLoaderStatus step_dtor_unload_asset(
    void* state,
    const PulseAssetLoadTask* ctx,
    const char** out_error
) {
    (void)state;
    (void)out_error;
    dtor_unload_step_count += 1;
    dtor_unload_asset* asset = (dtor_unload_asset*)ctx->out_asset;
    asset->value = 202;
    return PULSE_ASSET_LOADER_STATUS_DONE;
}

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

static PulseAssetRequest load_asset_file(
    PulseAssetSystemId assetSystem,
    uint64_t type_id,
    const char* path,
    const void* settings
) {
    PulseAssetLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetLoadDesc);
    desc.version = PULSE_ASSET_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    desc.settings = settings;
    return pulse_asset_system_load(assetSystem, &desc);
}

static PulseAssetRequest load_asset_memory(
    PulseAssetSystemId assetSystem,
    uint64_t type_id,
    const char* path,
    const void* data,
    uint64_t size,
    const void* settings
) {
    PulseAssetMemoryLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetMemoryLoadDesc);
    desc.version = PULSE_ASSET_MEMORY_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    desc.data = data;
    desc.size = size;
    desc.settings = settings;
    return pulse_asset_system_load_from_memory(assetSystem, &desc);
}

static PulseAssetRequest load_asset_memory_with_deps(
    PulseAssetSystemId assetSystem,
    uint64_t type_id,
    const char* path,
    const void* data,
    uint64_t size,
    const PulseAssetDependency* dependencies,
    uint32_t dependency_count,
    const void* settings
) {
    PulseAssetMemoryLoadDesc desc{};
    desc.struct_size = sizeof(PulseAssetMemoryLoadDesc);
    desc.version = PULSE_ASSET_MEMORY_LOAD_DESC_VERSION;
    desc.type_id = type_id;
    desc.path = path;
    desc.data = data;
    desc.size = size;
    desc.dependencies = dependencies;
    desc.dependency_count = dependency_count;
    desc.settings = settings;
    return pulse_asset_system_load_from_memory(assetSystem, &desc);
}

int main(void) {
    PulseAssetPluginDesc default_desc = pulse_asset_plugin_desc_default();
    assert(default_desc.struct_size == sizeof(PulseAssetPluginDesc));
    assert(default_desc.version == PULSE_ASSET_PLUGIN_DESC_VERSION);
    assert(default_desc.root_path != nullptr);
    assert(default_desc.max_requests_per_update == 8);

    PulseAppDesc app_desc = {
        .name = "test-asset",
    };
    PulseAppId app = pulse_create_app(&app_desc);
    assert(app != nullptr);

    PulseAssetPluginDesc desc = pulse_asset_plugin_desc_default();
    desc.root_path = "tests/asset/data";
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_RESULT_OK);
    assert(pulse_app_has_plugin(app, "PulseAssetPlugin"));
    assert(pulse_add_asset_plugin(app, &desc) == PULSE_RESULT_ERROR_DUPLICATE_PLUGIN);

    PulseAssetSystemId assetSystem = pulse_get_asset_system(app);

    const char hello_bytes[] = "hello world";

    PulseAssetTypeDesc type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        1,
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
        0,
        0,
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

    PulseAssetRequest text_request = load_asset_memory(assetSystem, text_type, "hello.txt", hello_bytes, 11, NULL);
    assert(text_request.type_id == text_type);
    assert(pulse_asset_request_is_valid(text_request));
    assert(text_request.index != PULSE_ASSET_INVALID_INDEX);
    assert(pulse_asset_system_get_state(assetSystem, text_request) == PULSE_ASSET_STATE_WAITING_LOAD);

    PulseAssetRequest same_text_request = load_asset_memory(assetSystem, text_type, "hello.txt", hello_bytes, 11, NULL);
    assert(pulse_asset_request_equals(same_text_request, text_request));

    assert(load_asset_memory(assetSystem, text_type, "", hello_bytes, 11, NULL).index == PULSE_ASSET_INVALID_INDEX);
    assert(load_asset_memory(assetSystem, text_type, "hello", hello_bytes, 11, NULL).index == PULSE_ASSET_INVALID_INDEX);
    assert(load_asset_memory(assetSystem, text_type, "hello.txt", nullptr, 11, NULL).index == PULSE_ASSET_INVALID_INDEX);
    assert(load_asset_memory(assetSystem, text_type, "hello.txt", hello_bytes, 0, NULL).index == PULSE_ASSET_INVALID_INDEX);
    assert(load_asset_file(assetSystem, text_type, "missing.bin", NULL).index == PULSE_ASSET_INVALID_INDEX);

    const char cache_bytes[] = "cache-one";
    const char cache_new_bytes[] = "cache-two";
    PulseAssetRequest cache_handle = load_asset_memory(assetSystem, text_type, "cache.txt", cache_bytes, 9, NULL);
    PulseAssetRequest same_cache_handle = load_asset_memory(assetSystem, text_type, "cache.txt", cache_new_bytes, 9, NULL);
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

    PulseAssetRequest latest_cache_handle = load_asset_memory(assetSystem, text_type, "cache.txt", cache_bytes, 9, NULL);
    assert(latest_cache_handle.index == refreshed_cache_handle.index);
    assert(latest_cache_handle.generation == refreshed_cache_handle.generation);

    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, text_request) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_is_ready(assetSystem, text_request));

    PulseAssetHandle text_handle = pulse_asset_system_get_handle(assetSystem, text_request);
    assert(pulse_asset_handle_is_valid(text_handle));
    void* text_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, text_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, text_handle, &text_ptr, nullptr));
    assert(text_ptr != nullptr);
    test_text_asset* text_asset = (test_text_asset*)text_ptr;
    assert(text_asset->size == 11);
    assert(text_asset->text[0] == 'h');

    text_asset->text[0] = 'H';
    pulse_asset_system_mark_modified(assetSystem, text_handle);

    void* second_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, text_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, text_handle, &second_ptr, nullptr));
    assert(second_ptr == text_ptr);
    pulse_asset_system_release(assetSystem, text_handle, nullptr);

    pulse_asset_system_release(assetSystem, text_handle, nullptr);

    // Multi-step pending loader
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
    assert(slow_type != 0 && slow_type != text_type);

    PulseAssetLoaderDesc slow_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        slow_type,
        "txt",
        nullptr,
        0,
        0,
        step_slow_asset,
        sizeof(slow_loader_state),
        alignof(slow_loader_state),
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &slow_loader_desc) == PULSE_RESULT_OK);

    PulseAssetRequest slow_request = load_asset_memory(assetSystem, slow_type, "slow.txt", hello_bytes, 11, NULL);
    assert(pulse_asset_system_get_state(assetSystem, slow_request) == PULSE_ASSET_STATE_WAITING_LOAD);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, slow_request) == PULSE_ASSET_STATE_PROCESSING);

    void* slow_ptr = nullptr;
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, slow_request) == PULSE_ASSET_STATE_LOADED);
    PulseAssetHandle slow_handle = pulse_asset_system_get_handle(assetSystem, slow_request);
    assert(pulse_asset_system_retain(assetSystem, slow_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, slow_handle, &slow_ptr, nullptr));
    assert(((slow_asset*)slow_ptr)->value == 42);
    pulse_asset_system_release(assetSystem, slow_handle, nullptr);

    // Failure paths
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
        0,
        0,
        step_fail_asset,
        sizeof(fail_loader_state),
        alignof(fail_loader_state),
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &fail_loader_desc) == PULSE_RESULT_OK);

    PulseAssetRequest fail_handle = load_asset_memory(assetSystem, fail_type, "fail.txt", hello_bytes, 11, NULL);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, fail_handle) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_error(assetSystem, fail_handle) != nullptr);
    void* fail_ptr = nullptr;
    assert(!pulse_asset_system_borrow(assetSystem, pulse_asset_system_get_handle(assetSystem, fail_handle), &fail_ptr, nullptr));

    // Missing file
    PulseAssetRequest missing_handle = load_asset_file(assetSystem, text_type, "missing.txt", NULL);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, missing_handle) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_error(assetSystem, missing_handle) != nullptr);

    PulseAssetTypeDesc settings_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        settings_type,
        sizeof(settings_asset),
        alignof(settings_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &settings_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc settings_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        settings_type,
        "txt",
        nullptr,
        nullptr,
        nullptr,
        step_settings_asset,
        0,
        0,
        sizeof(settings_loader_settings),
        alignof(settings_loader_settings),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &settings_loader_desc) == PULSE_RESULT_OK);

    settings_loader_settings stack_settings{77};
    PulseAssetRequest settings_request = load_asset_memory(assetSystem, settings_type, "settings.txt", hello_bytes, 11, &stack_settings);
    stack_settings.value = 12;

    settings_loader_settings ignored_settings{99};
    PulseAssetRequest same_settings_request = load_asset_memory(assetSystem, settings_type, "settings.txt", hello_bytes, 11, &ignored_settings);
    assert(same_settings_request.index == settings_request.index);
    assert(same_settings_request.generation == settings_request.generation);

    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(settings_step_count == 1);
    PulseAssetHandle settings_handle = pulse_asset_system_get_handle(assetSystem, settings_request);
    void* settings_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, settings_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, settings_handle, &settings_ptr, nullptr));
    assert(((settings_asset*)settings_ptr)->value == 77);
    pulse_asset_system_release(assetSystem, settings_handle, nullptr);

    PulseAssetTypeDesc cleanup_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        cleanup_type,
        sizeof(cleanup_asset),
        alignof(cleanup_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &cleanup_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc cleanup_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        cleanup_type,
        "txt",
        nullptr,
        ctor_cleanup_asset,
        dtor_cleanup_asset,
        step_cleanup_asset,
        sizeof(cleanup_loader_state),
        alignof(cleanup_loader_state),
        sizeof(int),
        alignof(int),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &cleanup_loader_desc) == PULSE_RESULT_OK);

    const char cleanup_bytes[] = "cleanup";
    int cleanup_success = 0;
    PulseAssetRequest cleanup_done = load_asset_memory(assetSystem, cleanup_type, "cleanup_done.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_done) == PULSE_ASSET_STATE_PROCESSING);
    assert(cleanup_ctor_count == 1);
    assert(cleanup_dtor_count == 0);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_done) == PULSE_ASSET_STATE_LOADED);
    assert(cleanup_dtor_count == 1);

    int cleanup_failure = 1;
    PulseAssetRequest cleanup_failed = load_asset_memory(assetSystem, cleanup_type, "cleanup_fail.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_failure);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_failed) == PULSE_ASSET_STATE_FAILED);
    assert(cleanup_dtor_count == 2);

    PulseAssetRequest cleanup_unload = load_asset_memory(assetSystem, cleanup_type, "cleanup_unload.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_unload) == PULSE_ASSET_STATE_PROCESSING);
    pulse_asset_system_cancel(assetSystem, cleanup_unload);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_unload) == PULSE_ASSET_STATE_PENDING_DELETE);
    assert(cleanup_dtor_count == 2);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_unload) == PULSE_ASSET_STATE_EMPTY);
    assert(cleanup_dtor_count == 3);

    PulseAssetRequest cleanup_shutdown = load_asset_memory(assetSystem, cleanup_type, "cleanup_shutdown.txt", cleanup_bytes, sizeof(cleanup_bytes), &cleanup_success);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, cleanup_shutdown) == PULSE_ASSET_STATE_PROCESSING);
    assert(cleanup_dtor_count == 3);

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

    const char force_bytes[] = "force";
    PulseAssetRequest force_loaded = load_asset_memory(assetSystem, force_type, "force_loaded.txt", force_bytes, sizeof(force_bytes), NULL);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, force_loaded) == PULSE_ASSET_STATE_PROCESSING);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, force_loaded) == PULSE_ASSET_STATE_LOADED);
    assert(force_ctor_count == 1);
    assert(force_dtor_count == 1);

    PulseAssetHandle force_loaded_handle = pulse_asset_system_get_handle(assetSystem, force_loaded);
    void* force_loaded_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, force_loaded_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, force_loaded_handle, &force_loaded_ptr, nullptr));
    assert(((force_asset*)force_loaded_ptr)->value == sizeof(force_bytes));

    PulseAssetRequest force_pending = load_asset_memory(assetSystem, force_type, "force_pending.txt", force_bytes, sizeof(force_bytes), NULL);
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

    PulseAssetRequest force_reloaded = load_asset_memory(assetSystem, force_type, "force_loaded.txt", force_bytes, sizeof(force_bytes), NULL);
    assert(pulse_asset_request_is_valid(force_reloaded));
    assert(!pulse_asset_request_equals(force_reloaded, force_loaded));
    pulse_asset_system_force_unload_assets(assetSystem, 0);

    PulseAssetTypeDesc aligned_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        aligned_type,
        sizeof(aligned_asset),
        alignof(aligned_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &aligned_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc aligned_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        aligned_type,
        "txt",
        nullptr,
        ctor_aligned_asset,
        nullptr,
        step_aligned_asset,
        sizeof(aligned_loader_state),
        alignof(aligned_loader_state),
        sizeof(aligned_loader_settings),
        alignof(aligned_loader_settings),
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &aligned_loader_desc) == PULSE_RESULT_OK);

    aligned_loader_settings aligned_settings{123};
    PulseAssetRequest aligned_request = load_asset_memory(assetSystem, aligned_type, "aligned.txt", hello_bytes, 11, &aligned_settings);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, aligned_request) == PULSE_ASSET_STATE_LOADED);
    assert(aligned_ctor_count == 1);
    PulseAssetHandle aligned_handle = pulse_asset_system_get_handle(assetSystem, aligned_request);
    void* aligned_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, aligned_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, aligned_handle, &aligned_ptr, nullptr));
    assert((((uintptr_t)aligned_ptr) % alignof(aligned_asset)) == 0);
    assert(((aligned_asset*)aligned_ptr)->value == 123);
    pulse_asset_system_release(assetSystem, aligned_handle, nullptr);

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

    PulseAssetTypeDesc self_cancel_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        self_cancel_type,
        sizeof(self_cancel_asset),
        alignof(self_cancel_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &self_cancel_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc self_cancel_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        self_cancel_type,
        "txt",
        nullptr,
        ctor_self_cancel_asset,
        dtor_self_cancel_asset,
        step_self_cancel_asset,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &self_cancel_loader_desc) == PULSE_RESULT_OK);

    PulseAssetTypeDesc dtor_unload_type_desc = {
        sizeof(PulseAssetTypeDesc),
        PULSE_ASSET_TYPE_DESC_VERSION,
        dtor_unload_type,
        sizeof(dtor_unload_asset),
        alignof(dtor_unload_asset),
        nullptr,
        nullptr,
    };
    assert(pulse_asset_system_register_type(assetSystem, &dtor_unload_type_desc) == PULSE_RESULT_OK);

    PulseAssetLoaderDesc dtor_unload_loader_desc = {
        sizeof(PulseAssetLoaderDesc),
        PULSE_ASSET_LOADER_DESC_VERSION,
        dtor_unload_type,
        "txt",
        nullptr,
        nullptr,
        dtor_unload_asset_loader,
        step_dtor_unload_asset,
        0,
        0,
        0,
        0,
        nullptr,
    };
    assert(pulse_asset_system_register_loader(assetSystem, &dtor_unload_loader_desc) == PULSE_RESULT_OK);

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

    // ---- 同一类型可注册多个命名 builder，build 请求按 name 选择 loader ----
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
    // 同名 builder 重复注册仍拒绝
    assert(pulse_asset_system_register_loader(assetSystem, &builder_named_a_desc) == PULSE_RESULT_ERROR_INVALID_STATE);

    PulseAssetLoaderDesc builder_named_b_desc = builder_named_a_desc;
    builder_named_b_desc.step = step_builder_named_b;
    builder_named_b_desc.loader_identifier = "named-b";
    assert(pulse_asset_system_register_loader(assetSystem, &builder_named_b_desc) == PULSE_RESULT_OK);

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
    void* named_a_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, named_a_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, named_a_handle, &named_a_ptr, nullptr));
    assert(((builder_asset*)named_a_ptr)->value == 1001);
    pulse_asset_system_release(assetSystem, named_a_handle, nullptr);

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
    void* named_b_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, named_b_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, named_b_handle, &named_b_ptr, nullptr));
    assert(((builder_asset*)named_b_ptr)->value == 2002);
    pulse_asset_system_release(assetSystem, named_b_handle, nullptr);

    // 未注册的 name：无匹配且无默认 builder -> 加载失败
    PulseAssetBuildDesc unknown_name_build{};
    unknown_name_build.struct_size = sizeof(PulseAssetBuildDesc);
    unknown_name_build.version = PULSE_ASSET_BUILD_DESC_VERSION;
    unknown_name_build.type_id = builder_named_type;
    unknown_name_build.name = "no-such-builder";
    assert(pulse_asset_system_build(assetSystem, &unknown_name_build).index == PULSE_ASSET_INVALID_INDEX);

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
    void* recovered_builder_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, recovered_builder_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, recovered_builder_handle, &recovered_builder_ptr, nullptr));
    assert(((builder_asset*)recovered_builder_ptr)->value == 77);
    pulse_asset_system_release(assetSystem, recovered_builder_handle, nullptr);

    PulseAssetBuildDesc missing_builder_desc{};
    missing_builder_desc.struct_size = sizeof(PulseAssetBuildDesc);
    missing_builder_desc.version = PULSE_ASSET_BUILD_DESC_VERSION;
    missing_builder_desc.type_id = text_type;
    assert(pulse_asset_system_build(assetSystem, &missing_builder_desc).index == PULSE_ASSET_INVALID_INDEX);

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
    void* builder_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, builder_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, builder_handle, &builder_ptr, nullptr));
    assert(((builder_asset*)builder_ptr)->value == 42);
    pulse_asset_system_release(assetSystem, builder_handle, nullptr);

    PulseAssetRequest second_builder_request = pulse_asset_system_build(assetSystem, &builder_desc);
    assert(second_builder_request.index != PULSE_ASSET_INVALID_INDEX);
    assert(second_builder_request.index != builder_request.index ||
           second_builder_request.generation != builder_request.generation);
    assert(builder_step_count == 2);

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
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_pending_request) == PULSE_ASSET_STATE_LOADED);
    assert(builder_pending_step_count == 2);
    assert(builder_pending_dtor_count == 1);
    PulseAssetHandle builder_pending_handle = pulse_asset_system_get_handle(assetSystem, builder_pending_request);
    void* builder_pending_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, builder_pending_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, builder_pending_handle, &builder_pending_ptr, nullptr));
    assert(((builder_asset*)builder_pending_ptr)->value == 17);
    pulse_asset_system_release(assetSystem, builder_pending_handle, nullptr);

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

    PulseAssetRequest builder_required_dep = load_asset_memory(assetSystem, slow_type, "builder_required_dep.txt", hello_bytes, 11, NULL);
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
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_required_dep) == PULSE_ASSET_STATE_PROCESSING);
    assert(pulse_asset_system_get_state(assetSystem, builder_wait_handle) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(builder_wait_step_count == 0);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_required_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, builder_wait_handle) == PULSE_ASSET_STATE_PROCESSING);
    assert(builder_wait_step_count == 0);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_wait_handle) == PULSE_ASSET_STATE_LOADED);
    assert(builder_wait_step_count == 1);
    PulseAssetHandle builder_wait_handle_value = pulse_asset_system_get_handle(assetSystem, builder_wait_handle);
    void* builder_wait_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, builder_wait_handle_value, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, builder_wait_handle_value, &builder_wait_ptr, nullptr));
    assert(((builder_asset*)builder_wait_ptr)->value == 1);
    pulse_asset_system_release(assetSystem, builder_wait_handle_value, nullptr);

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

    PulseAssetRequest builder_dynamic_dep = load_asset_memory(assetSystem, slow_type, "builder_dynamic_dep.txt", hello_bytes, 11, NULL);
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
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_dep) == PULSE_ASSET_STATE_PROCESSING);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_request) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(builder_dynamic_step_count == 1);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, builder_dynamic_request) == PULSE_ASSET_STATE_LOADED);
    assert(builder_dynamic_step_count == 2);
    assert(builder_dynamic_dtor_count == 1);
    PulseAssetHandle builder_dynamic_handle = pulse_asset_system_get_handle(assetSystem, builder_dynamic_request);
    void* builder_dynamic_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, builder_dynamic_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, builder_dynamic_handle, &builder_dynamic_ptr, nullptr));
    assert(((builder_asset*)builder_dynamic_ptr)->value == 1);
    pulse_asset_system_release(assetSystem, builder_dynamic_handle, nullptr);

    parent_step_count = 0;
    const char dep_bytes[] = "dependency";
    const char parent_bytes[] = "parent";
    PulseAssetRequest static_dep = load_asset_memory(assetSystem, text_type, "static_dep.txt", dep_bytes, 10, NULL);
    PulseAssetDependency static_deps[] = {{pulse_asset_system_to_asset_dep_ref_from_request(assetSystem, static_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED}};
    PulseAssetRequest static_parent = load_asset_memory_with_deps(
        assetSystem,
        parent_type,
        "static_parent.txt",
        parent_bytes,
        6,
        static_deps,
        1,
        NULL);
    assert(pulse_asset_system_get_state(assetSystem, static_parent) == PULSE_ASSET_STATE_WAITING_DEPENDENCIES);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, static_dep) == PULSE_ASSET_STATE_LOADED);
    assert(pulse_asset_system_get_state(assetSystem, static_parent) == PULSE_ASSET_STATE_PROCESSING);
    assert(parent_step_count == 0);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, static_parent) == PULSE_ASSET_STATE_LOADED);
    assert(parent_step_count == 1);
    PulseAssetHandle static_parent_handle = pulse_asset_system_get_handle(assetSystem, static_parent);
    void* static_parent_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, static_parent_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, static_parent_handle, &static_parent_ptr, nullptr));
    assert(((parent_asset*)static_parent_ptr)->value == 1);
    pulse_asset_system_release(assetSystem, static_parent_handle, nullptr);

    PulseAssetRequest static_failed_dep = load_asset_memory(assetSystem, fail_type, "static_failed_dep.txt", hello_bytes, 11, NULL);
    PulseAssetDependency failed_static_deps[] = {{pulse_asset_system_to_asset_dep_ref_from_request(assetSystem, static_failed_dep), PULSE_LOAD_DEPENDENCY_REQUIREMENT_REQUIRED}};
    PulseAssetRequest failed_static_parent = load_asset_memory_with_deps(
        assetSystem,
        parent_type,
        "failed_static_parent.txt",
        parent_bytes,
        6,
        failed_static_deps,
        1,
        NULL);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(pulse_asset_system_get_state(assetSystem, static_failed_dep) == PULSE_ASSET_STATE_FAILED);
    assert(pulse_asset_system_get_state(assetSystem, failed_static_parent) == PULSE_ASSET_STATE_FAILED);
    void* failed_static_parent_ptr = nullptr;
    assert(!pulse_asset_system_borrow(assetSystem, pulse_asset_system_get_handle(assetSystem, failed_static_parent), &failed_static_parent_ptr, nullptr));

    dynamic_step_count = 0;
    dynamic_ctor_count = 0;
    dynamic_dtor_count = 0;
    PulseAssetRequest dynamic_required_dep = load_asset_memory(assetSystem, slow_type, "dynamic_required_dep.txt", hello_bytes, 11, NULL);
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
    void* dynamic_parent_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, dynamic_parent_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, dynamic_parent_handle, &dynamic_parent_ptr, nullptr));
    assert(((dynamic_parent_asset*)dynamic_parent_ptr)->value == 1);
    pulse_asset_system_release(assetSystem, dynamic_parent_handle, nullptr);

    PulseAssetRequest dynamic_failed_dep = load_asset_memory(assetSystem, fail_type, "dynamic_failed_dep.txt", hello_bytes, 11, NULL);
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

    PulseAssetRequest dynamic_ready_dep = load_asset_memory(assetSystem, text_type, "dynamic_ready_dep.txt", hello_bytes, 11, NULL);
    PulseAssetRequest dynamic_optional_failed_dep = load_asset_memory(assetSystem, fail_type, "dynamic_optional_failed_dep.txt", hello_bytes, 11, NULL);
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
    void* dynamic_optional_parent_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, dynamic_optional_parent_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, dynamic_optional_parent_handle, &dynamic_optional_parent_ptr, nullptr));
    assert(((dynamic_parent_asset*)dynamic_optional_parent_ptr)->value == 2);
    pulse_asset_system_release(assetSystem, dynamic_optional_parent_handle, nullptr);

    self_cancel_step_count = 0;
    self_cancel_ctor_count = 0;
    self_cancel_dtor_count = 0;
    PulseAssetRequest self_cancel_handle = load_asset_memory(assetSystem, self_cancel_type, "self_cancel.txt", parent_bytes, 6, NULL);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(self_cancel_ctor_count == 1);
    assert(self_cancel_step_count == 1);
    assert(self_cancel_dtor_count == 1);
    assert(pulse_asset_system_get_state(assetSystem, self_cancel_handle) == PULSE_ASSET_STATE_EMPTY);

    dtor_unload_step_count = 0;
    dtor_unload_dtor_count = 0;
    PulseAssetRequest dtor_unload_handle = load_asset_memory(assetSystem, dtor_unload_type, "dtor_unload.txt", parent_bytes, 6, NULL);
    assert(pulse_app_update(app) == PULSE_RESULT_OK);
    assert(dtor_unload_step_count == 1);
    assert(dtor_unload_dtor_count == 1);
    assert(pulse_asset_system_get_state(assetSystem, dtor_unload_handle) == PULSE_ASSET_STATE_EMPTY);

    // Deep-copy settings: size/copy callbacks deep-copy nested data into one block
    // allocated by the asset system; mutating the caller's data afterwards must not
    // affect the loader.
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
    void* deep_ptr = nullptr;
    assert(pulse_asset_system_retain(assetSystem, deep_handle, nullptr));
    assert(pulse_asset_system_borrow(assetSystem, deep_handle, &deep_ptr, nullptr));
    deep_settings_asset* deep_asset = (deep_settings_asset*)deep_ptr;
    assert(deep_asset->value == 10 + 1 + 2 + 3);
    int expected_hash = 0;
    for (const char* p = "deep-settings-name"; *p; ++p) {
        expected_hash = expected_hash * 31 + *p;
    }
    assert(deep_asset->name_hash == expected_hash);
    pulse_asset_system_release(assetSystem, deep_handle, nullptr);

    // Copy callback failure must fail the request (the block is released with the job)
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

    PulseAssetBuildDesc deep_fail_desc = deep_desc;
    deep_fail_desc.type_id = deep_fail_copy_type;
    deep_fail_desc.name = "deep-fail-builder";
    PulseAssetRequest deep_fail_request = pulse_asset_system_build(assetSystem, &deep_fail_desc);
    assert(!pulse_asset_request_is_valid(deep_fail_request));

    // Bad generation
    PulseAssetRequest bad_generation = text_request;
    bad_generation.generation += 1;
    assert(pulse_asset_system_get_state(assetSystem, bad_generation) == PULSE_ASSET_STATE_EMPTY);
    assert(!pulse_asset_system_borrow(assetSystem, pulse_asset_system_get_handle(assetSystem, bad_generation), &fail_ptr, nullptr));

    // Unload test - pin_count starts at 1 after load
    pulse_asset_system_release(assetSystem, text_handle, nullptr);
    // After unload, pin_count should be 0 and asset destroyed
    assert(destroy_count == 1);
    assert(pulse_asset_system_get_state(assetSystem, text_request) == PULSE_ASSET_STATE_EMPTY);

    PulseAssetRequest reused_text_request = load_asset_memory(assetSystem, text_type, "reuse_after_unload.txt", hello_bytes, 11, NULL);
    assert(reused_text_request.index == text_request.index);
    assert(reused_text_request.generation == text_request.generation + 1);
    assert(pulse_asset_request_is_valid(reused_text_request));

    pulse_destroy_app(app);
    assert(cleanup_dtor_count == 4);

    printf("Asset plugin lifecycle tests passed!\n");
    return 0;
}
