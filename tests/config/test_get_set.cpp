#include "pulse_config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    PulseConfig* cfg = pulse_config_create();
    assert(cfg != nullptr);

    pulse_config_set_bool(cfg, "b", true);
    pulse_config_set_int(cfg, "i", 42);
    pulse_config_set_double(cfg, "d", 1.5);
    pulse_config_set_string(cfg, "s", "hello");

    assert(pulse_config_get_bool(cfg, "b", false) == true);
    assert(pulse_config_get_int(cfg, "i", 0) == 42);
    assert(pulse_config_get_double(cfg, "d", 0.0) == 1.5);
    assert(strcmp(pulse_config_get_string(cfg, "s", ""), "hello") == 0);

    // Type conversion rules: double->int truncates, int->double promotes.
    assert(pulse_config_get_int(cfg, "d", 0) == 1);
    assert(pulse_config_get_double(cfg, "i", 0.0) == 42.0);
    assert(pulse_config_get_bool(cfg, "missing", true) == true);

    // Object child.
    PulseConfig* child = pulse_config_create();
    pulse_config_set_string(child, "k", "v");
    pulse_config_set_obj(cfg, "o", child);
    pulse_config_release(child);

    PulseConfig* got_obj = pulse_config_get_obj(cfg, "o");
    assert(got_obj != nullptr);
    assert(strcmp(pulse_config_get_string(got_obj, "k", ""), "v") == 0);

    // Array child (create an array by parsing a JSON array).
    PulseConfigArray* arr = (PulseConfigArray*)pulse_config_create_from_json("[1,2,3]", 7);
    assert(arr != nullptr);
    pulse_config_set_array(cfg, "arr", arr);
    pulse_config_release((PulseConfig*)arr);

    PulseConfigArray* got_arr = pulse_config_get_array(cfg, "arr");
    assert(got_arr != nullptr);
    assert(pulse_config_array_count(got_arr) == 3);
    assert(pulse_config_get_int(pulse_config_array_get(got_arr, 1), nullptr, 0) == 2);

    // Remove.
    assert(pulse_config_has(cfg, "i"));
    assert(pulse_config_remove(cfg, "i") == true);
    assert(!pulse_config_has(cfg, "i"));
    assert(pulse_config_remove(cfg, "i") == false);

    // Merge: overrides win, nested objects deep-merge.
    PulseConfig* defaults = pulse_config_create_from_json("{\"a\":1,\"nested\":{\"x\":10,\"y\":20}}", 39);
    PulseConfig* overrides = pulse_config_create_from_json("{\"a\":2,\"nested\":{\"y\":99}}", 30);
    assert(defaults && overrides);
    PulseConfig* merged = pulse_config_merge(defaults, overrides);
    assert(merged != nullptr);
    assert(pulse_config_get_int(merged, "a", 0) == 2);
    PulseConfig* nested = pulse_config_get_obj(merged, "nested");
    assert(nested != nullptr);
    assert(pulse_config_get_int(nested, "x", 0) == 10);
    assert(pulse_config_get_int(nested, "y", 0) == 99);

    pulse_config_release(merged);
    pulse_config_release(defaults);
    pulse_config_release(overrides);
    pulse_config_release(cfg);
    return 0;
}
