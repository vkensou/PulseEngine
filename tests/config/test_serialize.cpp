#include "pulse_config.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    const char* json = "{\"b\":true,\"n\":1,\"arr\":[1,\"x\"],\"o\":{\"k\":\"v\"}}";
    PulseConfig* cfg = pulse_config_create_from_json(json, strlen(json));
    assert(cfg != nullptr);

    size_t len = 0;
    char* text = pulse_config_to_json(cfg, &len);
    assert(text != nullptr);
    assert(len == strlen(text));
    assert(strstr(text, "\"b\":true") != nullptr);
    assert(strstr(text, "\"n\":1") != nullptr);
    assert(strstr(text, "\"k\":\"v\"") != nullptr);

    // Roundtrip.
    PulseConfig* cfg2 = pulse_config_create_from_json(text, len);
    assert(cfg2 != nullptr);
    assert(pulse_config_get_bool(cfg2, "b", false) == true);
    assert(pulse_config_get_int(cfg2, "n", 0) == 1);
    PulseConfigArray* arr = pulse_config_get_array(cfg2, "arr");
    assert(arr != nullptr && pulse_config_array_count(arr) == 2);
    assert(strcmp(pulse_config_get_string(pulse_config_array_get(arr, 1), nullptr, ""), "x") == 0);

    char* pretty = pulse_config_to_json_pretty(cfg, nullptr);
    assert(pretty != nullptr);
    assert(strstr(pretty, "\n") != nullptr);
    assert(strstr(pretty, "\"b\": true") != nullptr);

    pulse_config_free_string(text);
    pulse_config_free_string(pretty);
    pulse_config_release(cfg2);
    pulse_config_release(cfg);
    return 0;
}
