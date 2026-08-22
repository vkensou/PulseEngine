#include "test_common.h"

int main() {
    PulseAppId app = make_transform_app("t-empty");
    assert(pulse_app_prepare(app) == PULSE_APP_PREPARE_RESULT_OK);
    assert(pulse_app_update(app) == PULSE_APP_UPDATE_RESULT_OK);
    pulse_destroy_app(app);
    return 0;
}
