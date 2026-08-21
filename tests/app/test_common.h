// Shared app test definitions, cut from tests/app/main.cpp.
// Only what every app test needs: the pulse app API and a runner that
// advances the app a fixed number of frames. Anything used by a single
// test lives in that test file.
#pragma once

#undef NDEBUG
#include <assert.h>
#include <stdio.h>

#include "pulse_app.h"

static EPulseResult test_runner(PulseAppId app, void* ctx) {
    int* frames = (int*)ctx;
    for (int i = 0; i < *frames; ++i) {
        EPulseResult result = pulse_app_update(app);
        if (result != PULSE_RESULT_OK) {
            return result;
        }
    }
    return PULSE_RESULT_OK;
}