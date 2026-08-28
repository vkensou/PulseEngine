#include "pulse_config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <string>

int main() {
    const char* json = "{\"name\":\"pulse\",\"width\":1280,\"height\":720.5,\"ok\":true,\"nothing\":null,\"list\":[\"a\",1,true,{}]}";
    PulseConfig* cfg = pulse_config_create_from_json(json, strlen(json));
    assert(cfg != nullptr);

    assert(pulse_config_has(cfg, "name"));
    assert(strcmp(pulse_config_get_string(cfg, "name", ""), "pulse") == 0);
    assert(pulse_config_get_int(cfg, "width", 0) == 1280);
    assert(pulse_config_get_double(cfg, "height", 0.0) == 720.5);
    assert(pulse_config_get_bool(cfg, "ok", false) == true);
    assert(pulse_config_get_type(cfg, "nothing") == PULSE_CONFIG_TYPE_NONE);

    PulseConfigArray* arr = pulse_config_get_array(cfg, "list");
    assert(arr != nullptr);
    assert(pulse_config_array_count(arr) == 4);

    PulseConfig* first = pulse_config_array_get(arr, 0);
    assert(first != nullptr);
    assert(strcmp(pulse_config_get_string(first, nullptr, ""), "a") == 0);

    PulseConfig* third = pulse_config_array_get(arr, 2);
    assert(third != nullptr);
    assert(pulse_config_get_bool(third, nullptr, false) == true);

    PulseConfig* fourth = pulse_config_array_get(arr, 3);
    assert(fourth != nullptr);
    assert(pulse_config_get_type(fourth, nullptr) == PULSE_CONFIG_TYPE_OBJECT);

    // Invalid JSON must fail and set an error message.
    PulseConfig* bad = pulse_config_create_from_json("{bad", 4);
    assert(bad == nullptr);
    assert(pulse_config_last_error() != nullptr);
    assert(pulse_config_last_error()[0] != '\0');

    // Excessive nesting must be rejected (depth > 64).
    std::string deep;
    for (int i = 0; i < 70; ++i) deep.push_back('[');
    deep.push_back('0');
    for (int i = 0; i < 70; ++i) deep.push_back(']');
    PulseConfig* too_deep = pulse_config_create_from_json(deep.data(), deep.size());
    assert(too_deep == nullptr);

    pulse_config_release(cfg);
    return 0;
}
