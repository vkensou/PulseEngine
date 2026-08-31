// Shared builder asset definitions: cut from tests/asset/test_common.h.
// Used by the builders and builder-async tests.
#pragma once

#include "test_common.h"

const uint64_t builder_type = 11;

struct builder_asset {
    int value;
};

struct builder_settings {
    int value;
    const int* external;
};