#include "pulse_config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    PulseConfig* parent = pulse_config_create_from_json("{\"child\":{\"x\":1}}", strlen("{\"child\":{\"x\":1}}"));
    assert(parent != nullptr);

    PulseConfig* child = pulse_config_get_obj(parent, "child");
    assert(child != nullptr);

    // Long-term hold: child survives parent release.
    pulse_config_addref(child);
    pulse_config_release(parent);

    assert(pulse_config_get_int(child, "x", 0) == 1);

    pulse_config_release(child);
    return 0;
}
