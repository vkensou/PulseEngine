// Shared asset test definitions, cut from tests/asset/main.cpp.
// Only what every asset test needs: the pulse asset API and the common
// load helpers. Anything used by a single test lives in that test file;
// definitions shared by several tests live in test_text.h, test_slow.h,
// test_fail.h, and test_builder.h.
#pragma once

#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "pulse_app.h"
#include "pulse_asset.h"
#include "pulse_vfs.h"

static void asset_test_add_root(const char* root) {
    assert(pulse_vfs_mount(root, "/", false));
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